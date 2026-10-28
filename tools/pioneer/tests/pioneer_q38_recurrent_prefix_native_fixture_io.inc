/* SPDX-License-Identifier: MIT */
// TEST physical endpoints in the actual native factory fixture. All real
// new native scopes,phases,source gates and journal code remain verbatim.
RecurrentPrefixNativeTransport* recurrentTransport=nullptr;
bool UseRecurrentPrefixIO()const;void ConfigureRecurrentPrefixTransport();
std::array<std::vector<uint32>,4> recurrentAliases;
int recurrentFailMap=-1,recurrentNullMap=-1;unsigned recurrentMapFaultFence=0;
#include "../../../src/add-ons/kernel/drivers/misc/PioneerQ38RecurrentPrefixNativeMapping.inc"
bool MapRuntime(){return runtimeMapped;}
bool MapQ38Panel(){return UseRecurrentPrefixIO()?recurrentTransport->MapPanel():ProjectionZeroIO::MapQ38Panel();}
bool Q38PanelMappingStable(){return UseRecurrentPrefixIO()?recurrentTransport->PanelStable():ProjectionZeroIO::Q38PanelMappingStable();}
uint32* Q38PanelSavedWords(){return UseRecurrentPrefixIO()?recurrentTransport->PanelSaved():ProjectionZeroIO::Q38PanelSavedWords();}
uint32* HostReadBackup(unsigned n){return UseRecurrentPrefixIO()?recurrentTransport->Backup(n):ProjectionZeroIO::HostReadBackup(n);}
uint32 ComputeNocRead(uint32 a){assert(UseRecurrentPrefixIO());return recurrentTransport->Read32(RecurrentPrefixNativeTransport::ComputeNoc,a);}
uint32 HostPciRead(unsigned p,uint16_t a,uint8 n){
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FINAL_RELEASE
 if(q38RecurrentPrefixFinalUnmapped)return p?RootRead(a,n):CardRead(a,n);
#endif
 uint32 value=UseRecurrentPrefixIO()?recurrentTransport->Read32(RecurrentPrefixNativeTransport::Pci,p,a,n):ProjectionZeroIO::HostPciRead(p,a,n);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FRESH_FINAL_TEST
 if(finalAuditReturned){finalQuietReads++;if(finalQuietFailure && a==4)return 0x406;}
#endif
 return value;
}
uint32 Q38PanelRead(uint32 a){return UseRecurrentPrefixIO()?recurrentTransport->Read32(RecurrentPrefixNativeTransport::Panel,a):ProjectionZeroIO::Q38PanelRead(a);}
void Q38PanelWrite(uint32 a,uint32 v){if(UseRecurrentPrefixIO())recurrentTransport->Write32(RecurrentPrefixNativeTransport::SetPanel,a,v);else ProjectionZeroIO::Q38PanelWrite(a,v);}
uint32 RuntimeCodeRead(unsigned i){return UseRecurrentPrefixIO()?recurrentTransport->Read32(RecurrentPrefixNativeTransport::Code,i):ProjectionZeroIO::RuntimeCodeRead(i);}
void RuntimeCodeWrite(unsigned i,uint32 v){if(UseRecurrentPrefixIO())recurrentTransport->Write32(RecurrentPrefixNativeTransport::SetCode,i,v);else ProjectionZeroIO::RuntimeCodeWrite(i,v);}
uint32 RuntimeControlRead(unsigned i){return UseRecurrentPrefixIO()?recurrentTransport->Read32(RecurrentPrefixNativeTransport::Control,i):ProjectionZeroIO::RuntimeControlRead(i);}
uint32 RuntimeStatusRead(unsigned i){return UseRecurrentPrefixIO()?recurrentTransport->Read32(RecurrentPrefixNativeTransport::Status,i):ProjectionZeroIO::RuntimeStatusRead(i);}
void RuntimeStatusWrite(unsigned i,uint32 v){if(UseRecurrentPrefixIO())recurrentTransport->Write32(RecurrentPrefixNativeTransport::SetStatus,i,v);else ProjectionZeroIO::RuntimeStatusWrite(i,v);}
void Pause(){if(UseRecurrentPrefixIO())recurrentTransport->Pause();else ProjectionZeroIO::Pause();}
uint64 Q38CohortNow(){return UseRecurrentPrefixIO()?recurrentTransport->Now():ProjectionZeroIO::Q38CohortNow();}
uint32 ResidentNocRead(uint32 a){
 uint32 value=UseRecurrentPrefixIO()?recurrentTransport->Read32(RecurrentPrefixNativeTransport::Noc,a):ProjectionZeroIO::ResidentNocRead(a);
#ifdef PIONEER_Q38_RECURRENT_PREFIX_FRESH_FINAL_TEST
 if(finalAuditReturned && finalIdleFailure && a==0x178)return value|1;
#endif
 return value;
}
void ResidentNocWrite(uint32 a,uint32 v){if(UseRecurrentPrefixIO())recurrentTransport->Write32(RecurrentPrefixNativeTransport::SetNoc,a,v);else ProjectionZeroIO::ResidentNocWrite(a,v);}
