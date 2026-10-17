/* SPDX-License-Identifier: MIT */
// Qualified scalar subset, used ONLY by offline actual-bytecode tests.
#ifndef PIONEER_SCALAR_INSTRUCTION_H
#define PIONEER_SCALAR_INSTRUCTION_H
static float PioneerScalarFlush(float value)
{
	uint32_t bits=Bits(value); return bits&0x7f800000?value:Float(bits&0x80000000);
}
template<class M> static void PioneerScalarStep(M& m,uint32_t (&fp)[32])
{
	uint32_t i=m.l1[m.Index(m.pc)],op=i&127;
	if (op!=7 && op!=0x27 && op!=0x53) { m.Machine::Step(); return; }
	assert(++m.steps<m.StepLimit());
	unsigned d=(i>>7)&31,f=(i>>12)&7,s=(i>>15)&31,t=(i>>20)&31,hi=i>>25;
	if (op==7) { assert(f==2); fp[d]=m.Load(m.regs[s]+Sign(i>>20,12)); }
	else if (op==0x27) { assert(f==2); m.Store(m.regs[s]+Sign((hi<<5)|d,12),fp[t]); }
	else {
		float a=PioneerScalarFlush(Float(fp[s])),b=PioneerScalarFlush(Float(fp[t]));
		switch (hi) {
			case 0: assert(f==7 || !f); fp[d]=Bits(PioneerScalarFlush(a+b)); break;
			case 4: assert(f==7 || !f); fp[d]=Bits(PioneerScalarFlush(a-b)); break;
			case 8: assert(f==7 || !f); fp[d]=Bits(PioneerScalarFlush(a*b)); break;
			case 0x10: assert(f<=2); fp[d]=f==0?((fp[s]&0x7fffffff)|(fp[t]&0x80000000)):f==1?((fp[s]&0x7fffffff)|((~fp[t])&0x80000000)):(fp[s]^(fp[t]&0x80000000)); break;
			case 0x50: assert(f<=2); if (d) m.regs[d]=f==0?a<=b:f==1?a<b:a==b; break;
			case 0x68: assert(t<=1); fp[d]=Bits(t?float(m.regs[s]):float(int32_t(m.regs[s]))); break;
			case 0x70: assert(!t && !f); if (d) m.regs[d]=fp[s]; break;
			case 0x78: assert(!t && !f); fp[d]=m.regs[s]; break;
			default: fprintf(stderr,"UNQUALIFIED scalar FP instruction%08x PC%08x\n",i,m.pc); assert(false);
		}
	}
	m.regs[0]=0; m.pc+=4;
}
#endif
