# Conservative shape check only; this does not validate answer quality.
BEGIN { result=0; complete=0; tokens=0; started=0; preflight=0; upload=0; bad=0 }
/^HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet$/ { preflight++; next }
/^HYBRID_PREFLIGHT [0-9]+\/1344$/ { if($2=="1344/1344")all_verified++; next }
/^HYBRID_UPLOAD [0-9]+\/21137039360$/ { upload++;if($2=="21137039360/21137039360")all_uploaded++;next }
/^HYBRID_STARTED upload_and_start_seconds=[0-9]+\.[0-9][0-9][0-9]$/ { started++; next }
/^HYBRID_RESULT prompt_tokens=1 generated_tokens=[0-9]+ post_start_seconds=[0-9]+\.[0-9][0-9][0-9] \(includes prompt processing\)$/ {
 result++; split($3,a,"="); expected=a[2]+0; split($4,b,"="); seconds=b[2]+0;
 if(expected<0 || expected>32 || seconds<=0 || !started || complete)bad=1; next
}
/^HYBRID_TOKEN [0-9]+$/ {
 if(result!=1 || complete || $2+0>=248320 || tokens>=32)bad=1;
 if(tokens==0)first=$2+0;tokens++;next
}
/^HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated$/ { complete++;next }
{ bad=1 }
END {
 if(bad || preflight!=1 || all_verified!=1 || !upload || all_uploaded!=1 || started!=1 || result!=1 || complete!=1 || tokens!=expected)exit 1;
 printf "HYBRID_SHAPE_CLEAN outputs=%d first_token=%d post_start_seconds=%.3f;QUALITY_NOT_VALIDATED\n",tokens,tokens?first:-1,seconds
}
