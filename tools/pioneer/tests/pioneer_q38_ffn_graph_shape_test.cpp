/* SPDX-License-Identifier: MIT */
#include "pioneer_q38_ffn_graph_fixture.h"
#include <cstdio>
int main(){unsigned positive=0,refused=0;
 for(unsigned b=0;b<7;b++)for(bool edge:{false,true})for(bool wide:{false,true})for(bool reverse:{false,true})for(unsigned types=0;types<8;types++) {
  FfnGraphFixture f(b,edge,wide,reverse,types);assert(PioneerQ38FfnGraphManifest(f.manifest.data(),f.source.data()));positive++;
  for(unsigned w=0;w<119;w++)assert(PioneerQ38FfnGraphNextPeer(f.manifest.data(),w)==unsigned(w<20)+unsigned(w<68)+3*unsigned(w<(wide?119U:20U))
   +(wide?0U:2*unsigned(w>=20 && w<68)));
 }
 FfnGraphFixture canonical;const auto saved=canonical.manifest;
 auto refuse=[&](){assert(!PioneerQ38FfnGraphManifest(canonical.manifest.data(),canonical.source.data()));refused++;canonical.manifest=saved;};
 for(unsigned i:{0U,1U,2U,3U,4U,5U,7U,8U,9U,10U,11U,12U,13U,14U,15U}){canonical.manifest[i]^=1;refuse();}
 for(unsigned i=43;i<64;i++){canonical.manifest[i]=1;refuse();}
 for(unsigned i=183;i<192;i++){canonical.manifest[i]=1;refuse();}
 for(unsigned w=0;w<119;w++){canonical.manifest[64+w]=1;refuse();}
 for(unsigned role=0;role<3;role++) {
  unsigned at=PioneerQ38FfnGraphRoleOffset(role);
  for(unsigned i:{0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U,1087U}){canonical.manifest[at+i]^=1;refuse();}
  for(unsigned w=0;w<119;w++)for(unsigned field=0;field<9;field++) {
   unsigned i=field<8?at+16+w*8+field:at+16+119*8+w;
   if(field==1)canonical.manifest[i]=7;else if(field==2)canonical.manifest[i]=0;else canonical.manifest[i]^=1;
   refuse();
  }
 }
 for(unsigned w=0;w<119;w++){canonical.source[w*16]=0;assert(!PioneerQ38FfnGraphManifest(saved.data(),canonical.source.data()));canonical.source[w*16]=0x16005;refused++;}
 canonical.manifest=saved;canonical.manifest[6]=0x7ffffff8;canonical.manifest[13]=0x7ffffffd;
 for(unsigned w=0;w<119;w++)canonical.manifest[64+w]=0x7ffffff8;
 assert(PioneerQ38FfnGraphManifest(canonical.manifest.data(),canonical.source.data()));positive++;
 auto finished=canonical.manifest;
 for(unsigned w=0;w<119;w++)finished[64+w]=PioneerQ38FfnGraphNextPeer(canonical.manifest.data(),w);
 finished[5]=2;finished[6]=canonical.manifest[13];finished[13]=finished[6]+1;finished[14]=0;
 assert(PioneerQ38FfnGraphManifest(finished.data(),canonical.source.data()));positive++;
 for(unsigned w=0;w<119;w++)assert(PioneerQ38FfnGraphNextPeer(finished.data(),w)==finished[64+w]+1);
 // Shape is explicitly NOT canonical learned/model ownership:these valid
 // data changes require a future private native owner to authenticate/refuse.
 canonical.manifest=saved;canonical.manifest[PioneerQ38FfnGraphRoleOffset(0)+16]+=64;canonical.manifest[32]++;
 assert(PioneerQ38FfnGraphManifest(canonical.manifest.data(),canonical.source.data()));positive++;
 printf("PASS OFFLINE complete FFN manifest DATA:positive%u/refusals%u/all3roles/all119 peers/row orders/mixed Q4_K-Q6_K/STOP headroom/disjoint exact shadow+physical counter storage;NO_CANONICAL_OWNER_OR_NATIVE_CAPABILITY\n",positive,refused);
}
