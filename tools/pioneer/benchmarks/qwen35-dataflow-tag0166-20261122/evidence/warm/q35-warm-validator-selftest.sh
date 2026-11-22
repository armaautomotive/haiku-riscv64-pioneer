#!/bin/bash
set -euo pipefail
umask 077
mkdir validator-selftest
cd validator-selftest
trap 'rm -f base work.stdout work.exit; cd ..; rmdir validator-selftest' EXIT
valid_warm_outputs() {
	[ "$(cat work.exit)" = 0 ] &&
	grep -Fxq 'P100A_Q35_DATAFLOW unload_clean=1 state=2 active=0 cold=0 service_closed=1' work.stdout &&
	awk '
		$1=="P100A_Q35_DATAFLOW" && $2~/^warm_run=/ {
			delete value
			for (i=2;i<=NF;i++) {split($i,p,"="); value[p[1]]=p[2]}
			run=value["warm_run"]
			if ((run==1 || run==2) && value["prompt_tokens"]==5 && value["output_tokens"]==2 && value["prompt_seconds"]+0>0 && value["decode_seconds"]+0>0 && value["decode_tokens_per_second"]+0>0) seen[run]++
			else invalid++
		}
		$1=="P100A_Q35_DATAFLOW" && $2~/^model_load_seconds=/ {
			split($2,p,"="); if (p[2]+0>0) loads++; else invalid++
		}
		$1=="P100A_Q35_DATAFLOW" && $2=="generated_ids=11751,13" {ids++}
		$1=="P100A_Q35_DATAFLOW" && $2~/^generated_ids=/ && $2!="generated_ids=11751,13" {invalid++}
		/P100A_Q35_DATAFLOW output_begin run=1/ {begin1++}
		/P100A_Q35_DATAFLOW output_end run=1/ {end1++}
		/P100A_Q35_DATAFLOW output_begin run=2/ {begin2++}
		/P100A_Q35_DATAFLOW output_end run=2/ {end2++}
		$0==" Paris." {text++}
		/P100A_Q35_DATAFLOW diagnostic_one_run=/ {invalid++}
		END {exit !(loads==1 && seen[1]==1 && seen[2]==1 && ids==2 && begin1==1 && end1==1 && begin2==1 && end2==1 && text==2 && !invalid)}
	' work.stdout
}

cat > base <<'DATA'
P100A_Q35_DATAFLOW model_load_seconds=1.5 resident_bytes=100
P100A_Q35_DATAFLOW warm_run=1 prompt_tokens=5 prompt_seconds=1.0 output_tokens=2 decode_seconds=2.0 decode_tokens_per_second=0.5
P100A_Q35_DATAFLOW generated_ids=11751,13
P100A_Q35_DATAFLOW output_begin run=1
 Paris.
P100A_Q35_DATAFLOW output_end run=1
P100A_Q35_DATAFLOW warm_run=2 prompt_tokens=5 prompt_seconds=1.1 output_tokens=2 decode_seconds=2.1 decode_tokens_per_second=0.4
P100A_Q35_DATAFLOW generated_ids=11751,13
P100A_Q35_DATAFLOW output_begin run=2
 Paris.
P100A_Q35_DATAFLOW output_end run=2
P100A_Q35_DATAFLOW unload_clean=1 state=2 active=0 cold=0 service_closed=1
DATA
printf '0\n' > work.exit
cp base work.stdout
valid_warm_outputs || exit 1
sed 's/generated_ids=11751,13/generated_ids=0,0/g' base > work.stdout
if valid_warm_outputs; then exit 1; fi
sed 's/warm_run=2/warm_run=1/g' base > work.stdout
if valid_warm_outputs; then exit 1; fi
sed 's/unload_clean=1/unload_clean=0/g' base > work.stdout
if valid_warm_outputs; then exit 1; fi
sed 's/model_load_seconds=1.5/model_load_seconds=0/g' base > work.stdout
if valid_warm_outputs; then exit 1; fi
printf '%s\n' WARM_VALIDATOR_SELFTEST_OK
