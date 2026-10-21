/* SPDX-License-Identifier: MIT */
#ifndef PIONEER_Q38_ROOT_COMMAND_REUSE_H
#define PIONEER_Q38_ROOT_COMMAND_REUSE_H
#ifndef PIONEER_Q38_WORK_ROOT_COMMAND_REUSE
#error Separate OFFLINE root command-programming experiment ONLY
#endif
// Register programming ONLY,not a packet/source/address/completion/owner
// capability. Caller independently proves complete NIU idle,private route,
// source extent,all counters and immutable original10-word command.
// Read EVERY actual field EVERY time. Elide a write ONLY if that read
// already equals the required value;otherwise SAME write+exact readback.
// No software cache/retained guess. NOC_CTRL's hardware-mutated reserved
// bits MUST also compare equal:an unequal field is rewritten,not masked.
// Blackhole retains command arguments except reserved NOC_CTRL fields:
// https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/NoC/MemoryMap.md
template<class IO> static bool PioneerQ38ProgramRootCommand(IO& io,const uint32_t (&command)[10],bool idle) {
 if(!idle)return false;
 for(unsigned word=0;word<10;word++){
  uint32_t actual=io.Read(word);
  if(actual==command[word])continue;
  io.Write(word,command[word]);
  if(io.Read(word)!=command[word])return false;
 }
 return true; // NO doorbell/fence/response/cleanup authority
}
#endif
