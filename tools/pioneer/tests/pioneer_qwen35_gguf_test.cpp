/* SPDX-License-Identifier: MIT */
// Actual packed reader and role checks, independently authenticated file.
#include "../PioneerQwen35FleetPlan.h"
#include "../PioneerQwen35Preload.h"
#include "../PioneerQwen35ManifestValidation.h"
#include "../PioneerModelWeightCache.h"
#include "../TenstorrentGgufQuant.h"
#include <array>
#include <cassert>
#include <fcntl.h>
#include <memory>
#include <unistd.h>

static bool SameFile(const struct stat& a,const struct stat& b) {
 if (a.st_dev!=b.st_dev || a.st_ino!=b.st_ino || a.st_size!=b.st_size
  || a.st_mtime!=b.st_mtime || a.st_ctime!=b.st_ctime) return false;
#if defined(__APPLE__)
 return a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec;
#else
 return a.st_mtim.tv_nsec==b.st_mtim.tv_nsec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
#endif
}

static void CheckPreload(PioneerQwen35Gguf& model, const PioneerQwen35PackedPlan& plan,
 const PioneerQwen35::DeviceLayout& manifest) {
 namespace D=PioneerQwen35;
 D::Range rope{};
 assert(D::ResolveRope(manifest,plan,rope) && rope.bank==0 && rope.bytes==128*D::Rotary*4);
 std::array<uint8_t,D::LoadPageBytes> ropePage{};
 D::Range partial={0,rope.address,17*D::Rotary*4};
 assert(D::FillRope(partial,D::LoadPageBytes,ropePage.size(),ropePage.data()));
 float pair[2];
 memcpy(pair,ropePage.data(),sizeof(pair));
 for (unsigned i=D::Rotary*4; i<ropePage.size(); i++) assert(!ropePage[i]);
 assert(pair[0]==std::cos(16.0f) && pair[1]==std::sin(16.0f));
 assert(!D::FillRope(rope,1,ropePage.size(),ropePage.data()));

 auto buffer=std::make_unique<uint8_t[]>(D::LoadSpanBytes);
 const auto& parts=plan.Parts();
 const unsigned pagesPerBank=unsigned(plan.Capacity/D::LoadPageBytes);
 std::vector<uint8_t> initialized(D::Banks*pagesPerBank);
 uint64_t expected=rope.bytes;
 for (const auto& part:parts)
  if (part.kind==PIONEER_Q35_WEIGHT) expected+=part.allocated;
 unsigned fragment=0, at=0, ropeAt=0, calls=0;
 uint64_t accepted=0, reported=0;
 auto check=[&](unsigned bank,unsigned offset,unsigned bytes,const uint8_t* data) {
  assert(bytes && bytes<=D::LoadSpanBytes && !(bytes&(D::LoadPageBytes-1)));
  while (fragment<parts.size() && parts[fragment].kind!=PIONEER_Q35_WEIGHT) fragment++;
  if (fragment<parts.size()) {
   const auto& part=parts[fragment];
   assert(bank==part.bank && offset==part.proposedAddress-D::BankBase+at && at+bytes<=part.allocated);
   assert(plan.ReadWeightPage(model,fragment,(at+bytes)/D::LoadPageBytes-1,ropePage.data(),ropePage.size()));
   assert(!memcmp(data+bytes-ropePage.size(),ropePage.data(),ropePage.size()));
   at+=bytes;
   if (at==part.allocated) { fragment++; at=0; }
  } else {
   assert(bank==rope.bank && offset==rope.address-D::BankBase+ropeAt && ropeAt+bytes<=rope.bytes);
   assert(D::FillRope(rope,ropeAt,ropePage.size(),ropePage.data()));
   assert(!memcmp(data,ropePage.data(),ropePage.size()));
   ropeAt+=bytes;
  }
  for (unsigned i=0; i<bytes/D::LoadPageBytes; i++) {
   auto& bit=initialized.at(bank*pagesPerBank+offset/D::LoadPageBytes+i);
   assert(!bit);
   bit=1;
  }
  accepted+=bytes;
  calls++;
  return true;
 };
 assert(!D::UploadModel(model,plan,manifest,buffer.get(),D::LoadSpanBytes-1,check,reported));
 assert(!calls && !reported);
 unsigned rejected=0;
 auto refuse=[&](unsigned,unsigned,unsigned,const uint8_t*) { return ++rejected<2; };
 assert(!D::UploadModel(model,plan,manifest,buffer.get(),D::LoadSpanBytes,refuse,reported));
 assert(rejected==2 && reported==D::LoadSpanBytes);
 assert(D::UploadModel(model,plan,manifest,buffer.get(),D::LoadSpanBytes,check,reported));
 assert(ropeAt==rope.bytes && accepted==reported && reported==expected);
 auto uploaded=[&](unsigned bank,unsigned page) {
  return initialized.at(bank*pagesPerBank+page)!=0;
 };
 const unsigned count=unsigned(reported/D::LoadPageBytes);
 assert(D::ValidateManifest(manifest,rope,uploaded,count));
 assert(!D::ValidateManifest(manifest,rope,uploaded,count-1));
 D::Range shifted=rope;
 shifted.address+=D::LoadPageBytes;
 assert(!D::ValidateManifest(manifest,shifted,uploaded,count));
 const auto& first=parts.front();
 auto& weightPage=initialized.at(first.bank*pagesPerBank+(first.proposedAddress-D::BankBase)/D::LoadPageBytes);
 weightPage=0;
 assert(!D::ValidateManifest(manifest,rope,uploaded,count));
 weightPage=1;
 const auto& state=manifest.layers[0].delta[0].parts[0];
 auto& statePage=initialized.at(state.bank*pagesPerBank+(state.address-D::BankBase)/D::LoadPageBytes);
 assert(!statePage);
 statePage=1;
 assert(!D::ValidateManifest(manifest,rope,uploaded,count));
 statePage=0;
 auto aliased=std::make_unique<D::DeviceLayout>(manifest);
 aliased->layers[0].delta[1].parts[0]=state;
 assert(!D::ValidateManifest(*aliased,rope,uploaded,count));
 printf("PASS: complete preload byte stream, %u spans and %llu padded bytes\n",calls,
  static_cast<unsigned long long>(reported));
}

int main(int argc,char** argv) {
 assert(argc==2 || (argc==3 && (!strcmp(argv[1],"--reject") || !strcmp(argv[1],"--mutation"))));
 const bool special=argc==3, mutation=special && !strcmp(argv[1],"--mutation");
 int fd=open(argv[special?2:1],(mutation?O_RDWR:O_RDONLY)|O_NOFOLLOW|O_NONBLOCK|O_CLOEXEC); assert(fd>=0);
 std::unique_ptr<FILE,decltype(&fclose)> source(fdopen(fd,"rb"),&fclose); assert(source);
 if (mutation) {
  PioneerQwen35Gguf changing; assert(changing.Open(source.get()) && changing.Stable());
  const auto* tensor=changing.TiedHead(); assert(tensor);
  uint8_t byte=1;
  assert(pwrite(fd,&byte,1,off_t(tensor->offset))==1);
  assert(!changing.Stable());
  assert(!changing.ReadPacked(0,0,1,&byte,1));
  return 0;
 }
 if (special) {
  PioneerQwen35Gguf invalid;
  assert(!invalid.Open(source.get()) && !invalid.Stable() && invalid.Tensors().empty() && !invalid.TiedHead());
  return 0;
 }
 struct stat before{},after{}; assert(!fstat(fd,&before));
 PioneerModelWeightCache::Digest digest{}; std::string error;
 assert(PioneerModelWeightCache::HashFile(fd,PioneerQwen35Gguf::FullBytes,digest,error));
 assert(PioneerModelWeightCache::Hex(digest)=="00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4");
 assert(!fstat(fd,&after) && SameFile(before,after));
 PioneerQwen35Gguf model; assert(model.Open(source.get()) && model.Stable() && model.Tensors().size()==426);
 assert(model.TiedHead() && model.TiedHead()==model.Find("token_embd.weight") && !model.Find("output.weight"));
 assert(model.TiedHead()->type==14 && model.TiedHead()->inputs==2560 && model.TiedHead()->outputs==248320);
 PioneerQwen35TiledReader tiled; assert(tiled.Bind(model) && !tiled.Bind(model));
 unsigned formats[15]={}; std::array<uint8_t,210> read{},direct{};
 unsigned pages=0,groups=0;
 for (unsigned i=0;i<model.Tensors().size();i++) {
  const auto& tensor=model.Tensors()[i]; formats[tensor.type]++;
  unsigned count=PioneerQ35BlockBytes(tensor.type);
  assert(count && count<=read.size());
  for (uint64_t pos:{UINT64_C(0),tensor.bytes-count}) {
   assert(model.ReadPacked(i,pos,count,read.data(),read.size()));
   assert(pread(fd,direct.data(),count,off_t(tensor.offset+pos))==ssize_t(count));
   assert(!memcmp(read.data(),direct.data(),count));
  }
  assert(!model.ReadPacked(i,tensor.bytes,count,read.data(),read.size()));
  assert(!model.ReadPacked(i,0,count,read.data(),count-1));
  if (!tensor.type) continue;
  const unsigned blocks=unsigned(tensor.inputs/PioneerQ35Block(tensor.type));
  const unsigned originalRow=blocks*count, tileBytes=PioneerQ35TileBytes(tensor.type);
  const unsigned tiledRow=PioneerQ35TiledRowBytes(tensor.type,tensor.inputs);
  assert(tiledRow && uint64_t(tiledRow)*16==uint64_t(tileBytes)*blocks);
  for (unsigned first:{0U,unsigned(tensor.outputs)-16}) {
   std::vector<uint8_t> original(size_t(originalRow)*16),expected(size_t(tiledRow)*16);
   assert(pread(fd,original.data(),original.size(),off_t(tensor.offset+uint64_t(first)*originalRow))==ssize_t(original.size()));
   for (unsigned block=0;block<blocks;block++) for (unsigned b=0;b<count;b++) for (unsigned row=0;row<16;row++)
    expected[block*tileBytes+b*16+row]=original[row*originalRow+block*count+b];
   for (unsigned page=0;page*4096<expected.size();page++) {
    std::array<uint8_t,4096> result{};
    assert(tiled.ReadPage(model,i,first,16,page,result.data(),result.size()));
    for (unsigned b=0;b<4096;b++) {
     size_t at=size_t(page)*4096+b;
     assert(result[b]==(at<expected.size()?expected[at]:0));
    }
    pages++;
   }
   groups++;
  }
 }
 assert(formats[0]==177 && formats[8]==48 && formats[12]==131 && formats[13]==48 && formats[14]==22);
 assert(!model.ReadPacked(426,0,1,read.data(),read.size()));
 assert(!model.ReadPacked(0,0,0,read.data(),read.size()));
 assert(!model.ReadPacked(0,0,1,nullptr,read.size()));
 std::array<uint8_t,4096> invalid; invalid.fill(0xa5); auto beforeInvalid=invalid;
 assert(!tiled.ReadPage(model,0,0,16,0,invalid.data(),invalid.size())); // F32
 assert(!tiled.ReadPage(model,1,1,16,0,invalid.data(),invalid.size()));
 assert(!tiled.ReadPage(model,1,0,16,UINT32_MAX,invalid.data(),invalid.size()));
 assert(!tiled.ReadPage(model,1,0,16,0,invalid.data(),invalid.size()-1));
 assert(invalid==beforeInvalid && groups==498);
 PioneerQwen35PackedPlan plan;
 assert(!plan.Build(model,0) && !plan.Ready() && plan.Parts().empty());
 assert(!plan.Build(model,32769) && !plan.Ready());
 assert(plan.Build(model,128,true) && plan.Ready());
 namespace D=PioneerQwen35;
 auto device=std::make_unique<D::DeviceLayout>();
 assert(!D::BuildDeviceLayout(model,plan,127,*device));
 assert(D::BuildDeviceLayout(model,plan,128,*device));
 assert(device->magic==D::LayoutMagic && device->version==D::LayoutVersion && device->context==128);
 uint64_t bankEnds[7]={}; unsigned weightRows[426]={},stateRows[32][2]={},convRows[32][2]={},kvRows[32]={},workspaces=0;
 for (unsigned partIndex=0;partIndex<plan.Parts().size();partIndex++) {
  const auto& part=plan.Parts()[partIndex];
  assert(part.bank<7 && part.proposedAddress==PioneerQwen35PackedPlan::Base+bankEnds[part.bank]
   && part.bytes && part.allocated%4096==0 && part.bytes<=part.allocated
   && part.allocated-part.bytes<4096);
  bankEnds[part.bank]+=part.allocated;
  D::Range range{};
  if (part.kind==PIONEER_Q35_WEIGHT) {
   assert(part.tensor<426 && part.firstRow==weightRows[part.tensor]);
   weightRows[part.tensor]+=part.rows;
   const auto& t=model.Tensors()[part.tensor];
   assert(part.rowBytes==(t.type?PioneerQ35TiledRowBytes(t.type,t.inputs):t.bytes/t.outputs));
   assert(part.bytes==uint64_t(part.rows)*part.rowBytes);
   std::array<uint8_t,4096> actual{},expected{};
   for (unsigned page:{0U,(part.bytes-1)/4096}) {
    assert(plan.ReadWeightPage(model,partIndex,page,actual.data(),actual.size()));
    if (t.type) assert(tiled.ReadPage(model,part.tensor,part.firstRow,part.rows,page,expected.data(),expected.size()));
    else {
     expected.fill(0); unsigned n=unsigned(std::min(UINT64_C(4096),uint64_t(part.bytes)-uint64_t(page)*4096));
     assert(pread(fd,expected.data(),n,off_t(t.offset+uint64_t(part.firstRow)*part.rowBytes+uint64_t(page)*4096))==ssize_t(n));
    }
    assert(actual==expected);
   }
  } else if (part.kind==PIONEER_Q35_DELTA_STATE) {
   assert(part.layer<32 && part.layer%4!=3 && part.role<2 && part.rowBytes==128*128*4);
   assert(part.firstRow==stateRows[part.layer][part.role]); stateRows[part.layer][part.role]+=part.rows;
   assert(device->ResolveDelta(part.layer,part.role,part.firstRow,range)
    && range.bank==part.bank && range.address==part.proposedAddress && range.bytes==part.rowBytes);
  } else if (part.kind==PIONEER_Q35_CONV_STATE) {
   assert(part.layer<32 && part.layer%4!=3 && part.role<2 && part.rowBytes==12
    && part.firstRow==convRows[part.layer][part.role]);
   convRows[part.layer][part.role]+=part.rows;
   assert(device->ResolveConv(part.layer,part.role,part.firstRow,range)
    && range.bank==part.bank && range.address==part.proposedAddress && range.bytes==16*part.rowBytes);
  } else if (part.kind==PIONEER_Q35_KV_STATE) {
   assert(part.layer<32 && part.layer%4==3 && part.rowBytes==128*256*2 && part.firstRow==kvRows[part.layer]);
   kvRows[part.layer]+=part.rows;
   assert(device->ResolveKv(part.layer,part.firstRow%4,127,part.firstRow>=4,range)
    && range.bank==part.bank && range.address==part.proposedAddress+127*512 && range.bytes==512);
  } else { assert(part.kind==PIONEER_Q35_WORKSPACE && part.bytes==64*1024*1024); workspaces++; }
 }
 for (unsigned i=0;i<426;i++) assert(weightRows[i]==model.Tensors()[i].outputs);
 unsigned projectionCount=0,jobCount=0;
 for (unsigned tensor=0;tensor<model.Tensors().size();tensor++) {
  const auto& t=model.Tensors()[tensor];
  std::vector<PioneerQ35ProjectionBand> jobs;
  if (!t.type) {
   assert(!PioneerQ35BuildProjection(model,plan,tensor,119,jobs) && jobs.empty());
   continue;
  }
  assert(PioneerQ35BuildProjection(model,plan,tensor,119,jobs) && !jobs.empty());
  for (unsigned i=0;i<jobs.size();i++) {
   const auto& job=jobs[i]; const auto& part=plan.Parts().at(job.fragment);
   assert(job.wave==i/119 && job.worker==i%119 && job.bank==part.bank);
   assert(part.tensor==tensor && job.firstRow>=part.firstRow && job.groups>0 && job.groups<=16);
   const unsigned row=job.firstRow-part.firstRow;
   assert(row+job.groups*16<=part.rows && !(row&15));
   assert(job.address==part.proposedAddress+row*part.rowBytes && !(job.address&63));
   assert(job.bytes==job.groups*16*part.rowBytes);
   D::Range range{};
   assert(device->Weight(part.layer,part.role)->ResolveBand(job.firstRow,job.groups,range));
   assert(range.bank==job.bank && range.address==job.address && range.bytes==job.bytes);
  }
  std::sort(jobs.begin(),jobs.end(),[](const auto& a,const auto& b){return a.firstRow<b.firstRow;});
  unsigned end=0;
  for (const auto& job:jobs) {
   assert(job.firstRow==end);
   end+=job.groups*16;
  }
  assert(end==t.outputs);
  projectionCount++;
  jobCount+=jobs.size();
 }
 assert(projectionCount==249);
 namespace E=PioneerQwen35Embedding;
 const unsigned tied=unsigned(model.TiedHead()-model.Tensors().data());
 unsigned embeddings=0;
 for (const auto& part:plan.Parts()) {
  if (part.kind!=PIONEER_Q35_WEIGHT || part.tensor!=tied) continue;
  for (unsigned token:{part.firstRow,part.firstRow+15,part.firstRow+part.rows-1}) {
   PioneerQ35EmbeddingProposal location{};
   assert(plan.ResolveEmbedding(token,location) && location.bank==part.bank);
   assert(&plan.Parts().at(location.part)==&part && location.lane==token%16);
   D::Range range{}; unsigned lane=~0U;
   assert(device->ResolveEmbedding(token,range,lane) && lane==location.lane
    && range.bank==location.bank && range.address==location.address && range.bytes==E::GroupBytes);
   std::array<uint8_t,E::GroupBytes> data{};
   const unsigned relative=location.address-part.proposedAddress;
   for (unsigned copied=0;copied<data.size();) {
    std::array<uint8_t,4096> page{};
    const unsigned absolute=relative+copied,within=absolute%4096;
    assert(plan.ReadWeightPage(model,location.part,absolute/4096,page.data(),page.size()));
    const unsigned count=std::min(4096-within,unsigned(data.size())-copied);
    memcpy(data.data()+copied,page.data()+within,count);
    copied+=count;
   }
   for (unsigned block=0;block<E::Blocks;block++) {
    std::array<float,256> decoded{},reference{};
    std::array<uint8_t,210> raw{};
    assert(model.ReadPacked(tied,(uint64_t(token)*E::Blocks+block)*raw.size(),raw.size(),raw.data(),raw.size()));
    assert(E::DecodeBlock(data.data()+block*E::TileBytes,E::TileBytes,location.lane,decoded.data(),decoded.size()));
    assert(PioneerDecodeQ6K(raw.data(),reference.data()) && !memcmp(decoded.data(),reference.data(),sizeof(decoded)));
   }
   embeddings++;
  }
 }
 PioneerQ35EmbeddingProposal bad{};
 assert(embeddings==21 && !plan.ResolveEmbedding(E::Vocabulary,bad));
 D::Range refused{};
 assert(!device->ResolveKv(3,0,128,false,refused) && !device->ResolveKv(2,0,0,false,refused));
 assert(!device->ResolveDelta(3,0,0,refused) && !device->ResolveConv(0,0,1,refused));
 for (unsigned layer=0;layer<32;layer++) {
  if (layer%4==3) assert(kvRows[layer]==8 && !convRows[layer][0] && !convRows[layer][1]
   && !stateRows[layer][0] && !stateRows[layer][1]);
  else assert(!kvRows[layer] && convRows[layer][0]==8192 && convRows[layer][1]==8192
   && stateRows[layer][0]==32 && stateRows[layer][1]==32);
 }
 assert(workspaces==7);
 for (unsigned bank=0;bank<7;bank++) assert(bankEnds[bank]==plan.Used(bank) && bankEnds[bank]<=plan.Capacity);
 assert(!plan.ReadWeightPage(model,plan.Parts().size(),0,invalid.data(),invalid.size()));
 CheckPreload(model,plan,*device);
 assert(model.Stable() && !fstat(fd,&after) && SameFile(before,after));
 printf("PASS: 4B GGUF, 426 roles, %u tiled groups, %u pages, %u exact tied Q6_K embedding rows and bounded state plan\n",groups,pages,embeddings);
 printf("PASS: complete packed coverage for %u projections in %u bounded worker bands\n",projectionCount,jobCount);
}
