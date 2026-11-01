#!/usr/bin/env python3
"""Strict reachable formatter audit,NOT a whole-client/kernel stack grant.

GCC may emit conflicting external declarations outside this function's
reachable graph. Preserve raw blocks and use the unchanged strict parser
on the requested subgraph;ANY reachable conflicting declaration still fails.
No missing-node/alias/frame guess or string-stripped symbol is accepted.
"""
import hashlib
import json
from audit_pioneer_q38_stack_graph import BLOCK,FIELD,audit,parse_graph,symbol

FORMATTER='PioneerQ38WriteFrontierFailedReply'
ALLOWED_EXTERNALS=frozenset(('fprintf','fputc','memcpy','memset'))

def audit_formatter_graph(source,functions):
    records=[];roots=set();calls={}
    for match in BLOCK.finditer(source):
        values={key:json.loads(value) if value.startswith('"') else value
                for key,value in FIELD.findall(match[2])}
        if match[1]=='node':
            name=values['title']
            if FORMATTER in symbol(name):roots.add(name)
        else:
            name=values['sourcename'];calls.setdefault(name,set()).add(values['targetname'])
        records.append((match[1],name,match[0]))
    if len(roots)!=1:raise ValueError('exact one actual formatter root required')
    reachable=set();pending=list(roots)
    while pending:
        name=pending.pop()
        if name in reachable:continue
        reachable.add(name);pending.extend(calls.get(name,()))
    # Verbatim original GCC records ONLY. Including ALL outgoing edges is
    # essential:an unknown,indirect or device-control target must NOT vanish.
    selected='\n'.join(raw for _,name,raw in records if name in reachable)+'\n'
    nodes,edges=parse_graph(selected) # unchanged strict duplicate/node checks
    results=audit(nodes,edges,functions,list(roots))
    if len(results)!=1 or results[0]['cycles'] or results[0]['dynamic']:
        raise ValueError('formatter graph is cyclic,dynamic or ambiguous')
    result=results[0]
    if not result['known_module_chain_bytes'] or not set(result['unresolved'])<=ALLOWED_EXTERNALS:
        raise ValueError('formatter has unknown or non-stdio/memory reachability: '+str(result))
    report={'kind':'PARTIAL_USER_FAILED_REPLY_FORMATTER_STACK_AND_IMPORT_AUDIT','roots':results,
            'whole_user_thread_stack_qualified':False,'accelerator_calls_in_formatter':0,
            'raw_graph_sha256':hashlib.sha256(source.encode()).hexdigest(),
            'focused_graph_sha256':hashlib.sha256(selected.encode()).hexdigest(),
            'all_raw_records':len(records),'focused_records':sum(name in reachable for _,name,_ in records),
            'limits':'Exact reachable formatter graph only;unreachable records are NOT audited. All reachable conflicts remain errors. Stdio/memory library costs excluded;NOT whole USER stack,driver,hardware or performance qualification.'}
    return selected,report
