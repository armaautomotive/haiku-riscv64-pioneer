/* SPDX-License-Identifier: MIT */
// OFFLINE candidate. No driver operation exposes this image yet.
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
static unsigned CounterOffset(unsigned word)
{
	static const unsigned offsets[]={0x208,0x20c,0x210,0x214,0x204,0x220,0x210,0x228};
	return offsets[word];
}
static unsigned Complete(unsigned workers)
{
	volatile uint32_t* stats=Words(0x143000);
	stats[4]=workers;
	for (unsigned i=0;i<8;i++) stats[16+i]=*Words(0xffb20000+CounterOffset(i));
	Fence(); return workers;
}
static void Transfer(bool write,unsigned worker,uint32_t local,uint32_t remote,unsigned bytes)
{
	if (!worker || worker>=120 || !bytes || bytes>16384 || ((remote^local)&31) || !NocIdle()) coordinator_fault_park();
	volatile uint32_t* noc=Words(0xffb20000);
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
	Fence(); noc[0x40/4]=1; (void)noc[0x40/4]; Fence();
	for (unsigned attempt=0;attempt<65536;attempt++) {
		bool complete=!noc[0x40/4];
		for (unsigned i=0;i<4;i++) complete &= noc[offsets[i]/4]==expected[i];
		if (complete && NocIdle()) {
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
static bool WaitPc(uint32_t pc) { return pc>=0x71280 && pc<=0x71294 && !(pc&3); }
static void ResetHeld(unsigned worker,uint32_t expected)
{
	// Read only! A NoC read has no authority to reset a peer.
	Transfer(false,worker,0x147090,0xffb121b0,4);
	if (*Words(0x147090)!=expected) coordinator_fault_park();
}
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
extern "C" unsigned coordinator_main()
{
	volatile uint32_t* job=Words(0x140000);
	unsigned operation=job[1],limit=job[2],k=job[3],n=job[4];
	uint32_t base=job[5];
	if (job[0]!=0x31445243 || operation>1 || !limit || limit>119) return 0;
	if (!job[6] || job[6]>=0x7fffffff || !NocIdle()) return 0;
	volatile uint32_t* stats=Words(0x143000);
	for (unsigned i=0;i<24;i++) stats[i]=0;
	stats[5]=n; stats[6]=job[6]; stats[7]=operation;
	for (unsigned i=0;i<8;i++) stats[8+i]=*Words(0xffb20000+CounterOffset(i));
	Fence();
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
	// Prove the ENTIRE cohort idle and every monotonic sequence before
	// the first descriptor write or doorbell. No global wrap or reuse.
	for (unsigned worker=1;worker<=workers;worker++) {
		uint32_t sequence=job[64+worker-1];
		if (!sequence || sequence>=0x7fffffff || !PeerIdle(worker,sequence-1)) coordinator_fault_park();
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
	for (unsigned worker=1;worker<=workers;worker++) {
		uint32_t sequence=job[64+worker-1];
		bool idle=false;
		for (unsigned attempt=0;attempt<65536;attempt++) {
			if (PeerIdle(worker,sequence)) { idle=true; break; }
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
		if (n<=8192) {
			// No host per-peer remapping/readback for hidden projections.
			Transfer(false,worker,0x84000+firstRow*4,0x84000,rows*4);
			for (unsigned i=0;i<rows;i++) if ((Words(0x84000)[firstRow+i]&0x7f800000)==0x7f800000) coordinator_fault_park();
		}
		firstRow+=rows;
	}
	Fence(); return firstRow==n?Complete(workers):0;
}
