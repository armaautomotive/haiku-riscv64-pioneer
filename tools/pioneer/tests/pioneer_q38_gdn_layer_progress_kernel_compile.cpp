/* SPDX-License-Identifier: MIT */
// Standalone metadata compile/frame check;NOT a linked driver/native role.
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38GdnLayerProgress.h"
extern "C" bool pioneer_gdn_layer_bind(PioneerQ38GdnLayerProgress& p,const void* who,const PioneerQ38GdnIdentity& id,unsigned phase){
 return p.BindAfterFirstLayer(who,id,phase);
}
extern "C" bool pioneer_gdn_layer_claim(PioneerQ38GdnLayerProgress& p,const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
 return p.ClaimNext(who,id,layer,phase);
}
extern "C" bool pioneer_gdn_layer_pending(const PioneerQ38GdnLayerProgress& p,const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
 return p.OwnsPending(who,id,layer,phase);
}
extern "C" bool pioneer_gdn_layer_publish(PioneerQ38GdnLayerProgress& p,const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
 return p.PublishNext(who,id,layer,phase);
}
extern "C" bool pioneer_gdn_layer_published(const PioneerQ38GdnLayerProgress& p,const void* who,const PioneerQ38GdnIdentity& id,unsigned layer,unsigned phase){
 return p.Published(who,id,layer,phase);
}
extern "C" bool pioneer_gdn_layer_snapshot(const PioneerQ38GdnLayerProgress& p,const void* who,const PioneerQ38GdnIdentity& id,uint64_t& mask,unsigned& next,bool& active){
 return p.Snapshot(who,id,mask,next,active);
}
extern "C" void pioneer_gdn_layer_fault(PioneerQ38GdnLayerProgress& p){p.Fault();}
