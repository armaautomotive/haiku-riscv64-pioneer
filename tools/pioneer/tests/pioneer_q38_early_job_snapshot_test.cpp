/* SPDX-License-Identifier: MIT */
// Host DATA test executes the actual MatrixJob source extracted by the runner.
// It is not an instruction-machine, hardware, performance or Work test.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "PioneerQ38EarlyJobSnapshot.h"
#include "PioneerQ38MixedBlockRootLayout.h"
#include "PioneerQ38PushedWorkerLayout.h"

namespace L = PioneerQ38MixedBlockRootLayout;
namespace M = PioneerQ38PushedWorkerLayout;
static uint32_t memory[0x180000 / 4] = {};
static volatile uint32_t* Words(unsigned address) {
 assert(!(address & 3) && address < sizeof(memory)); return memory + address / 4;
}
#define PIONEER_Q38_MIXED_LIVE_BLOCK_ROOT_IMAGE 1
#include "extracted-matrix-job.inc"

int main() {
 unsigned tested = 0, rejected = 0;
 for (unsigned w = 0; w < 119; ++w) for (unsigned type : {8U, 12U, 14U})
  for (unsigned sequence : {1U, 13U, 0x7ffffffeU}) {
  uint32_t d[64] = {}, b[64] = {}, receipt[64] = {}, before[8] = {};
  b[0] = M::Magic; b[1] = 1; b[2] = 64; b[3] = w+1; b[4] = M::Peer(w+1);
  b[5] = b[16] = 0x16005; b[6] = 8192; b[7] = 64;
  for (unsigned i = 8; i < 16; ++i) b[i] = 0x51000000 + i;
  for (unsigned i = 17; i < 32; ++i) b[i] = 0x62000000 + i;
  for (unsigned i = 40; i <= 54; ++i) b[i] = M::BootWord(i);
  assert(M::BootShape(b));
  d[6] = 31; d[14] = type == 8 ? 5120 : 17408; d[15] = type; d[40] = 7; d[42] = 5;
  for (unsigned i = 0; i < 8; ++i) d[32+i] = b[8+i];
  auto* part = Words(L::PartsShadow+w*32);
  for (unsigned i = 0; i < 8; ++i) part[i] = 0x4000000 + w*0x10000 + i*64;
  part[2] = (w % 16) + 1;
  MatrixJob(d,w,sequence);
  uint32_t saved[64]; for (unsigned i = 0; i < 64; ++i) saved[i] = Words(L::Scratch)[i];
  auto matches = [&] {return PioneerQ38EarlyJobSnapshot::Matches(saved,d,part,w,sequence,
    M::Magic,M::Peer(w+1),L::Tile(type));};
  assert(matches());
  MatrixJob(d,w,sequence); // original Gather after the one doorbell increments the nonce
  for (unsigned i = 0; i < 64; ++i) assert(saved[i] == Words(L::Scratch)[i]);
  receipt[0] = M::ReceiptMagic; receipt[1] = M::ReceiptWords; receipt[2] = b[3]; receipt[3] = sequence;
  receipt[4] = saved[5]; receipt[5] = saved[6]; receipt[6] = b[4]; receipt[7] = M::Root;
  for (unsigned i = 0; i < 32; ++i) receipt[8+i] = saved[i];
  for (unsigned i = 0; i < 16; ++i) receipt[40+i] = b[16+i];
  assert(M::Receipt(receipt,b,saved,before));
  for (unsigned i = 0; i < 64; ++i) {
   saved[i] ^= 1;
   // Live-derived fields are checked before the receipt. Reserved words
   // are instead checked by the unchanged real receipt/job grammar.
   assert(i < 27 ? !matches() : matches());
   assert(!M::Receipt(receipt,b,saved,before));
   saved[i] ^= 1; ++rejected;
  }
  const unsigned dfields[] = {6,14,15,32,33,34,35,36,37,38,39,40,42};
  for (unsigned i : dfields) {
   d[i] ^= 1; assert(!matches()); MatrixJob(d,w,sequence);
   uint32_t rebuilt[64]; for (unsigned j = 0; j < 64; ++j) rebuilt[j] = Words(L::Scratch)[j];
   assert(!M::Receipt(receipt,b,rebuilt,before));
   d[i] ^= 1; ++rejected;
  }
  for (unsigned i : {0U,2U,4U}) {
   part[i] ^= 1; assert(!matches()); MatrixJob(d,w,sequence);
   uint32_t rebuilt[64]; for (unsigned j = 0; j < 64; ++j) rebuilt[j] = Words(L::Scratch)[j];
   assert(!M::Receipt(receipt,b,rebuilt,before));
   part[i] ^= 1; ++rejected;
  }
  assert(!PioneerQ38EarlyJobSnapshot::Matches(saved,d,part,w,sequence+1,M::Magic,M::Peer(w+1),L::Tile(type)));
  ++tested;
 }
 std::printf("PASS actual MatrixJob source: %u worker/type cases, %u injected word changes rejected by live-field check or unchanged receipt; DATA only, NO_DEVICE\n", tested, rejected);
}
