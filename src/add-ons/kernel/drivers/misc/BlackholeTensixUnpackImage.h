/* SPDX-License-Identifier: MIT */
#ifndef BLACKHOLE_TENSIX_UNPACK_IMAGE_H
#define BLACKHOLE_TENSIX_UNPACK_IMAGE_H
#include <stdint.h>

// Independently assembled/decoded fixed L1-unpack fixture, not firmware.
// Code ends at 70404; private headers/payloads occupy 70700..70a1f.
enum {
 TT_UNPACK_CODE_WORDS = 648,
 TT_UNPACK_IMAGE_WORDS = 1 + TT_UNPACK_CODE_WORDS + 256 + 3,
 TT_UNPACK_PARK_PC = 0x703bc, TT_UNPACK_FINAL_PHASE = 0x703a4
};
// Exact integer fixtures only, bounded by +/-256; no kernel floating point.
static inline uint32_t BlackholeFixedIntegerFp32(int value)
{
 if (value == 0) return 0;
 uint32_t sign = value < 0 ? 0x80000000U : 0;
 uint32_t magnitude = value < 0 ? uint32_t(-value) : uint32_t(value);
 unsigned exponent = 0;
 for (uint32_t rest = magnitude; rest > 1; rest >>= 1) exponent++;
 return sign | ((127+exponent)<<23) | ((magnitude<<(23-exponent)) & 0x7fffff);
}
static inline int BlackholeFixedWeight(unsigned row, unsigned column, bool dense)
{
 return dense ? int((row+2*column)%5)-2 : (row == column ? 1 : 0);
}
static inline int BlackholeFixedVector(unsigned fixture, unsigned row, unsigned column)
{
 int value = int((3*row+column)%16)-8;
 return fixture == 0 ? value : -value-1;
}
static inline uint32_t BlackholeNonuniformExpected(unsigned stage, unsigned output)
{
 unsigned fixture = output/128, row = (output%128)/16, column = output%16;
 if (stage == 22) return BlackholeFixedIntegerFp32(BlackholeFixedWeight(row,column,true));
 if (stage == 23) return BlackholeFixedIntegerFp32(BlackholeFixedVector(fixture,0,column));
 int sum = 0;
 for (unsigned k = 0; k < 16; k++)
  sum += BlackholeFixedVector(fixture,row,k)*BlackholeFixedWeight(k,column,stage == 21);
 return BlackholeFixedIntegerFp32(sum);
}
static inline uint32_t BlackholeTensixUnpackImageWord(unsigned index, unsigned stage)
{
 if (index == 0) return 0x0006a06f;
 uint32_t address = 0x70000 + (index-1)*4;
 if ((stage == 17 || stage == 22) && address == 0x70334) return 0x120022b7;
 if ((stage == 18 || stage == 23) && address == 0x70334) return 0x130012b7;
 if ((stage == 17 || stage == 18 || stage == 22 || stage == 23) && address == 0x7037c) return 0x36c002b7;
 if ((stage == 17 || stage == 18 || stage == 22 || stage == 23) && address == 0x70380) return 0x00542023;
 if ((stage == 17 || stage == 18 || stage == 22 || stage == 23) && address == 0x70384) return 0x03c000ef;
 if (stage == 19 && address == 0x70240) return 0x1480006f;
 if (stage == 19 && address == 0x70244) return 0x00000013;
 if (stage >= 20 && stage <= 23) {
  if (address == 0x70270) return 0x1d00006f;
  if (address == 0x70274) return 0x00000013;
  if (address >= 0x70440 && address < 0x70478) {
   const uint32_t copy[] = {0x00070e37,0x500e0e13,0x000a0463,0x100e0e13,
    0x000713b7,0x92038393,0x04000313,0x000e2283,0x0053a023,
    0x004e0e13,0x00438393,0xfff30313,0xfe0316e3,0xe55ff06f};
   return copy[(address-0x70440)/4];
  }
  if (address >= 0x70500 && address < 0x70700) {
   unsigned fixture = (address-0x70500)/256, element = ((address-0x70500)%256)/2;
   uint32_t lo = BlackholeFixedIntegerFp32(BlackholeFixedVector(fixture,element/16,element%16))>>16;
   uint32_t hi = BlackholeFixedIntegerFp32(BlackholeFixedVector(fixture,element/16,element%16+1))>>16;
   return lo | (hi<<16);
  }
  if (address >= 0x70710 && address < 0x70910) {
   unsigned element = (address-0x70710)/2; bool dense = stage == 21 || stage == 22;
   uint32_t lo = BlackholeFixedIntegerFp32(BlackholeFixedWeight(element/16,element%16,dense))>>16;
   uint32_t hi = BlackholeFixedIntegerFp32(BlackholeFixedWeight(element/16,element%16+1,dense))>>16;
   return lo | (hi<<16);
  }
 }
 const uint32_t code[] = {
		0xffe40437,0xffe004b7,0x0fc48493,0xffef0937,
		0x001809b7,0xfc098993,0x00000a13,0xb20002b7,
		0x00542023,0x39c000ef,0x00100393,0x04400e13,
		0x01039e93,0xb20002b7,0x01d2e2b3,0x00542023,
		0x00138393,0xffc396e3,0xb20202b7,0x00128293,
		0x00542023,0xb20302b7,0x00128293,0x00542023,
		0xb20502b7,0x00428293,0x00542023,0x354000ef,
		0x000042b7,0x2b528293,0x00592023,0x00092303,
		0x00037313,0x20aa02b7,0x00592223,0x00492303,
		0x00037313,0x00000293,0x00592423,0x00892303,
		0x00037313,0x00000293,0x00592623,0x00c92303,
		0x00037313,0x00000293,0x00592c23,0x01892303,
		0x00037313,0x00000293,0x00592e23,0x01c92303,
		0x00037313,0x00000293,0x36592823,0x37092303,
		0x00037313,0x00000293,0x04592823,0x05092303,
		0x00037313,0x000102b7,0xfff28293,0x06592023,
		0x06092303,0x00037313,0x10000293,0x06592823,
		0x07092303,0x00037313,0x10000293,0x0c592423,
		0x0c892303,0x00037313,0x001002b7,0x0e592023,
		0x0e092303,0x00037313,0x10000293,0x0e592623,
		0x0ec92303,0x00037313,0x001002b7,0x01528293,
		0x10592023,0x10092303,0x00037313,0x01000293,
		0x10592223,0x10492303,0x00037313,0x00500293,
		0x12592023,0x12092303,0x00037313,0x00300293,
		0x12592223,0x12492303,0x00037313,0x000072b7,
		0x07028293,0x12592823,0x13092303,0x00037313,
		0x004002b7,0x04028293,0x14592823,0x15092303,
		0x00037313,0x001002b7,0x01028293,0x14592c23,
		0x15892303,0x00037313,0x001002b7,0x01528293,
		0x1c592023,0x1c092303,0x00037313,0x00800293,
		0x1c592223,0x1c492303,0x00037313,0x00500293,
		0x1e592023,0x1e092303,0x00037313,0x00300293,
		0x1e592223,0x1e492303,0x00037313,0x000072b7,
		0x09128293,0x1e592823,0x1f092303,0x00037313,
		0x0ff0000f,0x5e2402b7,0xc0028293,0x00542023,
		0x5e4202b7,0xc0028293,0x00542023,0x184000ef,
		0x000703b7,0x70038393,0x0003a023,0x0003a223,
		0x0003a423,0x0003a623,0x000713b7,0x91038393,
		0x0003a023,0x0003a223,0x0003a423,0x0003a623,
		0x3f8042b7,0xf8028293,0x3f804e37,0xf80e0e13,
		0x000a0863,0x400042b7,0x40404e37,0x040e0e13,
		0x000703b7,0x71038393,0x08000313,0x0053a023,
		0x00438393,0xfff30313,0xfe031ae3,0x000713b7,
		0x92038393,0x04000313,0x01c3a023,0x00438393,
		0xfff30313,0xfe031ae3,0xffc3a303,0x00037313,
		0x0ff0000f,0x370002b7,0x00f28293,0x00542023,
		0x101802b7,0x00000013,0x00100393,0x00542023,
		0x00128293,0xfff38393,0xfe039ae3,0x0c4000ef,
		0x516002b7,0x00a28293,0x00542023,0x546002b7,
		0x00f28293,0x00542023,0x420002b7,0x0c128293,
		0x00542023,0x428002b7,0x0c128293,0x00542023,
		0x090000ef,0x26c002b7,0x02028293,0x00542023,
		0x080000ef,0xffbd93b7,0x80038393,0x00180e37,
		0x800e0e13,0x009a1e93,0x01de0e33,0x08000f13,
		0x0003a283,0x005e2023,0x00438393,0x004e0e13,
		0xffff0f13,0xfe0f16e3,0x0ff0000f,0x00000013,
		0x00000013,0x00000013,0x001a0a13,0x0149a223,
		0x0049a283,0x0002f293,0x00200293,0xea5a12e3,
		0x020000ef,0x4d4152b7,0x43128293,0x0059a423,
		0x0089a303,0x00037313,0x0ff0000f,0x0000006f,
		0x0019a023,0x0009a303,0x00037313,0x0ff0000f,
		0x0004a023,0x0004a303,0xfe031ee3,0xa21022b7,
		0x81628293,0x00542023,0x450002b7,0x17e28293,
		0x00542023,0x0004a303,0xfe030ee3,0x0ff0000f,
		0x00008067
 };
 return index <= sizeof(code)/sizeof(code[0]) ? code[index-1] : 0;
}
#endif

