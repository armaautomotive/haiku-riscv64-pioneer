#!/usr/bin/env python3
"""Independently check a CLOSED cohort77 receipt;never execute device commands.

A parse is not native ownership/completion authority. The caller must first
observe the actual parent terminate,then qualify its full native lifecycle.
"""
import argparse
from decimal import Decimal, localcontext
import json
import pathlib
import re

MARKER='ONE_COORDINATOR_INTERVAL;NO_SUM_OF_READER_CLOCKS;NOT_PEAK_CAPACITY_AUTHENTICITY_OR_MODEL_TPS_PROOF'
CAPTURED_ROUTES=(0x312,0x3d2,0x492,0x311,0x3d1,0x491,0x551)

def check(body,generation,sweeps,depth,*,direct_tlb=False):
    assert generation>0 and 1<=sweeps<=256 and depth in (1,2,4,8,16)
    assert isinstance(direct_tlb,bool)
    tag='2026110300020077' if direct_tlb else '2026110300010077'
    lines=body.splitlines();assert len(lines)==272 and lines[-1]==MARKER,'complete unique envelope'
    h=re.fullmatch(r'COHORT_DIAGNOSTIC tag'+tag+r' generation([0-9]+) operationrun sweeps([0-9]+) depth([0-9]+) '
        r'ioctl0 errno0 close0 host_us([0-9]+) version6 phase3 retained0 complete1 preflight0 '
        r'busy_mhz([0-9]+) idle_mhz800 views1 pci1 reservation1',lines[0])
    assert h,'clean closed native receipt'
    gen,loops,queue,host,busy=map(int,h.groups())
    assert (gen,loops,queue)==(generation,sweeps,depth) and host>0 and 800<busy<=1400
    words=[]
    for i,line in enumerate(lines[1:269]):
        m=re.fullmatch(r'COHORT_WORD ([0-9]+) ([0-9a-f]{8})',line)
        assert m and int(m[1])==i,'ordered complete DATA words'
        words.append(int(m[2],16))
    assert words[:11]==[6,1,10,0,255,255,255,1,493457,493457,7*depth*4096] and not words[267]
    nonce=words[14];assert 0<nonce<2**32
    def pair(v,index):return v[index]|(v[index+1]<<32)
    total=7*8388608*sweeps
    for role in range(8):
        v=words[11+role*32:43+role*32];assert len(v)==32
        assert v[:6]==[0x36435244,6,role,nonce,sweeps,depth]
        assert pair(v,6)==8388608*sweeps*(1 if role else 7)
        assert pair(v,10)>pair(v,8) and pair(v,12)==pair(v,10)-pair(v,8)
        assert v[15]==512*sweeps*(1 if role else 7)
        if role:
            assert v[14]==CAPTURED_ROUTES[role-1] and not any(v[18:]),'captured card bank ordering'
        else:
            assert 28<=v[14]<=21+7*65536 and v[16]==7 and v[17]==v[14]+14
            releases=[pair(v,18+i*2) for i in range(7)]
            assert 0<releases[0] and all(a<b for a,b in zip(releases,releases[1:])) and releases[-1]<pair(v,12)
            cycles=pair(v,12)
    m=re.fullmatch(r'COHORT_TIMING version1 state([12]) start_us([0-9]+)\.\.([0-9]+) end_us([0-9]+)\.\.([0-9]+) '
        r'elapsed_us([0-9]+)\.\.([0-9]+) cycles([0-9]+) bytes([0-9]+)',lines[269])
    assert m,'complete timing DATA'
    state,sl,su,el,eu,minimum,maximum,measured,transferred=map(int,m.groups())
    assert all(v<2**64 for v in (sl,su,el,eu,minimum,maximum,measured,transferred))
    assert measured==cycles and transferred==total
    result={'scope':'closed receipt DATA only;not native proof by itself','generation':gen,'sweeps':loops,'depth':queue,
        'bytes':total,'coordinatorCycles':cycles,'hostWholeOperationUs':host,'firmwareMHzRecordedNotUsed':busy,
        'timingState':state,'modelTokensPerSecond':None,'peakOrCapacityOrAuthenticityEstablished':False}
    if state==2:
        assert minimum==maximum==0 and lines[270]=='NO_ACCEPTED_BANDWIDTH_ESTIMATE'
        result['bandwidthGBps']=None;result['counterGHzRelativeToHost']=None
        return result
    assert 0<sl<=su<el<=eu and minimum==el-su and maximum==eu-sl and 0<minimum<=maximum
    rate=re.fullmatch(r'HOST_CLOCK_BRACKET_GBPS ([0-9]+\.[0-9]{9})\.\.([0-9]+\.[0-9]{9}) '
        r'COUNTER_GHZ ([0-9]+\.[0-9]{9})\.\.([0-9]+\.[0-9]{9})',lines[270])
    assert rate,'finite positive-bracket rates'
    with localcontext() as ctx:
        ctx.prec=40
        bounds=[Decimal(n)/Decimal(t)/1000 for n,t in ((total,maximum),(total,minimum),(cycles,maximum),(cycles,minimum))]
        assert all(abs(Decimal(s)-v)<=Decimal('0.00000000051') for s,v in zip(rate.groups(),bounds)),'displayed rates match exact bounds'
        result['bandwidthGBps']=[str(v) for v in bounds[:2]]
        result['counterGHzRelativeToHost']=[str(v) for v in bounds[2:]]
    result['observationIntervalUs']=[minimum,maximum]
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=pathlib.Path)
    p.add_argument('--generation',required=True,type=int);p.add_argument('--sweeps',required=True,type=int)
    p.add_argument('--depth',required=True,type=int)
    p.add_argument('--direct-tlb',action='store_true',help='Require the distinct direct-scope revision tag;never auto-detect it.')
    a=p.parse_args()
    try:result=check(a.log.read_text(),a.generation,a.sweeps,a.depth,direct_tlb=a.direct_tlb)
    except (AssertionError,UnicodeError,OSError) as error:
        p.exit(1,'STOP closed cohort receipt rejected: '+str(error)+'\n')
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__=='__main__':main()
