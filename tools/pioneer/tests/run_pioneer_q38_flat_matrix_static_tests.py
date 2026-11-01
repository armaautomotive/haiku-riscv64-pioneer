#!/usr/bin/env python3
"""Selected non-grammar bodies only;actual grammar execution is a separate gate."""
import ast
from collections import Counter
import hashlib
import os
import pathlib
import runpy
import struct
import sys

assert len(sys.argv)==2 and not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
new=pathlib.Path(sys.argv[1]);assert new.is_absolute() and not new.is_symlink()
old=new/'unchanged-empty'
here=pathlib.Path(__file__).resolve().parent
saved=sys.argv[:]
try:
    sys.argv=[str(here/'run_pioneer_q38_empty_receipt_poll_emitted_tests.py'),str(old)]
    audit=runpy.run_path(sys.argv[0],run_name='__main__')
finally:
    sys.argv=saved
tree=ast.parse((here.parent/'build_qwen38_flat_matrix_grammar_root.py').read_text())
pins=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
          and any(isinstance(t,ast.Name) and t.id=='PINS' for t in n.targets))
assert len(pins)==5
for name,size,digest in pins:
    data=(new/name).read_bytes();assert len(data)==size and hashlib.sha256(data).hexdigest()==digest
# GCC coalesced the NEW route literal with the existing canonical route
# OBJECT. The inherited decoder intentionally refuses ambiguous identities.
# Leave that decoder/source unchanged. This NEW consumer's object view may
# drop ONLY the extra known symbol after proving SAME start,size,ALL28 bytes
# and the authoritative seven route values. No other alias is collapsed;
# no executable target,MMIO,unknown object or numeric result is normalized.
body_globals=audit['bodies'].__globals__
original_objects=body_globals['objects']
alias_views=0
def exact_route_alias_view(directory):
    global alias_views
    rows=original_objects(directory)
    if directory!=new:return rows
    alias='PioneerQ38FlatMatrixGrammar::JobShape(unsigned int const*, unsigned int const*, unsigned int, unsigned int)::routes'
    matches=[r for r in rows if r[2]==alias]
    assert len(matches)==3,'ALL3 converted TUs have separately named route DATA'
    removed=[]
    for row in matches:
        at,size,label,data=row
        assert size==28 and data==struct.pack('<7I',786,978,1170,785,977,1169,1361)
        canonical=[r for r in rows if r[0]==at and r[2]=='PioneerQ38ProjectionRoute(unsigned int)::route']
        assert len(canonical)<=1
        if canonical:
            assert canonical[0][1]==size and canonical[0][3]==data
            removed.append(row)
    assert len(removed)==3,'Only the THREE actually coalesced aliases may be dropped'
    alias_views+=1
    return [r for r in rows if r not in removed]
body_globals['objects']=exact_route_alias_view
def scope_induction_literal(directory,rows):
    result=[]
    for name,words,calls,literals,scalars in rows:
        if name!='GlobalChain::Scope(bool)':
            result.append((name,words,calls,literals,scalars));continue
        # EXACT frozen function,not a generic permission to normalize code
        # pointers or addresses outside OBJECT bounds. Scope compares32
        # header words. a3 starts-8;only unsigned a3<=5 may load through a5:
        # iterations8..13,exactly the SIX words of this readonly CSWTCH.
        # The apparent code-end pointer is NEVER read on other iterations.
        table=body_globals['audit']['functions'](directory)
        entries=[(at,size) for at,(size,label) in table.items() if label==name]
        assert entries==[(0x64000,1548)]
        raw=(directory/'conv.bin').read_bytes()[:1548]
        actual=list(struct.unpack('<387I',raw))
        assert actual[83]&0xfff==0x7b7 and actual[84]&0xfff==0x637
        assert actual[86]&0xfffff==0x78793 and actual[87]&0xfffff==0x60613
        fixed={88:0xff800693,89:0x00500313,90:0x00052e03,91:0x00000893,
               92:0x38d37463,93:0x151e1263,94:0x00478793,95:0x00450513,
               96:0x00168693,97:0xfef612e3,318:0x0007a883,319:0xc79ff06f}
        for at,value in fixed.items():assert actual[at]==value,(name,at)
        sign=lambda value:value-4096 if value&2048 else value
        first=(actual[83]&0xfffff000)+sign(actual[86]>>20)
        last=(actual[84]&0xfffff000)+sign(actual[87]>>20)
        assert last-first==128
        objects=original_objects(directory)
        reads=[]
        for iteration in range(32):
            counter=(iteration-8)&0xffffffff
            if counter<=5:reads.append(first+iteration*4)
        assert reads==list(range(first+32,first+56,4))
        matches={(size,label,data) for at,size,label,data in objects if at==reads[0] and size==24}
        assert len(matches)==1
        size,label,data=next(iter(matches));assert label=='CSWTCH' and len(data)==24
        # Keep ALL actual table bytes/size and the exact iteration/load
        # pattern in the comparison. The loop-end pointer is NOT an object
        # read:its accidental placement inside another table is irrelevant.
        end_marker=[item for item in literals if item[:2]==(84,87)]
        assert len(end_marker)==1
        remaining=[item for item in literals if item[:2]!=(84,87)]
        normalized=words[:]
        for at in (83,84):normalized[at]&=0xfff
        for at in (86,87):normalized[at]&=0xfffff
        result.append((name,normalized,calls,remaining,scalars,
                       ('exact32wordLoop','unsignedMinus8Through23','sixReadonlyLoads8Through13',size,label,data)))
    return result
count=0
for name in (*audit['names'],'q38_layer_return_recurrent_empty_receipt_seal'):
    assert 'BootShape(' not in name and 'JobShape(' not in name,'grammar predicate is deliberately changed'
    before=scope_induction_literal(old,audit['literals'](old,audit['bodies'](old,name)))
    after=scope_induction_literal(new,audit['literals'](new,audit['bodies'](new,name)))
    if before!=after:
        for a,b in zip(before,after):
            print('BODY_DIFFERENCE',a[0],b[0],'words',len(a[1]),len(b[1]),
                  'wordChanges',[(i,hex(x),hex(y)) for i,(x,y) in enumerate(zip(a[1],b[1])) if x!=y][:16],
                  'calls',a[2],b[2],'objectsEqual',a[3]==b[3],'scalarEqual',a[4]==b[4],flush=True)
            if a[3]!=b[3]:print('OBJECT_METADATA',a[3],b[3],flush=True)
    assert before==after,('non-grammar instructions,calls or readonly bytes changed beyond LINK relocation',name)
    count+=len(before)
body_globals['objects']=original_objects
assert alias_views
print('EXACT_READONLY_ROUTE_SYMBOL_ALIASES all3 sameStart+28B+all7 canonical words proved;extraSymbolsOnly;parent decoder unchanged',flush=True)
print('EXACT_SCOPE_INDUCTION_RELOCATION32 iterations/conditionalReadonlyLoads8..13/ALL24 table bytes identical;no code-pointer dereference or generic out-of-object normalization grant',flush=True)
for name in ('q38_pushed_begin_shared','q38_pushed_receipt_shared','q38_pushed_end_shared'):
    before=audit['callback_body'](old,name);after=audit['callback_body'](new,name)
    assert before==after,('complete callback body changed beyond qualified direct LINK/readonly OBJECT relocation',name)
    print('EXPLICIT_UNRESOLVED_CALLBACKS_BYTE_PRESERVED',name,before[2],flush=True)
calls=audit['audit']['audit']['direct_named_calls']
for name in ('DensePartitionRoot::Transfer(','MixedRecurrentCohort::Packet('):
    assert Counter(calls(old,name))==Counter(calls(new,name)),name
print(f'PASS static3718->88e4/{count} selected arithmetic,release,commit,feedback,live-source/FIFO/TID/gather/empty-wrapper bodies '
      'byte-exact except authenticated LINK/readonly-literal and exact bounded readonly-induction relocations;3 whole callback bodies+indirect words preserved '
      'but callback targets NOT statically resolved;transfer/Packet direct-call multisets unchanged;'
      'changed Boot/Job predicate and inlined indexed-switch paths NOT proved here;'
      'NO connected-model,native,physical,Work or timing grant',flush=True)
