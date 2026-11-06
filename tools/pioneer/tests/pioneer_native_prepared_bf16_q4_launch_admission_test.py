#!/usr/bin/env python3
"""Exercise the REAL idle awk filter; no remote commands, files or device IO."""
from pathlib import Path
import subprocess
import sys
assert not sys.argv[1:]
stage='/boot/home/pioneer-native-prepared-bf16-q4-compare115-a'
source=(Path(__file__).resolve().parents[1]/'pioneer_native_prepared_bf16_q4_physical.sh').read_text()
idle=source.split('idle(){\n',1)[1].split('metadata(){\n',1)[0]
program=idle.split('| awk -v self="$$" \'\n',1)[1].rsplit("'\n",1)[0]
header='   Id Team\n'
parent='501 /bin/sh '+stage+'/parent.sh\n'
path_wrapper='500 /boot/system/bin/sh '+stage+'/launch.sh\n'
stdin_wrapper='500 /boot/system/bin/sh -s\n'
other='600 /boot/home/pioneer-native-unrelated-test\n'
cases=[(header+parent,True),(header+parent+path_wrapper,False),
       (header+parent+stdin_wrapper,True),(header+parent+stdin_wrapper+other,False),
       (header+parent+path_wrapper+other,False),(parent+stdin_wrapper,False),
       (header+header+parent+stdin_wrapper,False),(header+parent+'MALFORMED\n',False)]
for text,okay in cases:
 result=subprocess.run(['awk','-v','self=501',program],input=text,text=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
 assert (result.returncode==0)==okay,(text,result.stderr)
assert 'stage='+stage+'\n' in source
print('PASS_REAL_IDLE_FILTER cases8 pathWrapperCollisionRejected_stdinWrapperAdmitted_foreignOwnerAndMalformedStillRejected;NO_HARDWARE')
