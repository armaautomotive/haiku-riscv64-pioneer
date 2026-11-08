# Shape of tag129 fixed-input diagnostic only; NOT a numeric/ownership pass.
BEGIN { stage=0; preflight=0; uploaded=0; requested=0; rate=0; tokens=0; bad=0 }
/^HYBRID_FULL_CANDIDATE preflight; unqualified, no device open yet$/ {
 if(stage!=0)bad=1; stage=1; next
}
/^HYBRID_PREFLIGHT [0-9]+\/1351$/ {
 split($2,a,"/"); n=a[1]+0;
 if(stage!=1 || n<=preflight || n>1351)bad=1;
 preflight=n; next
}
/^HYBRID_UPLOAD [0-9]+\/21167624192$/ {
 split($2,a,"/"); n=a[1]+0;
 if((stage!=1 && stage!=2) || preflight!=1351 || n<=uploaded || n>21167624192)bad=1;
 stage=2; uploaded=n; next
}
/^HYBRID_START_REQUEST diagnostic only; no completion implied$/ {
 if(stage!=2 || uploaded!=21167624192 || requested++)bad=1;
 next
}
/^HYBRID_STARTED upload_and_start_seconds=[0-9]+\.[0-9][0-9][0-9]$/ {
 if(stage!=2 || uploaded!=21167624192 || requested!=1)bad=1;
 stage=3; next
}
/^HYBRID_PENDING seconds=[0-9]+\.[0-9][0-9][0-9] pc_first=[0-9]+ pc_second=[0-9]+; diagnostic only$/ {
 if(stage!=3)bad=1; next
}
/^HYBRID_RESULT prompt_tokens=1 generated_tokens=[01] inference_seconds=[0-9]+\.[0-9][0-9][0-9] \(includes START and prompt processing\)$/ {
 if(stage!=3)bad=1; split($3,a,"="); expected=a[2]+0;
 split($4,b,"="); if(b[2]+0<=0)bad=1; stage=4; next
}
/^QGW8_GENERATED_TOKENS_PER_SECOND_INCLUDING_START_AND_PROMPT [0-9]+\.[0-9][0-9][0-9][0-9][0-9][0-9]$/ {
 if(stage!=4 || rate || !expected || tokens)bad=1; rate++; next
}
/^HYBRID_TOKEN [0-9]+$/ {
 if(stage!=4 || !rate || $2+0>=248320 || tokens>=expected)bad=1;
 token=$2+0; tokens++; next
}
/^HYBRID_COMPLETE native FINISH and close succeeded; output quality not validated$/ {
 if(stage!=4 || tokens!=expected || rate!=!!expected)bad=1; stage=5; next
}
/^QGW8_HIDDEN_SNAPSHOT sha256=[0-9a-f]+ bytes=20548; DATA_ONLY_AFTER_CLOSE$/ {
 split($2,a,"="); if(stage!=5 || length(a[2])!=64)bad=1;
 digest=a[2]; stage=6; next
}
{ bad=1 }
END {
 if(bad || stage!=6)exit 1;
 printf "QGW8_HIDDEN_SHAPE_CLEAN outputs=%d token=%d sha256=%s;NUMERICS_NOT_CHECKED\n",tokens,tokens?token:-1,digest
}
