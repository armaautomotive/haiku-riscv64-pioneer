# Tag146 diagnostic transcript shape only. It cannot establish physical
# weight integrity, broad model quality, Work suitability or speed. The first
# two IDs are pinned to the prior fixed-input physical QGWD/QGWF diagnostics.
BEGIN { stage=0; preflight=0; uploaded=0; tokens=0; rate=0; bad=0 }
/^HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet$/ { if(stage!=0)bad=1;stage=1;next }
/^HYBRID_PREFLIGHT [0-9]+\/1351$/ { split($2,a,"/");n=a[1]+0;if(stage!=1 || n<=preflight || n>1351)bad=1;preflight=n;next }
/^HYBRID_UPLOAD [0-9]+\/21094223872$/ { split($2,a,"/");n=a[1]+0;if((stage!=1 && stage!=2) || preflight!=1351 || n<=uploaded || n>21094223872)bad=1;stage=2;uploaded=n;next }
/^HYBRID_START_REQUEST diagnostic only; no completion implied$/ { if(stage!=2 || uploaded!=21094223872 || start_request++)bad=1;next }
/^HYBRID_STARTED upload_and_start_seconds=[0-9]+\.[0-9][0-9][0-9]$/ { if(stage!=2 || uploaded!=21094223872 || start_request!=1)bad=1;stage=3;next }
/^HYBRID_PENDING seconds=[0-9]+\.[0-9][0-9][0-9] pc_first=[0-9]+ pc_second=[0-9]+; diagnostic only$/ { if(stage!=3)bad=1;next }
/^HYBRID_RESULT prompt_tokens=1 generated_tokens=17 inference_seconds=[0-9]+\.[0-9][0-9][0-9] \(includes START and prompt processing\)$/ {
 if(stage!=3)bad=1;split($4,a,"=");if(a[2]+0<=0)bad=1;stage=4;next
}
/^QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT [0-9]+\.[0-9][0-9][0-9][0-9][0-9][0-9]$/ { if(stage!=4 || rate || tokens)bad=1;rate++;next }
/^HYBRID_TOKEN [0-9]+$/ {
 token=$2+0;if(stage!=4 || !rate || token>=248320 || tokens>=17 ||
  (tokens<2 && token!=70253) || token==248044 || token==248046 || (token>=248063 && token<=248065))bad=1;
 tokens++;next
}
/^QGWD_RESIDENT_FIRST_REQUEST_CLOSED weights retained; output quality not validated$/ {
 if(stage!=4 || tokens!=17 || rate!=1)bad=1;stage=5;next
}
/^QGWF_OBSERVED_DECODE_UNAVAILABLE insufficient_or_invalid_count_window;NOT_A_SPEED_RESULT$/ { if(stage!=5)bad=1;stage=6;next }
/^QGWF_OBSERVED_DECODE_BOUNDS tokens=[0-9]+ first=[0-9]+ last=[0-9]+ min_us=[0-9]+ max_us=[0-9]+ lower_tps=[0-9]+\.[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9] upper_tps=[0-9]+\.[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9];COUNT_WINDOW_NOT_QUALITY$/ {
 if(stage!=5)bad=1;split($2,a,"=");split($3,b,"=");split($4,c,"=");split($5,d,"=");split($6,e,"=");split($7,f,"=");split($8,g,"=");
 count=a[2]+0;first=b[2]+0;last=c[2]+0;min=d[2]+0;max=e[2]+0;lower=f[2]+0;upper=g[2]+0;
 if(first<1 || last!=17 || count!=last-first || count<2 || min<=0 || max<min || lower>upper)bad=1;stage=6;next
}
/^QGWD_RESIDENT_SECOND_STARTED no second upload; diagnostic only$/ { if(stage!=6)bad=1;stage=7;next }
/^QGWD_RESIDENT_SECOND_PENDING seconds=[0-9]+\.[0-9][0-9][0-9] pc_first=[0-9]+ pc_second=[0-9]+$/ { if(stage!=7)bad=1;next }
/^QGWD_RESIDENT_SECOND_MATCH count=17 seconds_including_start=[0-9]+\.[0-9][0-9][0-9]; QUALITY_NOT_VALIDATED$/ {
 if(stage!=7)bad=1;split($3,a,"=");if(a[2]+0<=0)bad=1;stage=8;next
}
/^QGWD_RESIDENT_TWO_REQUESTS_UNLOADED; QUALITY_NOT_VALIDATED$/ { if(stage!=8)bad=1;stage=9;next }
{ bad=1 }
END { if(bad || stage!=9)exit 1;print "QGWD_RESIDENT_SHAPE_CLEAN two fixed 17-token requests, one upload, explicit unload; QUALITY_NOT_VALIDATED" }
