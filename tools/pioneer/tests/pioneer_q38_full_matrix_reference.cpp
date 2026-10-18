/* SPDX-License-Identifier: MIT */
// OFFLINE independent FP64 dot from ORIGINAL row-major GGUF dequantization.
// NOT linked into native executable;never substitutes output/opens a card.
#include "../PioneerQwen38Gguf.h"
#include "../PioneerQwen38Activation.h"
int main(int argc,char** argv)
{
	if (argc!=3 && argc!=4) return 2;
	unsigned corrupt=5120;
	if (argc==4) {
		if (!strcmp(argv[3],"--test-corrupt-first")) corrupt=0;
		else if (!strcmp(argv[3],"--test-corrupt-middle")) corrupt=2560;
		else if (!strcmp(argv[3],"--test-corrupt-last")) corrupt=5119;
		else return 2;
	}
	FILE* file=fopen(argv[1],"rb"); if (!file) return 1;
	PioneerQwen38Gguf model; if (!model.Open(file)) { fclose(file); return 1; }
	const auto* t=model.Find("blk.0.ffn_down.weight");
	if (!t || t->type!=12 || t->inputs!=17408 || t->outputs!=5120) { fclose(file); return 1; }
	unsigned tensor=unsigned(t-model.Tensors().data());
	FILE* exported=fopen(argv[2],"rb"); if (!exported) { fclose(file); return 1; }
	std::vector<float> actual(5120),weight(17408),input(17408),activation(17408);
	bool okay=fread(actual.data(),4,5120,exported)==5120 && fgetc(exported)==EOF && !ferror(exported);
	fclose(exported);
	if (okay && corrupt<5120) actual[corrupt]+=1000000; // offline observer self-test,never changes exported FILE
	for (unsigned k=0;k<17408;k++) input[k]=float(int((k*29+17)%255)-127)*.00390625f;
	for (unsigned b=0;okay && b<17408/256;b++) {
		PioneerQ38ActivationK q; okay=PioneerQ38QuantizeK(input.data()+b*256,256,&q);
		if (okay) for (unsigned k=0;k<256;k++) {
			int value=int((q.words[1+k/4]>>((k%4)*8))&255); if (value>=128) value-=256;
			activation[b*256+k]=float(value)*PioneerAttentionFloat(q.words[0]);
		}
	}
	double maximum=0,relativeBound=0,error2=0,reference2=0,actual2=0,dot=0; unsigned errors=0,worst=0;
	for (unsigned row=0;okay && row<5120;row++) {
		if (!model.ReadRow(tensor,row,weight.data(),17408) || !isfinite(actual[row])) { okay=false; break; }
		double reference=0,absolute=0;
		for (unsigned k=0;k<17408;k++) { double v=double(weight[k])*double(activation[k]); reference+=v; absolute+=fabs(v); }
		double error=fabs(double(actual[row])-reference),bound=1e-6+absolute*2e-6;
		if (error>maximum) { maximum=error; worst=row; }
		if (error/bound>relativeBound) relativeBound=error/bound;
		if (error>bound) errors++;
		error2+=error*error; reference2+=reference*reference; actual2+=double(actual[row])*actual[row]; dot+=reference*actual[row];
	}
	okay=okay && !errors && reference2>0 && actual2>0;
	printf("%s OFFLINE complete learned Q4_K FFN17408x5120 from original GGUF rows vs native exported graph output:errors%u maximum_absolute%.12g worst_row%u maximum_fraction_of_bound%.9g relative_L2%.12g cosine%.12g;NOT native CPU validation/fallback/LLM/token rate\n",
		okay?"PASS":"FAIL",errors,maximum,worst,relativeBound,sqrt(error2/(reference2+1e-300)),dot/sqrt((reference2+1e-300)*(actual2+1e-300)));
	fclose(file); return okay?0:1;
}
