#!/usr/bin/env python3
"""Whole fixed full-K Q4 affine transcript DATA,NEVER cleanup/device authority."""
import argparse,json,pathlib,re
from parse_pioneer_q38_full_offload_log import stable_text,require,InvalidLog
def parse_complete(text,stderr=''):
    require(isinstance(stderr,str) and not stderr,'nonempty/unknown stderr')
    require(isinstance(text,str) and len(text)<4096 and text.isascii() and '\r' not in text and '\0' not in text,'whole ASCII/LF data required')
    match=re.fullmatch(r'PASS_NATIVE_EXACT_Q4_AFFINE_FIXED_KERNEL k17408 rows16 dual_jobs2 exact_words32 padding_words32 polls([1-9][0-9]{0,4}) generation23;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_MODEL_OR_SPEED_CLAIM\n',text)
    require(match and 1<=int(match[1])<=20000,'exact single qualification line/poll bounds')
    return {'scope':'fixed synthetic original Q4 fullK/original perK16 FP32 affine,not model','width':17408,'rows':16,'jobs':2,
            'checked_output_words':32,'checked_padding_words':32,'polls':int(match[1]),'generation':23,
            'model_quality':False,'tokens_per_second':None,'execution_or_cleanup_authority_from_log':False}
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('stdout',type=pathlib.Path);parser.add_argument('stderr',type=pathlib.Path)
    args=parser.parse_args()
    try:result=parse_complete(stable_text(args.stdout),stable_text(args.stderr))
    except (OSError,InvalidLog,ValueError) as error:raise SystemExit('REFUSE_EXACT_Q4_AFFINE_LOG '+str(error))
    print(json.dumps(result,sort_keys=True));print('PASS_WHOLE_EXACT_Q4_AFFINE_TRANSCRIPT_DATA_ONLY;NOT_MODEL_SPEED_OR_CLEANUP_AUTHORITY')
if __name__=='__main__':main()
