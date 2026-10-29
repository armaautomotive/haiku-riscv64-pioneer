/* SPDX-License-Identifier: MIT */
// Actual production method,synthetic OS allocation state only. No native
// owner,completion,source,host-release or card capability is constructed.
#define PIONEER_HOST_READ_MOCK_ONLY
#include "pioneer_host_read_probe_test.cpp"
#define PIONEER_OUTPUT_BUFFER_MOCK_ONLY
#include "pioneer_output_buffer_test.cpp"
#include <array>
struct ActualBaseQuarantine:HostReadMock,OutputBufferMock {
 using HostReadMock::quarantined; // production base names the input-page flag,not output ownership
#include "token_native_base_host_quarantine.inc"
};
int main(){
 (void)Input; // shared legacy output helper is not exercised by this base-method test
 for(unsigned state=0;state<4;state++){
  ActualBaseQuarantine io;
  if(state)assert(io.AllocateHostPage()==0);
  if(state>=2){io.releaseFail=state==3;assert(io.ReleaseHostPage()==(state==3?-4:0));}
  const bool allocated=io.PageMock::allocated,cached=io.PageMock::cached;
  const unsigned releases=io.PageMock::releases,maps=io.PageMock::maps,pageWrites=io.PageMock::writes,nocWrites=io.Mock::writes;
  const auto physical=io.PageMock::physical,bytes=io.PageMock::bytes;std::array<uint32_t,TT_HOST_PAGE_WORDS> page;
  for(unsigned i=0;i<page.size();i++)page[i]=io.page[i];
  for(unsigned repeat=0;repeat<2;repeat++){
   io.QuarantineHostPage();
   assert(io.quarantined && !io.OutputBufferMock::quarantined && io.PageMock::allocated==allocated && io.PageMock::cached==cached && io.PageMock::releases==releases
    && io.PageMock::maps==maps && io.PageMock::writes==pageWrites && io.Mock::writes==nocWrites && io.PageMock::physical==physical && io.PageMock::bytes==bytes
    && !io.requests && !io.bmeWrites);
   for(unsigned i=0;i<page.size();i++)assert(io.page[i]==page[i]);
  }
 }
 puts("PASS actual production base quarantine/4 synthetic never-allocated,live,released,release-failed states/repeated metadata-only flagging/NO page,map,release,NoC or BME operation;legacy live-DMA mock unchanged;NOT native lifetime/END/physical/Work/TPS qualification");
}
