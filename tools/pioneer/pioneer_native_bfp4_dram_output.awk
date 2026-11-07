# Strict stdout gate for diagnostic120; no GPU access or validation fallback.
# A passing transcript requires the pinned client's full source authentication,
# 192 equal output words, six positive timing records, and three clean ENDs.
BEGIN {sample=0; words=0; times=0; total=0; auth=0; verified=0; final=0; bad=0}
$0=="AUTHENTICATING_FIXED_ORIGINAL_AND_FRESH_LEARNED_REFERENCE;CPU_DIAGNOSTIC_ONLY_NO_DEVICE_YET" && !auth && !verified && !total {auth=1;next}
$0=="AUTHENTICATED_FIXED_MODEL c600de03 tensor7 width5120 rows17408 samples3 payload_and_reference_exact;DEBUG_NOT_WORK" && auth==1 && !verified && !total {verified=1;next}
$1=="NATIVE_GDDR_BAND_RETURNED_WORD" && auth==1 && verified==1 && !final && sample<3 && times==0 && NF==7 {
 first=sample==0?0:sample==1?8704:17376
 if($2!="sample" sample || $3!="first" first || $4!="pass" int(words/32) || $5!="row" (words%32) || words>=64 ||
  length($6)!=11 || $6!~/^got[0-9a-f]+$/ || length($7)!=16 || $7!~/^expected[0-9a-f]+$/ || substr($6,4)!=substr($7,9))bad++
 words++;total++;next
}
$1=="BFP4_GDDR_BAND_CYCLES" && auth==1 && verified==1 && !final && sample<3 && words==64 && NF==4 {
 if($2!="sample" sample || $3!="pass" times || times>=2 || $4!~/^cycles[0-9]+;INCLUDES_GDDR_AND_COMPUTE_NOT_MODEL_SPEED$/ ||
  $4~/^cycles0+;/)bad++
 times++;next
}
$1=="PASS_NATIVE_BFP4_GDDR_BAND" && auth==1 && verified==1 && !final && sample<3 && NF==14 {
 first=sample==0?0:sample==1?8704:17376
 if(words!=64 || times!=2 || $2!="sample" sample || $3!="tensor7" || $4!="first" first || $5!="width5120" ||
  $6!="phases2" || $7!="dual_jobs2" || $8!="exact_words64" || $9!~/^polls[1-9][0-9]*$/ || substr($9,6)+0>20000 ||
  $10!="generation" generation || $11!="requests640" || $12!="flits3200" || $13!="saved_l134892" ||
  $14!="saved_gddr25600;EXPLICIT_END_GDDR_L1_NIU_FIFO_TLB_CLOCK_PCI_RESTORED;NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM")bad++
 sample++;words=times=0;next
}
$0=="PASS_NATIVE_BFP4_GDDR_BANDS samples3 exact_words192 clean_ends3;DEBUG_REFERENCE_NOT_WORK_NO_FULL_MODEL_QUALITY_OR_SPEED_CLAIM" &&
 auth==1 && verified==1 && sample==3 && total==192 && !words && !times && !final {final=1;next}
{bad++}
END {if(generation !~ /^[1-9][0-9]*$/ || sample!=3 || total!=192 || auth!=1 || verified!=1 || final!=1 || bad)exit 1}
