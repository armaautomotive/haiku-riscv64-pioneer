/* SPDX-License-Identifier: MIT */
// Read-only exact GGUF layout/sample/blueprint gate. NO model execution,
// template evaluation,device,driver,transport,allocation or launch access.
// Caller must separately authenticate the WHOLE file with published SHA256.
#include "PioneerQwen38Gguf.h"
#include "PioneerQwen38PackedPlan.h"
#include <fcntl.h>
#include <unistd.h>
int main(int argc,char** argv)
{
	if (argc!=2) { fprintf(stderr,"Usage: %s MODEL; read-only inspection,NOT SHA authentication/inference\n",argv[0]); return 2; }
	int descriptor=open(argv[1],O_RDONLY|O_NOFOLLOW);
	if (descriptor<0) { fprintf(stderr,"STOP:regular non-symlink model required\n"); return 1; }
	FILE* file=fdopen(descriptor,"rb");
	if (!file) { close(descriptor); return 1; }
	PioneerQwen38Gguf source;
	if (!source.Open(file)) { fprintf(stderr,"STOP:exact pinned Qwen3.8 GGUF geometry/metadata required\n"); fclose(file); return 1; }
	unsigned formats[15]={}; uint64_t packed=0,values=0;
	std::vector<float> row(17408);
	for (unsigned t=0;t<source.Tensors().size();t++) {
		const auto& tensor=source.Tensors()[t]; packed+=tensor.bytes; formats[tensor.type]++;
		for (unsigned r:{0U,unsigned(tensor.outputs-1)}) {
			if (!source.ReadRow(t,r,row.data(),unsigned(row.size()))) {
				fprintf(stderr,"STOP:bounded sample read/finite-scale check failed at tensor%u row%u\n",t,r); fclose(file); return 1;
			}
			for (unsigned i=0;i<tensor.inputs;i++) if ((PioneerAttentionBits(row[i])&0x7f800000)==0x7f800000) {
				fprintf(stderr,"STOP:nonfinite reference row\n"); fclose(file); return 1;
			}
			if (tensor.name.size()>=5 && tensor.name.compare(tensor.name.size()-5,5,"ssm_a")==0)
				for (unsigned i=0;i<48;i++) if (!(row[i]<0)) {
					fprintf(stderr,"STOP:GGUF already-converted negative A required\n"); fclose(file); return 1;
				}
			values+=tensor.inputs;
		}
	}
	if (formats[0]!=353 || formats[8]!=288 || formats[12]!=193 || formats[14]!=17
		|| packed!=UINT64_C(18962876416) || values!=8162688 || !source.Stable()) {
		fprintf(stderr,"STOP:exact format/size/sample totals required\n"); fclose(file); return 1;
	}
	PioneerQwen38PackedPlan plan;
	for (unsigned context:{1U,8192U,32768U}) {
		if (!plan.Build(source,context)) { fprintf(stderr,"STOP:bounded OFFLINE blueprint failed\n"); fclose(file); return 1; }
		uint64_t total=0;
		for (unsigned b=0;b<7;b++) total+=plan.Used(b);
		printf("OFFLINE_ONLY_CONTEXT=%u PROPOSED_BYTES=%llu PARTS=%zu;NO_ALLOCATION_OR_ROUTE_AUTHORITY\n",
			context,(unsigned long long)total,plan.Parts().size());
	}
	if (!source.Stable()) { fprintf(stderr,"STOP:model source changed\n"); fclose(file); return 1; }
	printf("READ_ONLY_LAYOUT_SAMPLE_PASS tensors=851 packed_bytes=%llu sampled_finite_values=%llu F32=353 Q8_0=288 Q4_K=193 Q6_K=17;WHOLE_SHA_STILL_REQUIRED;NOT_INFERENCE_OR_P100A_SUPPORT\n",
		(unsigned long long)packed,(unsigned long long)values);
	return fclose(file)?1:0;
}
