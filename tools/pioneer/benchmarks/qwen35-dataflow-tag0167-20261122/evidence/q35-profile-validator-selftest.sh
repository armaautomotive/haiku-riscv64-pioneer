#!/bin/bash
set -euo pipefail
umask 077
mkdir profile-validator-selftest
cd profile-validator-selftest
trap 'rm -f base base.stderr work.stdout work.stderr work.exit; cd ..; rmdir profile-validator-selftest' EXIT
valid_profile_output() {
	[ "$(cat work.exit)" = 0 ] &&
	grep -Fxq 'P100A_Q35_DATAFLOW unload_clean=1 state=2 active=0 cold=0 service_closed=1' work.stdout &&
	grep -Fxq 'P100A_Q35_DATAFLOW diagnostic_one_run=1' work.stdout &&
	awk '
		$1=="P100A_Q35_DATAFLOW" && $2~/^warm_run=/ {
			delete value
			for (i=2;i<=NF;i++) {split($i,p,"="); value[p[1]]=p[2]}
			if (value["warm_run"]==1 && value["prompt_tokens"]==5 && value["output_tokens"]==2 && value["prompt_seconds"]+0>0 && value["decode_seconds"]+0>0 && value["decode_tokens_per_second"]+0>0) valid++; else invalid++
		}
		$1=="P100A_Q35_DATAFLOW" && $2=="generated_ids=11751,13" {ids++}
		$1=="P100A_Q35_DATAFLOW" && $2~/^generated_ids=/ && $2!="generated_ids=11751,13" {invalid++}
		$0=="P100A_Q35_DATAFLOW output_begin run=1" {begin++}
		$0=="P100A_Q35_DATAFLOW output_end run=1" {end++}
		$0==" Paris." {text++}
		END {exit !(valid==1 && ids==1 && begin==1 && end==1 && text==1 && !invalid)}
	' work.stdout &&
	awk '
		BEGIN {split("embedding residual rms projection convolution delta silu dense_attention argmax fetch authorization sync_and_other total",names," "); for (i=1;i<=13;i++) expected[names[i]]=1}
		$1=="P100A_Q35_DATAFLOW" && $2=="graph_profile" {
			split($3,b,"="); split($4,c,"=");
			if (b[1]!="bin" || c[1]!="cycles" || !(b[2] in expected) || c[2]!~/^[0-9]+$/ || seen[b[2]]++) {invalid++; next}
			cycles[b[2]]=c[2]+0; count++
		}
		END {for (name in expected) if (name!="total") sum+=cycles[name]; exit !(count==13 && cycles["total"]>0 && sum==cycles["total"] && !invalid)}
	' work.stderr
}

cat > base <<'DATA'
P100A_Q35_DATAFLOW warm_run=1 prompt_tokens=5 prompt_seconds=1.0 output_tokens=2 decode_seconds=2.0 decode_tokens_per_second=0.5
P100A_Q35_DATAFLOW generated_ids=11751,13
P100A_Q35_DATAFLOW output_begin run=1
 Paris.
P100A_Q35_DATAFLOW output_end run=1
P100A_Q35_DATAFLOW diagnostic_one_run=1
P100A_Q35_DATAFLOW unload_clean=1 state=2 active=0 cold=0 service_closed=1
DATA
for name in embedding residual rms projection convolution delta silu dense_attention argmax fetch authorization sync_and_other; do
 printf 'P100A_Q35_DATAFLOW graph_profile bin=%s cycles=1\n' "$name"
done > base.stderr
printf 'P100A_Q35_DATAFLOW graph_profile bin=total cycles=12\n' >> base.stderr
printf '0\n' > work.exit
cp base work.stdout
cp base.stderr work.stderr
valid_profile_output || exit 1
sed 's/generated_ids=11751,13/generated_ids=0,0/g' base > work.stdout
if valid_profile_output; then exit 1; fi
cp base work.stdout
sed 's/bin=total cycles=12/bin=total cycles=13/' base.stderr > work.stderr
if valid_profile_output; then exit 1; fi
sed '/bin=projection /d' base.stderr > work.stderr
if valid_profile_output; then exit 1; fi
cp base.stderr work.stderr
cat base.stderr >> work.stderr
if valid_profile_output; then exit 1; fi
printf '%s\n' PROFILE_VALIDATOR_SELFTEST_OK
