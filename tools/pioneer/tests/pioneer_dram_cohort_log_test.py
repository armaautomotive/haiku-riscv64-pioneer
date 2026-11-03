#!/usr/bin/env python3
"""Synthetic closed receipt/parser refusal tests,not card timing evidence."""
import importlib.util
import pathlib

here=pathlib.Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('cohort_log',here/'compare_pioneer_dram_cohort_log.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

def fixture(sweeps,depth,state):
    words=[0]*268;words[:11]=[6,1,10,0,255,255,255,1,493457,493457,7*depth*4096]
    for role in range(8):
        v=[0]*32;v[:6]=[0x36435244,6,role,0x12345678,sweeps,depth]
        total=8388608*sweeps*(1 if role else 7);v[6]=total&0xffffffff;v[7]=total>>32
        v[8]=100;v[10]=1000100;v[12]=1000000;v[15]=512*sweeps*(1 if role else 7)
        if role:v[14]=module.CAPTURED_ROUTES[role-1]
        else:
            v[14]=28;v[16]=7;v[17]=42
            for i in range(7):v[18+i*2]=(i+1)*100
        words[11+role*32:43+role*32]=v
    total=7*8388608*sweeps
    lines=['COHORT_DIAGNOSTIC tag2026110300010077 generation13 operationrun sweeps%d depth%d ioctl0 errno0 close0 '
        'host_us10000000 version6 phase3 retained0 complete1 preflight0 busy_mhz1350 idle_mhz800 views1 pci1 reservation1'%(sweeps,depth)]
    lines+=['COHORT_WORD %d %08x'%(i,v) for i,v in enumerate(words)]
    lines+=['COHORT_TIMING version1 state%d start_us10..20 end_us1000..1010 elapsed_us%d..%d cycles1000000 bytes%d'%
            (state,980 if state==1 else 0,1000 if state==1 else 0,total)]
    lines+=[('HOST_CLOCK_BRACKET_GBPS %.9f..%.9f COUNTER_GHZ %.9f..%.9f'%
             (total/1000/1000,total/980/1000,1.0,1000000/980/1000)) if state==1 else 'NO_ACCEPTED_BANDWIDTH_ESTIMATE',module.MARKER]
    return '\n'.join(lines)+'\n'

good=bad=0
for loops,depth in ((1,1),(256,16)):
    for state in (1,2):
        body=fixture(loops,depth,state);r=module.check(body,13,loops,depth)
        assert r['bytes']==7*8388608*loops and r['timingState']==state and r['modelTokensPerSecond'] is None
        assert (r['bandwidthGBps'] is not None)==(state==1);good+=1
body=fixture(1,1,1);lines=body.splitlines()
cases=[body[:-len(module.MARKER)-1],body+lines[0]+'\n',body.replace('ioctl0','ioctl-1'),
 body.replace('close0','close-1'),body.replace('retained0','retained1'),body.replace('generation13','generation14'),
 body.replace('elapsed_us980..1000','elapsed_us0..0'),body.replace('start_us10..20','start_us10..2000'),
 body.replace('bytes58720256','bytes58720257'),body.replace('cycles1000000','cycles999999'),
 body.replace('58.720256000','58.720256100'),body.replace('counter','COUNTER')]
# Last string is a deliberate no-op in uppercase-only output;replace it with
# a malformed timing version rather than count an unexecuted mutation.
cases[-1]=body.replace('COHORT_TIMING version1','COHORT_TIMING version2')
for i in range(268):
    if i>=11 and i<267:
        role=(i-11)//32;field=(i-11)%32
        if role and field in (16,17):continue # payload samples are variable DATA
        if not role and field in (18,20,22,24,26,28,30):continue # valid ordered observations may vary
    changed=lines[:];prefix,value=changed[i+1].rsplit(' ',1);changed[i+1]=prefix+' %08x'%(int(value,16)^1)
    cases.append('\n'.join(changed)+'\n')
for body in cases:
    try:module.check(body,13,1,1)
    except AssertionError:bad+=1
    else:raise AssertionError('malformed synthetic receipt accepted')
print('PASS_COHORT_INDEPENDENT_LOG valid%d refused%d exactIntegerIntervals1 unavailableNoRate1 noModelTPS1;SYNTHETIC_DATA_NOT_HARDWARE'%(good,bad))
