# Fixed two-output tag142 transcript shape only. No CPU comparison, numerical
# quality claim, or independent device-recovery authority is implied.
BEGIN { stage=0; preflight=0; uploaded=0; tokens=0; rate=0; bad=0 }
/^HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet$/ { if(stage!=0)bad=1;stage=1;next }
/^HYBRID_PREFLIGHT [0-9]+\/1351$/ { split($2,a,"/");n=a[1]+0;if(stage!=1 || n<=preflight || n>1351)bad=1;preflight=n;next }
/^HYBRID_UPLOAD [0-9]+\/21094223872$/ { split($2,a,"/");n=a[1]+0;if((stage!=1 && stage!=2) || preflight!=1351 || n<=uploaded || n>21094223872)bad=1;stage=2;uploaded=n;next }
/^HYBRID_START_REQUEST diagnostic only; no completion implied$/ { if(stage!=2 || uploaded!=21094223872 || start_request++)bad=1;next }
/^HYBRID_STARTED upload_and_start_seconds=[0-9]+\.[0-9][0-9][0-9]$/ { if(stage!=2 || uploaded!=21094223872 || start_request!=1)bad=1;stage=3;next }
/^HYBRID_PENDING seconds=[0-9]+\.[0-9][0-9][0-9] pc_first=[0-9]+ pc_second=[0-9]+; diagnostic only$/ { if(stage!=3)bad=1;next }
/^HYBRID_RESULT prompt_tokens=1 generated_tokens=[0-9]+ inference_seconds=[0-9]+\.[0-9][0-9][0-9] \(includes START and prompt processing\)$/ {
 if(stage!=3)bad=1;split($3,a,"=");expected=a[2]+0;split($4,b,"=");seconds=b[2]+0;
 if(expected<1 || expected>2 || seconds<=0)bad=1;stage=4;next
}
/^QGWD_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT [0-9]+\.[0-9][0-9][0-9][0-9][0-9][0-9]$/ { if(stage!=4 || rate || tokens)bad=1;rate++;next }
/^HYBRID_TOKEN [0-9]+$/ { if(stage!=4 || !rate || $2+0>=248320 || tokens>=expected)bad=1;if(tokens==0)first=$2+0;tokens++;next }
/^HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated$/ {
 if(stage!=4 || tokens!=expected || rate!=1)bad=1;
 if(expected==1 && first!=248044 && first!=248046 && (first<248063 || first>248065))bad=1;
 if(expected==2 && (first==248044 || first==248046 || (first>=248063 && first<=248065)))bad=1;
 stage=5;next
}
{ bad=1 }
END { if(bad || stage!=5)exit 1;printf "QGWD_TWO_SHAPE_CLEAN outputs=%d first_token=%d inference_seconds=%.3f;QUALITY_NOT_VALIDATED\n",tokens,first,seconds }
