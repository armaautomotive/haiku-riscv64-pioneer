#!/usr/bin/env python3
"""Synthetic parser equivalence only; no physical performance or quality claim."""
import os
import pathlib
import runpy
import tempfile
import compare_pioneer_q38_token_manual_ttsync_log as candidate
import compare_pioneer_q38_token_phase_profile_log as parent

assert not any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ)
here=pathlib.Path(__file__).resolve().parent
# Every unchanged predecessor assertion runs FIRST, not a replaced validator.
runpy.run_path(str(here/'pioneer_q38_token_constant_q4_comparator_test.py'))
old=runpy.run_path(str(here/'pioneer_q38_token_phase_profile_comparator_test.py'))
positive=negative=equivalent=0
with tempfile.TemporaryDirectory(prefix='pioneer-manual-ttsync-SYNTHETIC-',dir='/private/tmp') as tmp:
    root=pathlib.Path(tmp);plain=root/'SYNTHETIC-phase.log';wrapped=root/'SYNTHETIC-manual.log';error=root/'SYNTHETIC.stderr'
    error.write_bytes(b'')
    def check(body):
        global positive,negative,equivalent
        plain.write_bytes(body);wrapped.write_bytes(candidate.BANNER+body)
        before=(plain.read_bytes(),wrapped.read_bytes(),error.read_bytes())
        outcomes=[]
        for reader,path in ((parent.read_log,plain),(candidate.read_log,wrapped)):
            try:outcomes.append(('accepted',reader(path,error)))
            except (ValueError,OSError):outcomes.append(('refused',None))
        assert outcomes[0]==outcomes[1]
        equivalent+=1
        if outcomes[0][0]=='accepted':positive+=1
        else:negative+=1
        assert before==(plain.read_bytes(),wrapped.read_bytes(),error.read_bytes())
    check(old['envelope']())
    check(old['envelope'](body=old['fixture'](unavailable=True).encode('ascii')))
    for word in range(96):
        for bit in (0,7,15,31):
            packet=old['packet']();packet[word]^=1<<bit
            check(old['envelope'](words=packet))
    raw=candidate.BANNER+old['envelope']()
    refusals=0
    for data in (old['envelope'](),candidate.BANNER+raw,raw[1:],raw.replace(candidate.BANNER,candidate.BANNER[:-1]),
                 raw.replace(b'HOST_LABEL',b'HOST_LABEl',1),candidate.BANNER+b'\0'+old['envelope'](),
                 candidate.BANNER+old['envelope']()+candidate.BANNER):
        wrapped.write_bytes(data)
        try:candidate.read_log(wrapped,error)
        except (ValueError,OSError):refusals+=1
        else:raise AssertionError('malformed host label accepted')
    assert refusals==7
    wrapped.write_bytes(raw);error.write_bytes(b'nonempty')
    try:candidate.read_log(wrapped,error)
    except (ValueError,OSError):refusals+=1
    else:raise AssertionError('stderr accepted')
print('PASS_MANUAL_TTSYNC_READER_SYNTHETIC equivalentParentDecisions'+str(equivalent)+
      ' accepted'+str(positive)+' refused'+str(negative)+' labelOrStderrRefusals'+str(refusals)+
      ';UNCHANGED phase/frontier/reference validators,NO new card/quality/Work result')
