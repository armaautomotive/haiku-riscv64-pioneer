#!/usr/bin/env python3
"""Pure graph-selector/admission negative controls,NOT compiler/hardware proof."""
import json
from pioneer_q38_formatter_graph import audit_formatter_graph

root='unit:_ZL34PioneerQ38WriteFrontierFailedReply_fake'
def node(name,label):return 'node: { title: '+json.dumps(name)+' label: '+json.dumps(label)+' }\n'
def edge(a,b):return 'edge: { sourcename: '+json.dumps(a)+' targetname: '+json.dumps(b)+' }\n'
base=node(root,'formatter\nunit.cpp:1:1\n384 bytes (static)')+node('check','consistency\nunit.cpp:2:1\n16 bytes (static)')
base+=edge(root,'check')+node('fprintf','fprintf')+node('memcpy','memcpy')+edge(root,'fprintf')+edge(root,'memcpy')
focused,report=audit_formatter_graph(base,{})
assert report['roots'][0]['known_module_chain_bytes']==400 and report['roots'][0]['unresolved']==['fprintf','memcpy']
assert not report['whole_user_thread_stack_qualified'] and report['accelerator_calls_in_formatter']==0
unreachable=node('strcmp','unrelated declaration1')+node('strcmp','unrelated declaration2')+edge('strcmp','ioctl')
same,added=audit_formatter_graph(base+unreachable,{})
assert same==focused and added['focused_graph_sha256']==report['focused_graph_sha256']
assert added['raw_graph_sha256']!=report['raw_graph_sha256'] and added['all_raw_records']>report['all_raw_records']
refused=0
def deny(source):
    global refused
    try:audit_formatter_graph(source,{})
    except ValueError:refused+=1
    else:raise AssertionError('unsafe/ambiguous graph admitted')
deny('');deny(node('other','not formatter'))
deny(base+node('other:PioneerQ38WriteFrontierFailedReply','second root\nunit.cpp:3:1\n8 bytes (static)'))
deny(base+node('check','conflicting consistency declaration'))
deny(base+edge(root,'strcmp')+unreachable) # same conflict is fatal once reachable
for forbidden in ('ioctl','open','close','system','__indirect_call','unknown.localalias'):
    deny(base+edge('check',forbidden))
deny(base+edge('check',root))
deny(base.replace('384 bytes (static)','384 bytes (dynamic)'))
deny(base.replace('384 bytes (static)','unknown frame'))
deny(base+edge(root,'missing')+edge('missing','fprintf')) # missing caller node cannot disappear
print(f'PASS formatter reachable graph/preserve verbatim blocks+all edges/unreachable declaration conflicts excluded ONLY outside explicit scope/{refused} reachable-conflict,unknown,control,alias,cycle,dynamic,missing-frame refusals;NO compiler,wholeUSER,native,hardware or TPS proof')
