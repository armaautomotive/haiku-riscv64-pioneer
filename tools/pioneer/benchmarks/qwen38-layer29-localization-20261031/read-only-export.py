"""Read-only debugger export of immutable token-0 DATA, NOT full-run evidence."""
import hashlib
import json
import pathlib
import lldb

target = lldb.debugger.GetSelectedTarget()
process = target.GetProcess()
try:
    assert process.GetProcessID() == 32972
    assert process.GetState() == lldb.eStateStopped
    endpoint = target.FindFirstGlobalVariable('tokenHeldEndpoint')
    io = endpoint.Cast(target.FindFirstType('PioneerSessionIO').GetPointerType()).Dereference()
    pointer = io.GetChildMemberWithName('nativeTrace')
    assert pointer.IsValid() and pointer.GetByteSize() == 8
    error = lldb.SBError()
    trace_address = process.ReadPointerFromMemory(pointer.GetLoadAddress(), error)
    assert error.Success() and trace_address
    impl_address = process.ReadPointerFromMemory(trace_address, error)
    assert error.Success() and impl_address
    impl_type = target.FindFirstType('Q38NativeTraceTest::Impl')
    assert impl_type.IsValid()
    impl = target.CreateValueFromAddress('observed_impl', lldb.SBAddress(impl_address, target), impl_type)
    data = impl.GetChildMemberWithName('data')
    tokens = data.GetChildMemberWithName('token')
    first = tokens.GetChildAtIndex(0)
    if first.GetType().IsArrayType():
        first = first.GetChildAtIndex(0)
    print('READ_ONLY_TRACE_FIELDS', first.GetTypeName(),
          [(first.GetChildAtIndex(i).GetName(), first.GetChildAtIndex(i).GetTypeName())
           for i in range(first.GetNumChildren())])
    metadata = {}
    for name in ('input', 'layers', 'rows', 'selected', 'runnerUp', 'embedded', 'committed'):
        field = first.GetChildMemberWithName(name)
        assert field.IsValid(), name
        metadata[name] = field.GetValueAsUnsigned()
    print('ACTUAL_TOKEN0_METADATA', metadata)
    assert metadata['input'] == 248319 and metadata['layers'] == 64
    assert metadata['rows'] == 248320 and metadata['embedded'] == metadata['committed'] == 1
    assert not data.GetChildMemberWithName('failed').GetValueAsUnsigned()
    assert not data.GetChildMemberWithName('saved').GetValueAsUnsigned()
    output = pathlib.Path('/private/tmp/pioneer-q38-native-token0-readonly-20261031-mx')
    output.mkdir(mode=0o700)
    metadata.update(scope='PARTIAL_NATIVE_HOST_EMULATION_DATA_ONLY_NOT_FULL_RUN_STOP_END_PHYSICAL_OR_TPS',
                    pid=32972, guest_state_writes=0, injected_products=0)
    for name, words in (('hidden', 65*5120), ('logits', 248320)):
        vector = first.GetChildMemberWithName(name).GetNonSyntheticValue()
        assert vector.IsValid() and vector.GetByteSize() == 24
        begin = process.ReadPointerFromMemory(vector.GetLoadAddress(), error)
        assert error.Success() and begin
        end = process.ReadPointerFromMemory(vector.GetLoadAddress()+8, error)
        assert error.Success() and end-begin == words*4
        payload = process.ReadMemory(begin, words*4, error)
        assert error.Success() and len(payload) == words*4
        with (output/(name+'.raw')).open('xb') as stream:
            stream.write(payload)
        metadata[name] = dict(words=words, sha256=hashlib.sha256(payload).hexdigest())
    record = first.GetChildMemberWithName('record')
    assert record.IsValid() and record.GetByteSize() == 256
    payload = process.ReadMemory(record.GetLoadAddress(), 256, error)
    assert error.Success() and len(payload) == 256
    with (output/'record.raw').open('xb') as stream:
        stream.write(payload)
    metadata['record'] = dict(words=64, sha256=hashlib.sha256(payload).hexdigest())
    with (output/'metadata.json').open('x') as stream:
        json.dump(metadata, stream, sort_keys=True, indent=2)
    print('EXPORTED_IMMUTABLE_PARTIAL_TOKEN0_DATA', output, metadata)
finally:
    # No target expression/function calls, writes, PC changes or model injections.
    error = process.Detach()
    assert error.Success(), error
    print('DETACHED_ORIGINAL_HISTORY_WITHOUT_GUEST_WRITES')
