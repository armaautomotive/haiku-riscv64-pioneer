# Timing tag142 transcript shape only. Not a quality or recovery proof.
BEGIN { stage=0; preflight=0; uploaded=0; tokens=0; rate=0; bad=0; timing=0 }
/^HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet$/ { if(stage!=0)bad=1;stage=1;next }
/^HYBRID_PREFLIGHT [0-9]+\/1351$/ { split($2,a,"/");n=a[1]+0;if(stage!=1 || n<=preflight || n>1351)bad=1;preflight=n;next }
/^HYBRID_UPLOAD [0-9]+\/21094223872$/ { split($2,a,"/");n=a[1]+0;if((stage!=1 && stage!=2) || preflight!=1351 || n<=uploaded || n>21094223872)bad=1;stage=2;uploaded=n;next }
/^HYBRID_START_REQUEST diagnostic only; no completion implied$/ { if(stage!=2 || uploaded!=21094223872 || start_request++)bad=1;next }
/^HYBRID_UPLOAD_TIMING valid=[01] read_calls=[0-9]+ read_bytes=[0-9]+ read_ns=[0-9]+ span_calls=[0-9]+ span_bytes=[0-9]+ span_ioctl_ns=[0-9]+;HOST_WALL_TIME_NOT_RAW_PCIE$/ {
 if(stage!=2 || start_request!=1 || timing++)bad=1;
 split($2,v,"=");valid=v[2]+0;split($3,a,"=");split($4,rb,"=");split($5,b,"=");
 split($6,sc,"=");split($7,sb,"=");split($8,c,"=");split(c[2],d,";");reads=a[2]+0;read_ns=b[2]+0;span_ns=d[1]+0;
 measurement=valid && reads && rb[2]+0==21094223872 && sc[2]+0==21907 && sb[2]+0==21094223872;next
}
/^HYBRID_STARTED upload_and_start_seconds=[0-9]+\.[0-9][0-9][0-9]$/ {
 if(stage!=2 || uploaded!=21094223872 || start_request!=1 || timing!=1)bad=1;
 split($2,a,"=");if((a[2]+0)<=0)bad=1;if((read_ns+span_ns)>((a[2]+0)*1000000000+1000000))measurement=0;stage=3;next
}
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
END { if(bad || stage!=5)exit 1;if(measurement)printf "QGWD_UPLOAD_TIMING_SHAPE_CLEAN outputs=%d read_ns=%.0f span_ioctl_ns=%.0f;NOT_RAW_PCIE_OR_QUALITY\n",tokens,read_ns,span_ns;
 else printf "QGWD_UPLOAD_TIMING_UNAVAILABLE outputs=%d;LIFECYCLE_SHAPE_ONLY_NOT_RAW_PCIE_OR_QUALITY\n",tokens }
