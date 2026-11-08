/* SPDX-License-Identifier: MIT */
// Small, separately named boot-volume persistence probe. Never opens a GPU,
// changes a driver, removes files, or writes directly to a disk device.
// A PASS before reboot is not proof of post-power-loss durability.
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __HAIKU__
#include <Drivers.h>
#include <fs_info.h>
#include <sys/ioctl.h>
#if defined(PIONEER_BFS_PROBE_TEST)
#error The host-only fake-flush backend must never be built for Haiku
#endif
#else
#ifndef PIONEER_BFS_PROBE_TEST
#error This diagnostic is only for Haiku or explicit host-only tests
#endif
#endif

static const char* const kNames[]={"small41","small38","log4704","blocks131072"};
static const size_t kSizes[]={41,38,4704,131072};
static const char* const kPrefix="pioneer-bfs-durability-";
static const size_t kCount=sizeof(kSizes)/sizeof(kSizes[0]);

static const char* Parent(){
#ifdef PIONEER_BFS_PROBE_TEST
 const char* p=getenv("PIONEER_BFS_PROBE_TEST_PARENT");return p && p[0]=='/'?p:nullptr;
#else
 return "/boot/home";
#endif
}
static bool Volume(const char* expected,dev_t& device){
 struct stat parent;
 if(!Parent() || lstat(Parent(),&parent)!=0 || !S_ISDIR(parent.st_mode) || parent.st_uid!=geteuid())return false;
 device=parent.st_dev;
#ifdef __HAIKU__
 fs_info info={};const dev_t boot=dev_for_path("/boot");
 if(boot<0 || boot!=device || fs_stat_dev(boot,&info)!=0 || strcmp(info.fsh_name,"bfs")!=0
  || strncmp(info.device_name,"/dev/disk/",10)!=0 || strcmp(info.device_name,expected)!=0)return false;
 printf("VOLUME bfs %s\n",info.device_name);
#else
 if(strcmp(expected,"/dev/pioneer-test")!=0)return false;
 printf("VOLUME host-test %s\n",expected);
#endif
 return true;
}
static bool CheckedFlush(const char* device){
#ifdef __HAIKU__
 // sync() asks each mounted filesystem to flush but has no error result; an
 // explicit checked device flush follows. Neither proves actual persistence.
 sync();int fd=open(device,O_RDONLY | O_CLOEXEC);
 if(fd<0){perror("open boot device");return false;}
 int result=ioctl(fd,B_FLUSH_DRIVE_CACHE,nullptr,size_t(0));int saved=errno;
 if(close(fd)!=0 && result==0){perror("close boot device");return false;}
 if(result!=0){errno=saved;perror("boot device flush");return false;}
#else
 (void)device;const char* fail=getenv("PIONEER_BFS_PROBE_TEST_FAIL");
 if(fail && strcmp(fail,"flush")==0){errno=EIO;perror("simulated boot device flush");return false;}
#endif
 return true;
}
static uint64_t Seed(const char* path){
 uint64_t hash=UINT64_C(1469598103934665603);
 for(const unsigned char* p=(const unsigned char*)path;*p;p++)hash=(hash^*p)*UINT64_C(1099511628211);
 return hash;
}
static unsigned char Pattern(uint64_t seed,size_t file,size_t offset){
 uint64_t x=seed ^ (UINT64_C(0x9e3779b97f4a7c15)*(file+1)) ^ (UINT64_C(0xbf58476d1ce4e5b9)*(offset+1));
 x^=x>>30;x*=UINT64_C(0xbf58476d1ce4e5b9);x^=x>>27;x*=UINT64_C(0x94d049bb133111eb);x^=x>>31;
 return (unsigned char)x;
}
static bool StagePath(const char* path){
 const char* parent=Parent();if(!parent)return false;size_t n=strlen(parent),p=strlen(kPrefix);
 if(strncmp(path,parent,n)!=0 || path[n]!='/' || strncmp(path+n+1,kPrefix,p)!=0)return false;
 const char* suffix=path+n+1+p;if(strlen(suffix)!=6)return false;
 for(;*suffix;suffix++)if(!((*suffix>='a' && *suffix<='z') || (*suffix>='A' && *suffix<='Z') || (*suffix>='0' && *suffix<='9')))return false;
 return true;
}
static bool Verify(const char* stage,dev_t device){
 struct stat directory;if(!StagePath(stage) || lstat(stage,&directory)!=0 || !S_ISDIR(directory.st_mode)
  || directory.st_dev!=device || directory.st_uid!=geteuid() || (directory.st_mode&0777)!=0700)return false;
 const uint64_t seed=Seed(stage);
 for(size_t i=0;i<kCount;i++){
  char path[1024];if(snprintf(path,sizeof(path),"%s/%s",stage,kNames[i])>=int(sizeof(path)))return false;
  int fd=open(path,O_RDONLY | O_NOFOLLOW | O_CLOEXEC);if(fd<0)return false;
  struct stat st;bool okay=fstat(fd,&st)==0 && S_ISREG(st.st_mode) && st.st_dev==device && st.st_uid==geteuid()
   && st.st_nlink==1 && (st.st_mode&0777)==0600 && st.st_size==off_t(kSizes[i]);
  unsigned char data[4096];size_t offset=0;
  while(okay && offset<kSizes[i]){
   const size_t want=kSizes[i]-offset<sizeof(data)?kSizes[i]-offset:sizeof(data);
   ssize_t got=read(fd,data,want);if(got<=0){okay=false;break;}
   for(ssize_t j=0;j<got;j++)if(data[j]!=Pattern(seed,i,offset+size_t(j))){okay=false;break;}
   offset+=size_t(got);
  }
  if(close(fd)!=0)okay=false;
  if(!okay)return false;
 }
 return true;
}
static bool Create(const char* expected,dev_t device){
 char stage[1024];if(snprintf(stage,sizeof(stage),"%s/%sXXXXXX",Parent(),kPrefix)>=int(sizeof(stage)))return false;
 umask(077);if(!mkdtemp(stage)){perror("mkdtemp");return false;}
 printf("STAGE %s\n",stage);fflush(stdout);const uint64_t seed=Seed(stage);
 for(size_t i=0;i<kCount;i++){
  char path[1024];if(snprintf(path,sizeof(path),"%s/%s",stage,kNames[i])>=int(sizeof(path)))return false;
  int fd=open(path,O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,0600);if(fd<0){perror("create probe file");return false;}
  unsigned char data[4096];size_t offset=0;bool okay=true;
  while(offset<kSizes[i]){
   const size_t want=kSizes[i]-offset<sizeof(data)?kSizes[i]-offset:sizeof(data);
   for(size_t j=0;j<want;j++)data[j]=Pattern(seed,i,offset+j);
   size_t written=0;while(written<want){ssize_t n=write(fd,data+written,want-written);if(n<=0){okay=false;break;}written+=size_t(n);}
   if(!okay)break;
   offset+=want;
  }
  if(okay && fsync(fd)!=0)okay=false;
  if(close(fd)!=0)okay=false;
  if(!okay){perror("write/fsync probe file");return false;}
 }
 if(!CheckedFlush(expected) || !Verify(stage,device))return false;
 printf("PASS_CREATE all four files read back after checked device flush; post-reboot durability UNTESTED\n");return true;
}
int main(int argc,char** argv){
 setvbuf(stdout,nullptr,_IONBF,0);dev_t device;
 if(argc==3 && strcmp(argv[1],"--create")==0){
  if(!Volume(argv[2],device) || !Create(argv[2],device)){fprintf(stderr,"STOP probe incomplete; preserve any printed stage\n");return 1;}return 0;
 }
 if(argc==4 && strcmp(argv[1],"--verify")==0){
  if(!Volume(argv[3],device) || !Verify(argv[2],device)){fprintf(stderr,"STOP probe verification failed; do not infer device safety\n");return 1;}
  puts("PASS_VERIFY all probe bytes match; this does not verify other files or device health");return 0;
 }
 if(argc==3 && strcmp(argv[1],"--flush-only")==0){
  if(!Volume(argv[2],device) || !CheckedFlush(argv[2])){fprintf(stderr,"STOP checked boot-device flush failed or unverified\n");return 1;}
  puts("PASS_CHECKED_DEVICE_FLUSH sync errors are not observable; no power-loss or file-durability guarantee");return 0;
 }
 fprintf(stderr,"usage: %s --create EXPECTED_BOOT_DEVICE | --verify STAGE EXPECTED_BOOT_DEVICE | --flush-only EXPECTED_BOOT_DEVICE\n",argv[0]);return 2;
}
