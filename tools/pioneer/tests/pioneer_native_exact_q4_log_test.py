#!/usr/bin/env python3
from parse_pioneer_native_exact_q4_log import parse_complete,InvalidLog
base='PASS_NATIVE_EXACT_Q4_FIXED_KERNEL rows16 dual_jobs2 exact_words32 padding_words32 polls1 generation21;EXPLICIT_END_L1_TLB_PCI_RESTORED;NO_FULLK_AFFINE_MODEL_OR_SPEED_CLAIM\n'
for polls in (1,100,20000):assert parse_complete(base.replace('polls1 ',f'polls{polls} '))['polls']==polls
bad=[('', ''),(base.rstrip('\n'),''),(base+'\n',''),(base+base,''),('prefix\n'+base,''),(base,'error'),(base,None),(None,''),(base.replace('\n','\r\n'),''),(base+'\0','')]
for old,new in [('rows16','rows15'),('dual_jobs2','dual_jobs1'),('exact_words32','exact_words64'),('padding_words32','padding_words0'),
                ('generation21','generation20'),('generation21','generation22'),('polls1','polls0'),('polls1','polls20001'),
                ('polls1','polls01'),('polls1','polls-1'),('polls1','polls1.0'),('polls1','polls99999'),
                ('EXACT_Q4','BFP'),('NO_FULLK_AFFINE_MODEL_OR_SPEED_CLAIM','MODEL_SPEED_PROVEN')]:bad.append((base.replace(old,new),''))
for text,stderr in bad:
    try:parse_complete(text,stderr)
    except InvalidLog:continue
    raise AssertionError('invalid transcript accepted')
print(f'PASS_EXACT_Q4_WHOLE_LOG positive3 refusals{len(bad)};DATA_ONLY_NO_DEVICE')
