/* SPDX-License-Identifier: MIT */
#define PIONEER_Q38_MODE_SESSION_MOCK_ONLY
#include "pioneer_q38_mode_session_test.cpp"
#include <tuple>
struct SpanSource:FakeSource {
	unsigned reads=0,failRead=0;
	explicit SpanSource(FILE* f):FakeSource(f) {}
	static uint32_t Pattern(unsigned part,unsigned page,unsigned word) { return 0x7fc00000U+(part*73+page*1024+word)*0x9e3779b9U; }
	bool ReadWeightPage(const PioneerQwen38PackedPlan&,unsigned f,unsigned page,uint8_t* data,unsigned bytes) {
		if (!Ready() || ++reads==failRead) return false;
		assert(bytes==4096); for (unsigned w=0;w<1024;w++) { uint32_t p=Pattern(f,page,w); memcpy(data+w*4,&p,4); }
		return Ready();
	}
};
struct SpanIO:WorkSessionIO {
	SpanSource& spanSource;
	unsigned spanFault=0,spans=0,spanEnds=0;
	bool spanClosed=false;
	std::vector<std::tuple<unsigned,uint64_t,uint64_t>> ranges;
	SpanIO(SpanSource& source,unsigned tensor):WorkSessionIO(source,tensor),spanSource(source) {}
	bool Call(int fd,uint32_t op,void* data,size_t bytes) {
		assert(fd==73 && !stopped);
		assert(op!=TT_PCI_PROBE_Q38_BANK_BULK_UPLOAD && op!=TT_PCI_PROBE_Q38_BANK_UPLOAD); // NO fallback
		if (op==
#ifdef PIONEER_Q38_SPAN_FAST
			TT_PCI_PROBE_Q38_SPAN_WORK_SUPPORT
#else
			TT_PCI_PROBE_Q38_SPAN_SUPPORT
#endif
			) {
			if (spanFault==1) return false;
			*static_cast<tt_q38_span_support*>(data)={55,1,7,4096,1048576,1,1,0}; return true;
		}
		if (op==
#ifdef PIONEER_Q38_SPAN_FAST
			TT_PCI_PROBE_Q38_SPAN_WORK_UPLOAD
#else
			TT_PCI_PROBE_Q38_SPAN_UPLOAD
#endif
			) {
			assert(bytes==sizeof(tt_q38_span_upload) && begins==1 && !runs && !spanClosed);
			auto& q=*static_cast<tt_q38_span_upload*>(data);
			assert(PioneerQ38SpanPublicShape(q.version,q.bank,q.offset,q.bytes,q.reserved));
			bool found=false;
			for (unsigned f=0;f<layout.Parts().size();f++) {
				const auto& p=layout.Parts()[f]; uint64_t off=uint64_t(p.proposedAddress)-layout.Base;
				if (p.kind!=PIONEER_Q38_WEIGHT || p.bank!=q.bank || q.offset<off || q.offset-off>=p.allocated) continue;
				assert(p.tensor==tensor && q.bytes<=p.allocated-(q.offset-off));
				unsigned first=unsigned((q.offset-off)/4096);
				for (unsigned page=0;page<q.bytes/4096;page++) for (unsigned w=0;w<1024;w++)
					assert(q.data[page*1024+w]==SpanSource::Pattern(f,first+page,w));
				found=true; break;
			}
			assert(found);
			for (auto [bank,lo,hi]:ranges) assert(bank!=q.bank || q.offset>=hi || q.offset+q.bytes<=lo);
			ranges.emplace_back(q.bank,q.offset,q.offset+q.bytes); spans++;
			q.result={1,TT_RUNTIME_PASSED,q.bytes,q.bytes/4096,spans,1,1,1,1,q.bytes/4,0,0};
#ifdef PIONEER_Q38_SPAN_FAST
			q.result.host_exact=q.result.matched_words=0;
#endif
			if (spanFault==2) { stopped=true; return false; }
			if (spanFault==3) { q.result.pages--; stopped=true; }
			if (spanFault==4) { q.result.nonce--; stopped=true; }
			if (spanFault==5) { q.result.host_exact^=1; stopped=true; }
			if (spanFault==6) { spanSource.valid=false; stopped=true; }
			if (spanFault==9) { q.result.reserved=1; stopped=true; }
			return true;
		}
		if (op==TT_PCI_PROBE_Q38_SPAN_END) {
			assert(!runs && spans && !spanClosed && bytes==sizeof(tt_q38_span_end)); spanEnds++;
			auto& e=*static_cast<tt_q38_span_end*>(data); e={1,0,1,0};
			if (spanFault==7) { stopped=true; return false; }
			if (spanFault==8) { e.closed=0; stopped=true; return true; }
			spanClosed=true; return true;
		}
		if (op==TT_PCI_PROBE_Q38_WORK_RUN) assert(spanClosed && spans);
		return WorkSessionIO::Call(fd,op,data,bytes);
	}
};
int main()
{
	(void)&Check; (void)&CohortHostTestMain;
	Fixture fixture; SpanSource source(fixture.file);
	const auto* t=source.Model().Find("blk.3.attn_k.weight"); assert(t); unsigned tensor=unsigned(t-source.Model().Tensors().data());
	std::vector<float> x(5120,.125f),y(1024,-99),before=y;
	{
		SpanIO io(source,tensor); PioneerQ38WorkSession<SpanIO,SpanSource> session(io,source);
		assert(session.Open() && session.PrimeSpan(tensor) && !io.runs && io.spans>1 && io.spanEnds==1 && !io.uploads);
		assert(session.Multiply(tensor,x.data(),5120,y.data(),1024,1)); unsigned spans=io.spans;
		assert(session.Multiply(tensor,x.data(),5120,y.data(),1024,1) && io.spans==spans && !io.uploads);
		assert(session.Finish() && io.ends==1 && io.closes==1);
	}
	for (unsigned fault=1;fault<=9;fault++) {
		source.valid=true; SpanIO io(source,tensor); io.spanFault=fault;
		PioneerQ38WorkSession<SpanIO,SpanSource> session(io,source); y=before;
		bool opened=session.Open(); assert(opened==(fault!=1));
		if (opened) assert(!session.PrimeSpan(tensor));
		assert(session.Failed() && io.closes==1 && !io.ends && !io.runs && !io.uploads && y==before);
		unsigned spans=io.spans,ends=io.spanEnds;
		assert(!session.Multiply(tensor,x.data(),5120,y.data(),1024,1) && !session.Finish() && !session.PrimeSpan(tensor));
		assert(spans==io.spans && ends==io.spanEnds);
	}
	source.valid=true;
	{
		SpanIO io(source,tensor); PioneerQ38WorkSession<SpanIO,SpanSource> session(io,source);
		assert(session.Open() && !session.Multiply(tensor,x.data(),5120,y.data(),1024,1));
		assert(session.Failed() && !io.spans && !io.runs && !io.ends && io.closes==1);
	}
	{
		SpanIO io(source,tensor); PioneerQ38WorkSession<SpanIO,SpanSource> session(io,source);
		assert(session.Open() && session.PrimeSpan(tensor) && !session.PrimeSpan(tensor));
		assert(session.Failed() && !io.runs && !io.ends && io.closes==1);
	}
	source.valid=true; source.failRead=source.reads+2;
	{
		SpanIO io(source,tensor); PioneerQ38WorkSession<SpanIO,SpanSource> session(io,source);
		assert(session.Open() && !session.PrimeSpan(tensor) && io.closes==1 && !io.spans && !io.spanEnds && !io.ends && !io.runs);
	}
	puts("PASS SPAN WORK SESSION: source before FD, support before BEGIN, exact packed page spans before ANY compute, explicit code-owner handoff, repeated resident matrices, NO legacy fallback, invalid/read/reply/source/END failures CLOSE ONLY without another device operation; NO DEVICE IO");
}
