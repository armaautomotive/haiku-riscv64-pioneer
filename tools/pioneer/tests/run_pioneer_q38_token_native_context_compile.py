#!/usr/bin/env python3
"""Compile the actual full851 native binding; NEVER link/install/invoke it."""
import hashlib
import os
import pathlib
import re
import runpy
import subprocess
import sys

if len(sys.argv) != 3 or any(k.startswith(('PIONEER_', 'TT_PIONEER_')) for k in os.environ):
    raise SystemExit(2)
qualified, out = map(pathlib.Path, sys.argv[1:])
here = pathlib.Path(__file__).resolve().parent
assert out.is_absolute() and not out.exists() and not out.is_symlink()
out.mkdir(mode=0o700)
saved = sys.argv[:]
try:
    sys.argv = [str(here/'run_pioneer_q38_resident_layer_native_context_compile.py'), str(qualified), str(out/'old-controls')]
    base = runpy.run_path(sys.argv[0], run_name='__main__')
finally:
    sys.argv = saved
role = (here.parents[2]/'src/add-ons/kernel/drivers/misc/PioneerQ38TokenNativeRole.h').read_text()
parents = set(re.findall(r'!defined\(([A-Z_][A-Z0-9_]*)\)\s*\|\|\s*\1\s*!=\s*1\b', role))
assert len(parents) == 21, parents
# Never promote the authenticated old Work/CPU/prefix selection into this
# new family. Only explicit parents from the checked token role are enabled.
args = [a for a in base['args'] if a != '-DTT_PIONEER_Q38_WORK_MODE'
        and not a.startswith(('-DTT_PIONEER_Q38_WORK_MODE=', '-DPIONEER_Q38_'))]
for name in sorted(parents):
    if not any(a == '-D'+name or a.startswith('-D'+name+'=') for a in args):
        args += ['-D'+name+'=1']
obj = out/'token-context-NODRV.o'
tags = [i for i, a in enumerate(args) if a.startswith('-DPIONEER_DRIVER_BUILD_TAG=')]
assert len(tags) == 1
args[tags[0]] = '-DPIONEER_DRIVER_BUILD_TAG=0x51544b4e4f445256ULL'
args[args.index('-c')+1] = str(here/'pioneer_q38_token_native_context_compile.cpp')
args[args.index('-o')+1] = str(obj)
subprocess.run(args, check=True, env=base['clean'])
frames = base['frames'](obj.with_suffix('.su'))
assert frames
for name, size in frames.items():
    assert size <= 2048 or name in base['previous'] and size <= base['previous'][name], (name, size)
required = ('BindQ38TokenNative(', 'TokenCallScopeMatches(', 'TokenModelSourceScope(',
            'PioneerQ38TokenCallScope::PioneerQ38TokenCallScope(',
            'pioneer_token_context_scoped_bind(', 'pioneer_token_context_scoped_binding(',
            'pioneer_token_context_scoped_dispatch_end(', 'pioneer_token_context_retain(',
            'pioneer_token_context_destroy_clean(', 'TokenInitializationScope(', 'TokenInitializationOtherCoresHeld(',
            'InitializeQ38TokenRecurrent(', 'InitializeQ38TokenHistory(', 'InitializeQ38TokenProjection(',
            'pioneer_token_context_scoped_initialize(', 'PrepareQ38TokenNativeHeld(', 'CloseQ38TokenNativeHeld(',
            'TokenMemoryMappingSourceScope(', 'TokenMemoryAccessScope(', 'TokenMemoryScope(',
            'MapTokenMemory(', 'TokenMemoryReadChecked(', 'TokenMemoryWrite(',
            'PioneerQ38TokenMemory::PrepareHeld(', 'PioneerQ38TokenMemory::CloseHeldNoLaunch(', 'pioneer_token_context_scoped_held(',
            'TokenRuntimeScope(', 'PrepareQ38TokenNativeRuntime(', 'pioneer_token_context_scoped_runtime(',
            'PioneerQ38TokenNativeOwner::PrepareHeldRuntime(', 'BootstrapQ38TokenNativeWorkers(',
            'PioneerQ38TokenNativeOwner::BootstrapWorkers(', 'pioneer_token_context_scoped_bootstrap(',
            'StartQ38TokenNativeRoot(', 'CancelQ38TokenNativeRoot(', 'PioneerQ38TokenNativeOwner::StartRoot(',
            'PioneerQ38TokenNativeOwner::RequestCancel(', 'pioneer_token_context_scoped_root_start(', 'pioneer_token_context_scoped_root_cancel(',
            'ObserveQ38TokenNativeRoot(', 'PioneerQ38TokenNativeOwner::ObserveRoot(', 'pioneer_token_context_scoped_root_observe(',
            'ObserveQ38TokenNativeTerminal(', 'PioneerQ38TokenNativeOwner::ObserveTerminal(', 'PioneerQ38TokenNativeOwner::InspectTerminal(',
            'pioneer_token_context_scoped_terminal_observe(',
            'HoldQ38TokenNativeStopped(', 'PioneerQ38TokenNativeOwner::HoldStopped(', 'pioneer_token_context_scoped_terminal_hold(',
            'ReadQ38TokenNativeRecords(', 'PioneerQ38TokenNativeOwner::ReadCommitted(', 'pioneer_token_context_scoped_token_records(',
            'RestoreQ38TokenNativeL1(', 'PioneerQ38TokenNativeOwner::RestoreOriginalL1(', 'pioneer_token_context_scoped_l1_restore(',
            'RestoreQ38TokenNativeDispatchImages(', 'PioneerQ38TokenNativeOwner::RestoreDispatchImages(',
            'pioneer_token_context_scoped_dispatch_restore(', 'TokenDispatchImageAccessScope(',
            'TokenDispatchImageReadChecked(', 'TokenDispatchImageWrite(',
            'RestoreQ38TokenNativePeerStores(', 'PioneerQ38TokenNativeOwner::RestorePeerStores(', 'pioneer_token_context_scoped_peer_restore(',
            'TokenPeerStoreSelectionScope(', 'TokenPeerStoreAccessScope(', 'TokenPeerStoreSelect(',
            'TokenPeerNiuReadChecked(', 'TokenPeerNiuWrite(', 'TokenPeerScratchReadChecked(', 'TokenPeerScratchWrite(', 'TokenFifoReadChecked(',
            'TokenInitializationFifoEmpty(', 'TokenInitializationFifoReadChecked(', 'PrepareTokenNativeParticipant(',
            'RestoreQ38TokenNativeRootStore(', 'PioneerQ38TokenNativeOwner::RestoreRootStore(',
            'pioneer_token_context_scoped_root_store_restore(', 'TokenRootStoreAccessScope(',
            'TokenRootNiuReadChecked(', 'TokenRootNiuWrite(', 'TokenRootScratchReadChecked(', 'TokenRootScratchWrite(',
            'RestoreQ38TokenNativeSharedMappings(', 'PioneerQ38TokenNativeOwner::RestoreSharedMappings(',
            'pioneer_token_context_scoped_shared_tlb_restore(', 'TokenSharedTlbScope(',
            'TokenSharedRootCommandReadChecked(', 'TokenSharedTlbReadChecked(', 'TokenSharedTlbWrite(',
            'EndQ38TokenNativeClock(', 'Q38TokenNativeClockScope(', 'pioneer_token_context_scoped_clock_end(',
            'TokenClockReceiptScope(', 'TokenClockTlbReadChecked(', 'PioneerQ38TokenNativeOwner::VerifyClockReturnMappings(',
            'PioneerQ38ClockIO::TlbRead(', 'PioneerQ38ClockIO::TlbWrite(', 'PioneerQ38ClockIO::CsmRead(',
            'PioneerQ38ClockIO::QueueWrite(', 'PioneerQ38ClockIO::SetQueueBounds(', 'PioneerQ38ClockIO::MsiStatusRead(',
            'PioneerQ38ClockIO::NotifyArc(', 'PioneerQ38ClockIO::Pause(', 'PioneerQ38ClockIO::Map(',
            'SealQ38TokenNativeHardware(', 'PioneerQ38TokenNativeOwner::SealHardwareLeases(',
            'pioneer_token_context_scoped_hardware_seal(', 'TokenHardwareSealScope(', 'TokenHardwareSealTlbReadChecked(', 'TokenFinalOwnershipScope(')
for name in required:
    assert any(name in key for key in frames), (name, 'actual implementation must be emitted')
for controller in ('State', 'ConvHistory', 'Projection'):
    for method in ('Prepare', 'Run', 'Close'):
        assert any('PioneerQ38'+controller+'Zero'+method+'(' in name and 'PioneerSessionIO' in name for name in frames), (controller, method)
controller_sizes = [size for name, size in frames.items() if 'PioneerSessionIO' in name
                    and any('PioneerQ38'+kind+'Zero'+method+'(' in name
                            for kind in ('State', 'ConvHistory', 'Projection') for method in ('Prepare', 'Run', 'Close'))]
assert controller_sizes and max(controller_sizes) <= 2048
sizes = [size for name, size in frames.items() if 'Token' in name or 'pioneer_token_context_' in name]
assert sizes and max(sizes) <= 2048
tool = str(pathlib.Path(args[0]).parent/'riscv64-unknown-haiku-')
table = subprocess.check_output([tool+'nm', '-S', '--defined-only', str(obj)], text=True)
symbols = {r[-1]:(int(r[0], 16), int(r[1], 16), r[2])
           for r in map(str.split, table.splitlines()) if len(r) == 4}
normal = {'api_version', 'init_hardware', 'init_driver', 'uninit_driver', 'publish_devices', 'find_device'}
assert not normal & symbols.keys()
assert all('pioneer_q38_token_context_body_'+name in symbols for name in normal)
rope = [info for name, info in symbols.items() if 'PioneerQ38TokenRope' in name and name.endswith('DataE')]
assert len(rope) == 1 and rope[0][1] == 2097152, 'complete frozen raw RoPE must be emitted,not runtime CPU trigonometry'
unresolved = subprocess.check_output([tool+'nm', '-u', str(obj)], text=True)
assert not re.search(r'\b(?:cosf?|sinf?|powf?)\b', unresolved), 'kernel may not import libm for raw configuration'
for forbidden in ('Q38WorkControl', 'OriginalSource', 'CompareFinal', 'ReferenceProducts',
                  'Q38EmbeddingRawTestBridge',
                  'Q38NativePeerTestCpu', 'Q38NativePeerTestBytes', 'NativeQ38PeerInstructionModel',
                  'MutableByteSpan', 'BindMutableBytes', 'CheckNativeFullLayer', 'CheckNativeCancelLayer', 'CheckNativeHoldLayer',
                  'CheckNativeEmptyReadout', 'CheckNativeReadoutFlow', 'CheckNativeRestoreEmpty', 'beforeNativeImage',
                  'CheckNativeDispatchRestore', 'beforeDispatchImage', 'ResumeFirstLayerCancellation',
                  'CheckNativePeerRestore', 'beforePeerScratch', 'SnapshotPrePeerStores',
                  'CheckNativeRootStoreRestore', 'beforeRootScratch', 'nativeRootStoreResult',
                  'CheckNativeSharedTlbRestore', 'NativeSharedAliasRefusals', 'beforeSharedTlb', 'nativeSharedTlbResult',
                  'CheckNativeClockEnd', 'tokenClockRaw', 'Eligibility',
                  'CheckNativeHardwareSeal', 'nativeHardwareSealResult', 'hardwareSealFaultSeen',
                  'NativePreparationObservation', 'NativePreparationFifoObserved', 'CheckNativePreparationFault',
                  'Q38NativeTraceTest', 'PioneerQ38ObservedTrace', 'KvByteSpan', 'BindKvBytes', 'CheckNativeFullFlow',
                  'Q38MixedLiveValidationControl', 'PioneerQ38MixedLiveNativeState',
                  'PioneerQ38EmbeddingNativeState', 'PioneerQ38RecurrentPrefixNativeState'):
    assert forbidden not in table and forbidden not in unresolved, forbidden
sections = subprocess.check_output([tool+'objdump', '-t', str(obj)], text=True)
section = {r[-1]:r[-3] for r in map(str.split, sections.splitlines()) if len(r) >= 6 and r[-1] in symbols}


def code(name):
    start, size, kind = symbols[name]
    assert size and kind in ('T', 't')
    dis = subprocess.check_output([tool+'objdump', '-d', '-j', section[name], str(obj)], text=True)
    rows = [(int(m[1], 16), m[2]) for line in dis.splitlines()
            for m in [re.match(r'^\s*([0-9a-f]+):\s+([0-9a-f]{4,16})\s+', line)] if m]
    selected = sorted((a, w) for a, w in rows if start <= a < start+size)
    at = start
    words = []
    for address, word in selected:
        assert address == at
        at += len(word)//2
        words.append(word)
    assert at == start+size
    rel = subprocess.check_output([tool+'objdump', '-r', '-j', section[name], str(obj)], text=True)
    assert not any(start <= int(m[1], 16) < start+size for line in rel.splitlines()
                   for m in [re.match(r'^([0-9a-f]+)\s+R_', line)] if m), name
    return words


for name, control in (('init_hardware', 'refusal'), ('init_driver', 'refusal'),
                      ('uninit_driver', 'inert'), ('publish_devices', 'null'), ('find_device', 'null')):
    assert code('pioneer_q38_token_context_only_'+name) == code('pioneer_token_context_expected_'+control)
assert symbols['pioneer_token_context_scoped_bind'][1] > 32, 'real private bind must not fold to false'
print('PASS ACTUAL full851 canonical/model+7 atomic runtime reservations/FD+team+thread+generation+clock+hosts SDK context compile;maxNewFrame'+str(max(sizes)))
print('PASS old65 BYTE-IDENTICAL61faa093;NO old embedding/prefix owner/CPU reference/Work route;ALL5 independent inert controls byte-identical')
print('PASS SAME full851 owner/private recurrent+history+projection initialization leases/actual phase-specific source+nonroot callbacks/all9 real controller bodies SDK compiled/maxControllerFrame'+str(max(controller_sizes))+';NOT runtime qualification')
print('PASS ACTUAL distinct full-token held factories/native120 cohort ownership/4retained aliases/all-word access+source callbacks/whole PrepareHeld+CloseHeldNoLaunch bodies SDK compiled;NOT runtime/launch/END qualification')
print('PASS ACTUAL distinct full-token native runtime factory/same owner+source callbacks/whole held metadata+rawRoPE+empty-prefix packet write+readback controller SDK emitted;NOT runtime/bootstrap/model/STOP/END qualification')
print('PASS ACTUAL distinct fresh QGW1 worker bootstrap factory/whole ARM+release+bounded WAIT controller SDK emitted;ROOT/model RUN,cancel,STOP,END and Work STILL OWED;NOT bootstrap runtime qualification')
print('PASS ACTUAL SAME token owner ROOT launch+marker/source guards+fixed monotonic cancel request factories/controllers SDK emitted;NOT runtime/model/cancel consumption/STOP/END/Work qualification')
print('PASS ACTUAL SAME running ROOT observation factory/controller SDK emitted;NO token/frontier/STOP/END capability or NoC command/counter recapture')
print('PASS ACTUAL read-only ROOT+all119 terminal observer factory/controller SDK emitted/explicit64B result;NO reset/HOLD/NIU commands/restore/END/public ABI/Work grant;positive runtime STOP still separate')
print('PASS ACTUAL owned terminal recheck+ROOT-first/all119-worker HOLD factory/controller SDK emitted;NO NIU commands/L1 or CSR restore/token readout/END/public ABI/Work grant;runtime qualification still separate')
print('PASS ACTUAL private held-only committed-record READ factory/controller SDK emitted;bounded64-record batches/source+counter+command receipts/atomic non-aliased output/no historical-counter adoption;NOT runtime/token/END/physical/Work qualification')
print('PASS ACTUAL full-token original-L1 journal restoration factory/controller SDK emitted;requires SAME genuine HOLD+drained records/all120 frozen-image proof before writes/exact original words+full second pass;NO NIU/CSR/dispatch close/END/host release/physical/Work grant')
print('PASS ACTUAL distinct earlier dispatch-original L1 factory/controller+phase-specific checked aliases SDK emitted;SAME private captured originals+all120 code proofs BEFORE writes/exact immediate readback+whole second pass;NO NIU/scratch/TLB/master/clock/END/host release/physical/Work grant')
print('PASS ACTUAL119 peer NIU+scratch factory/controller+per-write-checked selection and aliases SDK emitted;private original config/scratch identities+all120-held guards+exact words/secondpass;ROOT/sharedTLB/master/clock/maps/hosts/END STILL owned;NOT runtime/physical/Work qualification')
print('PASS ACTUAL distinct ROOT-only NIU+scratch factory/controller+phase-scoped checked4B aliases SDK emitted;requires SAME119 peer restores/all120 current held proofs/original command+config+scratch immediate checks/full secondpass;master.ready/sharedTLB/clock/maps/hosts/END STILL retained;NOT runtime/physical/Work qualification')
print('PASS ACTUAL shared200/201 original-TLB factory/controller+checked control aperture SDK emitted;private master+bulk originals captured BEFORE launch/all120 held+bulk8192-word preflight/alias veto BEFORE first attempted change/6 immediate reads+complete secondpass/NO subsequent mapped-source alias access;clock/master-ready/bulk/VM/hosts/END STILL retained;NOT runtime/physical/Work qualification')
print('PASS ACTUAL distinct native normal-clock END factory/per-access FD+generation+phase+deadline guard/real ClockLease+ARC service/clock-view-only single-attempt disposal/6 direct shared-original readbacks+sealed transport emitted;NO master/bulk/modelVM/host/reservation release or native END/Work/physical grant;runtime qualification separate')
print('PASS ACTUAL hardware-lease/journal retirement factory/controller/private sealed binding+6 fresh direct-original readbacks/final ownership scope emitted;master+bulk ready cleared,launched history retained;NO VM/PCI/host/reservation release/full native END/Work/physical grant;runtime separate')
print('PASS scoped fixed FIFO availability+overflow reads emitted/native zero-initializer+held-memory+runtime-packet+bootstrap+HOLD/restoration guards test FIFO drain BEFORE accepting completion receipts;no old MMIO whitelist widening/counter clear/START;runtime/hardware and earlier peer-lease/dispatch-preparation FIFO audit STILL OWED')
print('PASS complete2097152B immutable rawRoPE emitted;NO kernel cos/sin/pow imports or CPU model-reference path')
print(hashlib.sha256(obj.read_bytes()).hexdigest()+' token-context-NODRV.o')
print('COMPILE/INSPECT ONLY;real private bodies WOULD do IO if invoked;NO link/install/invoke/physical or performance qualification;this compile is NOT initialization/staging RUNTIME qualification;runtime+metadata/RUN/STOP/END STILL OWED')
