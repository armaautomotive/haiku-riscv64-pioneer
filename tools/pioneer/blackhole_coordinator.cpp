/* SPDX-License-Identifier: MIT */
// Fixed immutable image, exposed only through guarded private coordinator ABIs.
// Master0 coordinates fixed peers1..119; all addresses/routes are fixed or
// derived from a shape/arena checked twice (kernel and immutable program).
// Peer math is unchanged. No PCI/host DMA, arbitrary code or CSR writes.
typedef __UINT32_TYPE__ uint32_t;
static volatile uint32_t* Words(uint32_t a) { return reinterpret_cast<volatile uint32_t*>(a); }
static void Fence() { asm volatile("fence" ::: "memory"); }
extern "C" void coordinator_fault_park() __attribute__((noreturn));
static unsigned X(unsigned worker)
{
	static const unsigned columns[]={1,2,3,4,5,6,7,10,11,12,13,14};
	unsigned column=0; while (worker>=10) { worker-=10; column++; }
	if (column>=12) coordinator_fault_park();
	return columns[column];
}
static unsigned XY(unsigned worker)
{
	unsigned row=worker; while (row>=10) row-=10;
	return X(worker)|((2+row)<<6);
}
static bool NocIdle()
{
	volatile uint32_t* noc=Words(0xffb20000);
	bool idle=true;
	for (unsigned i=0;i<4;i++) idle &= noc[(i*0x800+0x40)/4]==0;
	for (unsigned i=0;i<32;i++) idle &= !(noc[(0x240+i*4)/4]&255);
	return idle;
}
#if PIONEER_COORDINATOR_PACKET_IDLE
// Separate fast image. Kernel entry/exit prove ALL initiators/IDs idle;
// other RISCs/NoC initiators on master0 are held and no code uses them.
// This immutable program uses only initiator0/transaction-ID0, serially.
// Exact command readback/counters/ID0/fences remain per packet. A full
// independent ALL-ID proof remains mandatory before final publication.
static bool PacketIdle()
{
	volatile uint32_t* noc=Words(0xffb20000);
	return !noc[0x40/4] && !(noc[0x240/4]&255);
}
#else
static bool PacketIdle() { return NocIdle(); }
#endif
static unsigned CounterOffset(unsigned word)
{
	static const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
	return offsets[word];
}
static unsigned Complete(unsigned workers)
{
#if PIONEER_COORDINATOR_PACKET_IDLE
	if (!NocIdle()) coordinator_fault_park();
#endif
	volatile uint32_t* stats=Words(0x143000);
	stats[4]=workers;
	for (unsigned i=0;i<8;i++) stats[16+i]=*Words(0xffb20000+CounterOffset(i));
	Fence(); return workers;
}
static void Transfer(bool write,unsigned worker,uint32_t local,uint32_t remote,unsigned bytes)
{
	if (!worker || worker>=120 || !bytes || bytes>16384 || ((remote^local)&31) || !PacketIdle()) coordinator_fault_park();
	volatile uint32_t* noc=Words(0xffb20000);
#if PIONEER_COORDINATOR_UNROLL_TRANSFER
	// Same command,ALL ten readbacks,four exact counter deltas and idle
	// proof. Avoid the volatile stack command and indexed setup loops;
	// this changes NO transfer authority,ordering or acceptance condition.
	unsigned c0=write?local:remote,c2=write?129U:XY(worker),c3=write?remote:local,c5=write?XY(worker):129U,c7=write?0x212U:0;
	unsigned o0=write?0x204U:0x208U,o1=write?0x220U:0x20cU,o3=write?0x228U:0x214U;
	uint32_t e0=noc[o0/4]+1,e1=noc[o1/4]+((bytes+63)>>6),e2=noc[0x210/4]+1,e3=noc[o3/4]+1;
	noc[0]=c0; noc[1]=0; noc[2]=c2; noc[3]=c3; noc[4]=0; noc[5]=c5; noc[6]=0; noc[7]=c7; noc[8]=bytes; noc[9]=0;
	uint32_t mismatch=(noc[0]^c0)|noc[1]|(noc[2]^c2)|(noc[3]^c3)|noc[4]|(noc[5]^c5)|noc[6]|(noc[7]^c7)|(noc[8]^bytes)|noc[9];
	if (mismatch) coordinator_fault_park();
#else
	const unsigned offsets[4]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
	uint32_t expected[4];
	for (unsigned i=0;i<4;i++) expected[i]=noc[offsets[i]/4]+(i==1?(bytes+63)/64:1);
	volatile uint32_t command[10];
	for (unsigned i=0;i<10;i++) command[i]=0;
	command[0]=write?local:remote; command[2]=write?129U:XY(worker);
	command[3]=write?remote:local; command[5]=write?XY(worker):129U;
	command[7]=write?0x212U:0; command[8]=bytes;
	for (unsigned i=0;i<10;i++) noc[i]=command[i];
	for (unsigned i=0;i<10;i++) if (noc[i]!=command[i]) coordinator_fault_park();
#endif
	Fence(); noc[0x40/4]=1; (void)noc[0x40/4]; Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool complete=!noc[0x40/4];
#if PIONEER_COORDINATOR_UNROLL_TRANSFER
		complete &= noc[o0/4]==e0; complete &= noc[o1/4]==e1; complete &= noc[0x210/4]==e2; complete &= noc[o3/4]==e3;
#else
		for (unsigned i=0;i<4;i++) complete &= noc[offsets[i]/4]==expected[i];
#endif
		if (complete && PacketIdle()) {
			volatile uint32_t* stats=Words(0x143000);
			stats[write?2:0]++; stats[write?3:1]+=(bytes+63)/64;
			Fence(); return;
		}
	}
	coordinator_fault_park(); // unknown completion: no retry or cleanup/reset
}
static uint32_t Pc(unsigned worker)
{
	Transfer(false,worker,0x147040,0xffb13140,4);
	return *Words(0x147040);
}
#if PIONEER_COORDINATOR_FUSED_OUTPUT
// Fixed kernel-owned16KiB host owner only. This separate immutable image
// has no arbitrary code, coordinates or host address from a userspace ABI.
// Exact kernel staging + ownership bounds precede BME enable. Only this
// bounded final write/read pair may reach PCIe; peers remain device-only.
static void HostTransfer(bool write,uint32_t low,uint32_t high,unsigned words)
{
#if PIONEER_COORDINATOR_WIDE_OUTPUT
	// Only the new fixed wide-owner image permits128B-aligned subspans.
	// Its parent validates the ENTIRE private1MiB owner before any launch.
	// A transfer remains<=16KiB,within48bits,fully ordered and compared.
	uint32_t last=low+words*4-1;
	if (!words || words>4096 || (words&31) || !high || high>0xffff || (low&31)
		|| (high==0xffff && last<low) || !PacketIdle()) coordinator_fault_park();
#else
	if (!words || words>4096 || (words&31) || !high || high>0xffff || (low&4095)
		|| (high==0xffff && low>0xffffc000) || !PacketIdle()) coordinator_fault_park();
#endif
	volatile uint32_t* noc=Words(0xffb20000);
	const unsigned offsets[4]={write?0x204U:0x208U,write?0x220U:0x20cU,0x210,write?0x228U:0x214U};
	uint32_t expected[4];
	for (unsigned i=0;i<4;i++) expected[i]=noc[offsets[i]/4]+(i==1?words/16:1);
	volatile uint32_t command[10]; for (unsigned i=0;i<10;i++) command[i]=0;
	command[0]=write?0x84000:low; command[1]=write?0:high; command[2]=write?129U:1555U;
	command[3]=write?low:0x144000; command[4]=write?high:0; command[5]=write?1555U:129U;
	command[7]=write?0x212U:0; command[8]=words*4;
	for (unsigned i=0;i<10;i++) noc[i]=command[i];
	for (unsigned i=0;i<10;i++) if (noc[i]!=command[i]) coordinator_fault_park();
	Fence(); noc[0x40/4]=1; (void)noc[0x40/4]; Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool complete=!noc[0x40/4];
		for (unsigned i=0;i<4;i++) complete &= noc[offsets[i]/4]==expected[i];
		if (complete && PacketIdle()) {
			Words(0x143000)[write?2:0]++; Words(0x143000)[write?3:1]+=words/16;
			Fence(); return;
		}
	}
	coordinator_fault_park();
}
#endif
#if !PIONEER_COORDINATOR_TRUSTED_IDLE
static bool WaitPc(uint32_t pc) { return pc>=0x71280 && pc<=0x71294 && !(pc&3); }
#endif
static void ResetHeld(unsigned worker,uint32_t expected)
{
	// Read only! A NoC read has no authority to reset a peer.
	Transfer(false,worker,0x147090,0xffb121b0,4);
	if (*Words(0x147090)!=expected) coordinator_fault_park();
}
#if !PIONEER_COORDINATOR_TRUSTED_IDLE
static bool PeerIdle(unsigned worker,uint32_t sequence)
{
	Transfer(false,worker,0x147000,0x71300,16);
	volatile uint32_t* mailbox=Words(0x147000);
	if (mailbox[0]!=sequence || mailbox[2] || mailbox[3]!=0x43524431) coordinator_fault_park();
	if (mailbox[1]!=sequence) return false;
	uint32_t first=Pc(worker),second=Pc(worker);
	if (!WaitPc(first) || !WaitPc(second)) return false;
	ResetHeld(worker,0x46800);
	return true;
}
#else
static bool PeerReleased(unsigned worker,uint32_t sequence)
{
	// The fixed worker publishes finished ONLY after its full source,
	// NoC-idle,counter/status proof. It cannot issue another NoC operation
	// until OUR next doorbell. Kernel boot/previous-job ownership proves
	// the initial idle state; END still live-audits every STOP before reset.
	Transfer(false,worker,0x147000,0x71300,16);
	volatile uint32_t* mailbox=Words(0x147000);
	if (mailbox[2] || mailbox[3]!=0x43524431) coordinator_fault_park();
	return mailbox[0]==sequence && mailbox[1]==sequence;
}
#endif
extern "C" unsigned coordinator_main()
{
	volatile uint32_t* job=Words(0x140000);
	unsigned operation=job[1],limit=job[2],k=job[3],n=job[4];
	uint32_t base=job[5];
#if PIONEER_COORDINATOR_FUSED_OUTPUT
#if PIONEER_COORDINATOR_WIDE_OUTPUT
	if (job[0]!=0x31445243 || (operation!=0 && operation!=1 && operation!=3 && operation!=6
#if PIONEER_COORDINATOR_GREEDY
		&& operation!=9
#endif
		) || !limit || limit>119) return 0;
	if ((operation==6
#if PIONEER_COORDINATOR_GREEDY
		|| operation==9
#endif
		) && (!n || n>262144 || !job[8] || job[8]>0xffff || (job[7]&0xfffff)
		|| (job[8]==0xffff && job[7]>0xfff00000) || job[9]!=0x100000)) return 0;
#if PIONEER_COORDINATOR_GREEDY
	if (operation==9 && (n!=151936 || k!=1024 || limit!=119 || !job[10] || job[10]>256)) return 0;
#endif
#else
	if (job[0]!=0x31445243 || (operation!=0 && operation!=1 && operation!=3) || !limit || limit>119) return 0;
#endif
	if (operation==3 && (!n || n>4096 || !job[8] || job[8]>0xffff || (job[7]&4095)
		|| (job[8]==0xffff && job[7]>0xffffc000) || job[9]!=16384)) return 0;
	if (operation!=3 && (job[7] || job[8] || job[9])
#if PIONEER_COORDINATOR_WIDE_OUTPUT
		&& operation!=6
#if PIONEER_COORDINATOR_GREEDY
		&& operation!=9
#endif
#endif
	) return 0;
#else
	if (job[0]!=0x31445243 || operation>(PIONEER_COORDINATOR_OUTPUT_COMPARE?2U:1U) || !limit || limit>119) return 0;
#endif
	if (!job[6] || job[6]>=0x7fffffff || !NocIdle()) return 0;
	volatile uint32_t* stats=Words(0x143000);
	for (unsigned i=0;i<24;i++) stats[i]=0;
	stats[5]=n; stats[6]=job[6]; stats[7]=operation;
	for (unsigned i=0;i<8;i++) stats[8+i]=*Words(0xffb20000+CounterOffset(i));
	Fence();
#if PIONEER_COORDINATOR_OUTPUT_COMPARE
	if (operation==2) {
		// No NoC/host DMA here. Kernel must FIRST establish fresh ordered
		// host readback into its owned144000..148000 scratch and turn BME
		// OFF. Compare every original/readback bit, never a hash or sum.
		if (limit!=1 || (k!=0 && k!=4096) || !n || n>4096 || (n&31) || base) return 0;
		uint32_t mismatch=0;
		for (unsigned i=0;i<n;i++) {
			uint32_t original=Words(0x84000)[k+i],returned=Words(0x144000)[i];
			mismatch |= original^returned;
			mismatch |= (original&0x7f800000)==0x7f800000;
		}
		stats[0]=mismatch; stats[1]=k; Fence(); return Complete(n);
	}
#endif
	if (!operation) {
		// Read-only image qualification. CPU preflight records each held PC.
		for (unsigned worker=1;worker<=limit;worker++) {
			ResetHeld(worker,0x47800);
			uint32_t first=Pc(worker),second=Pc(worker);
			ResetHeld(worker,0x47800);
			if (first!=job[64+worker-1] || second!=first) coordinator_fault_park();
			Words(0x141000)[worker-1]=first;
		}
		Fence(); return Complete(limit);
	}
	if ((k!=1024 && k!=2048 && k!=3072 && k!=5120 && k!=6144) || !n || n>262144 || (n&31)
		|| (base&4095) || base<0x04000000 || base>=0x82400000) return 0;
	unsigned rowBytes=k+(k>>3),bytes=n*rowBytes;
	if (bytes>0x82400000-base) return 0;
	unsigned blocks=n>>5,workers=blocks<limit?blocks:limit,blocksPerWorker=0,extra=blocks;
	while (extra>=workers) { extra-=workers; blocksPerWorker++; }
	if (blocksPerWorker+(extra!=0)>128) return 0; // peer output lease <=4096 rows
#if PIONEER_COORDINATOR_COMPACT_INPUT
	// One compact owned host READ contains exactly the vector followed by
	// its scales. Keep the qualified peer ABI unchanged: relocate/verify
	// scales into83000 locally, without PCIe padding or another host call.
	// Source and destination never overlap except the identical6144 case.
	for (unsigned i=0;i<(k>>5);i++) Words(0x83000)[i]=Words(0x80000+k*2)[i];
	Fence();
	for (unsigned i=0;i<(k>>5);i++)
		if (Words(0x83000)[i]!=Words(0x80000+k*2)[i]) coordinator_fault_park();
#endif
	// Prove the ENTIRE cohort idle and every monotonic sequence before
	// the first descriptor write or doorbell. No global wrap or reuse.
	for (unsigned worker=1;worker<=workers;worker++) {
		uint32_t sequence=job[64+worker-1];
#if PIONEER_COORDINATOR_TRUSTED_IDLE
		if (!sequence || sequence>=0x7fffffff) coordinator_fault_park();
#else
		if (!sequence || sequence>=0x7fffffff || !PeerIdle(worker,sequence-1)) coordinator_fault_park();
#endif
	}
	unsigned firstRow=0;
	for (unsigned worker=1;worker<=workers;worker++) {
		unsigned rows=(blocksPerWorker+((worker-1)<extra))*32;
		volatile uint32_t* descriptor=Words(0x147100);
		for (unsigned i=0;i<8;i++) descriptor[i]=0;
		descriptor[0]=base+firstRow*rowBytes; descriptor[1]=k>>4; descriptor[2]=rows>>4; descriptor[3]=1; descriptor[4]=XY(worker);
		Fence();
		Transfer(true,worker,0x147100,0x70500,32);
		Transfer(false,worker,0x147120,0x70500,32);
		for (unsigned i=0;i<8;i++) if (Words(0x147120)[i]!=descriptor[i]) coordinator_fault_park();
		*Words(0x147140)=job[64+worker-1]; Fence();
		Transfer(true,worker,0x147140,0x71300,4);
		firstRow+=rows;
	}
	if (firstRow!=n) coordinator_fault_park();
	firstRow=0;
#if PIONEER_COORDINATOR_GREEDY
	unsigned best=0,bestBits=0,bestKey=0;
#endif
	for (unsigned worker=1;worker<=workers;worker++) {
		uint32_t sequence=job[64+worker-1];
		bool idle=false;
		for (unsigned attempt=0;attempt<65536;attempt++) {
#if PIONEER_COORDINATOR_TRUSTED_IDLE
			if (PeerReleased(worker,sequence)) { idle=true; break; }
#else
			if (PeerIdle(worker,sequence)) { idle=true; break; }
#endif
			Transfer(false,worker,0x147088,0x17ffc8,4);
			if (*Words(0x147088)==0x4e4f4346) coordinator_fault_park();
		}
		if (!idle) coordinator_fault_park();
		unsigned rows=(blocksPerWorker+((worker-1)<extra))*32;
		Transfer(false,worker,0x147180,0x71340,64);
		volatile uint32_t* proof=Words(0x147180);
		unsigned tiles=(k>>4)*(rows>>4),reads=tiles>>5;
		if (proof[8]!=0x703ec || proof[9]!=tiles || proof[10]!=0x4d415431 || proof[11]!=sequence) coordinator_fault_park();
		for (unsigned i=0;i<4;i++) if (uint32_t(proof[4+i]-proof[i])!=(i==1?reads*144+256:reads+1)) coordinator_fault_park();
		// Keep the full peer proof device-resident for debug retrieval.
		for (unsigned i=0;i<16;i++) Words(0x141000)[(worker-1)*16+i]=proof[i];
		if (n<=8192
#if PIONEER_COORDINATOR_WIDE_OUTPUT
			|| operation==6
#if PIONEER_COORDINATOR_GREEDY
			|| operation==9
#endif
#endif
		) {
			// No host per-peer remapping/readback for hidden projections.
			unsigned outputRow=firstRow;
#if PIONEER_COORDINATOR_WIDE_OUTPUT
			if (operation==6
#if PIONEER_COORDINATOR_GREEDY
				|| operation==9
#endif
				) outputRow=0;
#endif
			Transfer(false,worker,0x84000+outputRow*4,0x84000,rows*4);
			for (unsigned i=0;i<rows;i++) if ((Words(0x84000)[outputRow+i]&0x7f800000)==0x7f800000) coordinator_fault_park();
#if PIONEER_COORDINATOR_GREEDY
			if (operation==9) for (unsigned i=0;i<rows;i++) {
				uint32_t bits=Words(0x84000)[i],key=PioneerDecoderLogitKey(bits);
				if (!firstRow && !i) { best=0; bestBits=bits; bestKey=key; }
				else if (key>bestKey) { best=firstRow+i; bestBits=bits; bestKey=key; }
			}
#endif
#if PIONEER_COORDINATOR_WIDE_OUTPUT
			if (operation==6) {
				uint32_t low=job[7]+firstRow*4,high=job[8]+(low<job[7]);
				HostTransfer(true,low,high,rows); HostTransfer(false,low,high,rows);
				uint32_t mismatch=0; for (unsigned i=0;i<rows;i++) mismatch|=Words(0x84000)[i]^Words(0x144000)[i];
				if (mismatch) coordinator_fault_park();
			}
#endif
		}
		firstRow+=rows;
	}
#if PIONEER_COORDINATOR_GREEDY
	if (operation==9) {
		if (firstRow!=151936) coordinator_fault_park();
		for (unsigned i=0;i<32;i++) Words(0x84000)[i]=0;
		Words(0x84000)[0]=TT_DECODER_GREEDY_MAGIC; Words(0x84000)[1]=job[6]; Words(0x84000)[2]=151936;
		Words(0x84000)[3]=best; Words(0x84000)[4]=bestBits; Words(0x84000)[5]=job[10];
		Words(0x84000)[6]=119; Words(0x84000)[7]=9; Fence();
		HostTransfer(true,job[7],job[8],32); HostTransfer(false,job[7],job[8],32);
		uint32_t mismatch=0; for (unsigned i=0;i<32;i++) mismatch|=Words(0x84000)[i]^Words(0x144000)[i];
		if (mismatch) coordinator_fault_park();
	}
#endif
#if PIONEER_COORDINATOR_FUSED_OUTPUT
	if (operation==3) {
		HostTransfer(true,job[7],job[8],n);
		HostTransfer(false,job[7],job[8],n); // ordered fresh read, never an ACK-only proof
		uint32_t mismatch=0;
		for (unsigned i=0;i<n;i++) mismatch |= Words(0x84000)[i]^Words(0x144000)[i];
		if (mismatch) coordinator_fault_park(); // ALL bits, not XOR/hash across words
	}
#endif
	Fence(); return firstRow==n?Complete(workers):0;
}
