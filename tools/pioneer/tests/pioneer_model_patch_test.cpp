/* SPDX-License-Identifier: MIT */
#define PIONEER_PATCH_TEST_ABI
#define main PatchMain
#include "../pioneer_model_patch.cpp"
#undef main
#include <cassert>
#include <string>
#include <vector>
static void Write(const std::string& path,const std::vector<char>& data)
{
	FILE* file=fopen(path.c_str(),"wb"); assert(file);
	assert(fwrite(data.data(),1,data.size(),file)==data.size() && !fclose(file));
}
static std::vector<char> Read(const std::string& path)
{
	struct stat st; assert(!stat(path.c_str(),&st)); std::vector<char> data(size_t(st.st_size));
	FILE* file=fopen(path.c_str(),"rb"); assert(file);
	assert(fread(data.data(),1,data.size(),file)==data.size() && !fclose(file)); return data;
}
int main()
{
	char directory[]="/private/tmp/pioneer-model-patch.XXXXXXXX"; assert(mkdtemp(directory));
	std::string base=directory,target=base+"/target",bad=base+"/bad",fresh=base+"/fresh",link=base+"/link";
	const unsigned mib=1024*1024;
	std::vector<char> before(6*mib),original(4*mib),replacement(2*mib);
	for (unsigned i=0;i<before.size();i++) before[i]=char(i*17+29);
	for (unsigned i=0;i<original.size();i++) original[i]=char(i*31+37);
	for (unsigned i=0;i<replacement.size();i++) replacement[i]=char(i*13+71);
	memcpy(before.data()+2*mib,original.data()+mib,2*mib);
	char* argv[]={const_cast<char*>("patch"),const_cast<char*>("--apply"),const_cast<char*>(target.c_str()),const_cast<char*>(bad.c_str()),const_cast<char*>(fresh.c_str())};
	auto reset=[&] { Write(target,before); Write(bad,original); Write(fresh,replacement); };
	reset(); assert(PatchMain(5,argv)==0); auto expected=before;
	memcpy(expected.data()+2*mib,replacement.data(),2*mib);
	assert(Read(target)==expected && Read(bad)==original && Read(fresh)==replacement);
	assert(PatchMain(5,argv)==1 && Read(target)==expected); // refuses replay,before any write
	for (unsigned position:{0U,mib-1,mib,2*mib-1}) {
		reset(); auto changed=before; changed[2*mib+position]^=1; Write(target,changed);
		assert(PatchMain(5,argv)==1 && Read(target)==changed && Read(bad)==original);
	}
	for (unsigned fault=0;fault<6;fault++) {
		reset();
		if (fault==0) assert(!truncate(target.c_str(),6*mib-1));
		if (fault==1) assert(!truncate(bad.c_str(),4*mib-1));
		if (fault==2) assert(!truncate(fresh.c_str(),2*mib-1));
		if (fault==3) { assert(!unlink(fresh.c_str())); assert(!symlink(bad.c_str(),fresh.c_str())); }
		if (fault==4) { assert(!unlink(bad.c_str())); assert(!symlink(fresh.c_str(),bad.c_str())); }
		if (fault==5) assert(!::link(target.c_str(),link.c_str()));
		auto intact=Read(target); assert(PatchMain(5,argv)==1 && Read(target)==intact);
		if (fault==3) assert(!unlink(fresh.c_str()));
		if (fault==4) assert(!unlink(bad.c_str()));
		if (fault==5) assert(!unlink(link.c_str()));
	}
	assert(!unlink(target.c_str()) && !unlink(bad.c_str()) && !unlink(fresh.c_str()) && !rmdir(directory));
	puts("PASS: exact unpublished-model leaf patch,whole old-range preflight before ANY write,all surrounding bytes/originals preserved,replay/last-block mismatch/length/symlink/hardlink refusal;NO_GPU");
}
