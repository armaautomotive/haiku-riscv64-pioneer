# Complete one-shot upload diagnostic transcript gate; not model inference.
NR==1 {
 if($0!="HYBRID_UPLOAD_DIAGNOSTIC source authentication and cache verification; NO DEVICE OPEN YET")exit 1
 next
}
$1=="HYBRID_UPLOAD_PREFLIGHT" {
 if(NF!=2 || progress || pass || $2!=(++checked*21 "/1344"))exit 1
 next
}
$1=="HYBRID_UPLOAD_PROGRESS" {
 if(checked!=64 || pass || NF!=6 || $3!="bytes;" || $4!="NO" || $5!="MODEL" || $6!="EXECUTION")exit 1
 if(split($2,v,"/")!=2 || v[1]!~/^[0-9]+$/ || v[2]!="21137039360")exit 1
 n=v[1]+0
 if(n<=last || n>21137039360 || n%4096)exit 1
 last=n;progress++;next
}
$1=="PASS_HYBRID_UPLOAD_RELEASE" {
 if(checked!=64 || !progress || last!=21137039360 || pass || NF!=8 || $2!="fragments3551" || $3!="spans22005" ||
  $4!="bytes21137039360" || $5!~/^seconds[0-9]+\.[0-9][0-9][0-9];$/ ||
  $6!="SPANS_AND_BANKS_ENDED" || $7!="WEIGHTS_RELEASED" || $8!="NO_MODEL_EXECUTION_OR_TOKEN_RATE")exit 1
 pass=1;next
}
{exit 1}
END{if(checked!=64 || !progress || !pass || last!=21137039360)exit 1}
