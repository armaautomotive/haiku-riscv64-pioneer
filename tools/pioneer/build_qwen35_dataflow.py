#!/usr/bin/env python3
"""Build the standalone Qwen35 dataflow driver and benchmark client."""
import argparse
import hashlib
import os
import re
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
TAG = '2026112100010168'
GRAPH_TRIAL_TAG = '2026112100020168'
RECOVERY_QUALIFICATION_TAG = '2026112300030001'
FIRMWARE_RECOVERY_QUALIFICATION_TAG = '2026112400030022'
FLEET_INSPECTION_QUALIFICATION_TAG = '2026112300030004'
RISC_RESET_SAMPLE_QUALIFICATION_TAG = '2026112400030023'
ROLE_STAGE_QUALIFICATION_TAG = '2026112300030005'
ROLE_PRESTACK_QUALIFICATION_TAG = '2026112300030010'
ROLE_LOCAL_RESTORE_QUALIFICATION_TAG = '2026112300030011'
ROLE_SERVICE_QUALIFICATION_TAG = '2026112300030012'
ROLE_SERVICE_FAULT_QUALIFICATION_TAG = '2026112300030013'
ROLE_FLEET_SERVICE_QUALIFICATION_TAG = '2026112300030021'
SPAN_QUALIFICATION_TAG = '2026112300030017'
MODEL_PRELOAD_QUALIFICATION_TAG = '2026112300030018'


def verify_qualification_excludes_projection_witness(build, driver):
    nm = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
    symbols = subprocess.check_output([nm, '-C', driver], text=True)
    for name in ('PioneerQ35ProjectionScratchCompileWitness(',
                 'PioneerQ35ProjectionScratchHaikuScopeCompileWitness('):
        if name in symbols:
            raise RuntimeError('loadable qualification contains compile-only projection witness: ' + name)
    print('qualification excludes non-loadable projection scratch witnesses')


def verify_snapshot_witness_refuses_load(build, driver):
    objdump = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump'
    for symbol in ('init_hardware', 'init_driver'):
        disassembly = subprocess.check_output(
            [objdump, '-d', '--disassemble=' + symbol, driver], text=True)
        if (not re.search(r'<'+symbol+r'>:', disassembly)
                or not re.search(r'\blui\s+a0,0x80000\b', disassembly)
                or not re.search(r'\baddi?\s+a0,a0,15\b', disassembly)
                or not re.search(r'\bret\b', disassembly)
                or re.search(r'\b(?:call|jal|jalr)\b', disassembly)):
            raise RuntimeError('snapshot witness does not safely reject ' + symbol)
    print('snapshot witness init entrypoints reject loading; NO HARDWARE')


def verify_fabric_guard_witness_refuses_load(build, guard):
    objdump = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-objdump'
    nm = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-nm'
    symbols = subprocess.check_output([nm, '-C', guard], text=True)
    disassembly = subprocess.check_output(
        [objdump, '-d', '--disassemble=_ZL11StandardOpsiz', guard], text=True)
    if ('sFabricRecoveryWitness' not in symbols or 'CompleteFabricRecovery(' not in symbols
            or '<_ZL11StandardOpsiz>:' not in disassembly
            or not re.search(r'\blui\s+a0,0x80000\b', disassembly)
            or not re.search(r'\baddi?\s+a0,a0,15\b', disassembly)
            or not re.search(r'\bret\b', disassembly)
            or re.search(r'\b(?:call|jal|jalr)\b', disassembly)):
        raise RuntimeError('fabric guard witness does not unconditionally reject module loading')
    print('fabric guard witness rejects module loading; NO HARDWARE')


def verify_kernel_stack_budget(driver_directory, qualification=False, firmware_recovery=False,
                               fleet_inspection=False, role_stage=False, role_prestack=False,
                               span_qualification=False, tree_settlement=False):
    usage_file = driver_directory / 'tenstorrent_pci_probe.su'
    entries = []
    for line in usage_file.read_text().splitlines():
        fields = line.split('\t')
        if len(fields) < 3:
            continue
        try:
            frame = int(fields[-2])
        except ValueError:
            continue
        entries.append((fields[0], frame, fields[-1]))

    if span_qualification:
        markers = ('Q38Span', 'Qwen35Span', 'Q38BankControl', 'ResidentSession',
                   'PioneerResidentLease', 'PioneerBulkUpload', 'PioneerWideOutput',
                   'BlackholeDramAllCoresHeld', 'ProbeControl')
        selected = [(name, frame, kind) for name, frame, kind in entries
                    if any(marker in name for marker in markers)]
        for required in ('Q38SpanControl(', 'PioneerQ38SpanUpload(', 'Q38BankControl(', 'ProbeControl('):
            if not any(required in name for name, _, _ in selected):
                raise RuntimeError('missing span qualification stack frame: ' + required)
        if any(kind != 'static' for _, _, kind in selected):
            raise RuntimeError('unbounded span qualification stack frame')
        bound = sum(frame for _, frame, _ in selected) + 4096
        if bound > 18432:
            raise RuntimeError(f'span qualification stack envelope is {bound} bytes')
        print(f'qwen35_span_qualification_stack_envelope_bytes={bound}/18432 '
              '(sum of selected frames plus 4096-byte allowance; not a whole-program proof)')
        return

    if fleet_inspection:
        markers = ('Qwen35FleetInspection', 'PioneerQ35FleetInspectionScope',
                   'Qwen35DedicatedFleet', 'PrepareQ38Dispatch', 'Q38BankControl',
                   'BeginResidentSession', 'EndResidentSession', 'ProbeControl',
                   'PioneerWorkerNocProbe', 'PioneerResidentLease',
                   'Q38CohortPeerLease', 'BlackholeDramAllCoresHeld')
        if role_stage:
            markers += ('Qwen35RoleStage', 'PioneerQ35RoleStageQualification',
                        'DataflowRoleFleetImagesOwner', 'DataflowRoleFleetCodeOwner',
                        'DataflowRoleFleetBootOwner', 'DataflowRoleCodeOwner',
                        'DataflowRoleBootOwner', 'PioneerQwen35RoleFleetPciIO')
        if role_prestack:
            markers += ('PioneerQwen35RolePrestackProbe',)
        selected = [(name, frame, kind) for name, frame, kind in entries
                    if any(marker in name for marker in markers)
                    and (not role_stage or 'PioneerQwen35RoleBorrowedScopeWitness' not in name)]
        required_frames = ['Qwen35FleetInspectionControl(', 'PioneerQwen35InspectHeldFleet(',
                         'PioneerQwen35DedicatedFleetPrepare(', 'PioneerQwen35DedicatedFleetClose(',
                         'Q38BankControl(', 'ProbeControl(']
        if role_stage:
            required_frames += ['Qwen35RoleStageControl(', 'DataflowRoleFleetImagesOwner::CaptureAll(',
                                'DataflowRoleCodeOwner::Stage(', 'DataflowRoleBootOwner::Stage(',
                                'DataflowRoleFleetImagesOwner::RestoreAll(']
        if role_prestack:
            required_frames += ['PioneerQwen35RolePrestackProbe<']
        for required in required_frames:
            if not any(required in name for name, _, _ in selected):
                raise RuntimeError('missing fleet inspection stack frame: ' + required)
        if any(kind != 'static' for _, _, kind in selected):
            raise RuntimeError('unbounded fleet inspection stack frame')
        bound = sum(frame for _, frame, _ in selected) + 4096
        if bound > 18432:
            raise RuntimeError(f'fleet inspection stack envelope is {bound} bytes')
        print(f'qwen35_fleet_inspection_stack_envelope_bytes={bound}/18432 '
              '(sum of selected frames plus 4096-byte allowance; not a whole-program proof)')
        return

    if firmware_recovery:
        markers = ('Qwen35Recovery', 'PioneerFirmware', 'PioneerPostReset',
                   'PioneerQ38ArcQueue', 'BlackholeArc', 'PioneerPciePayload',
                   'PioneerRecoveryRootCommand', 'PioneerPreReset', 'ProbeControl')
        selected = [(name, frame, kind) for name, frame, kind in entries
                    if any(marker in name for marker in markers)]
        required_frames = ['Qwen35RecoveryRequest(', 'Qwen35RecoveryFinishLocked(',
                           'PioneerFirmwareRecoveryOwner<OwnerIO, Guard>::Resume(',
                           'PioneerFirmwareReset<IO>::Start(', 'ProbeControl(']
        if qualification:
            # The compiler inlines the owner Start and Retire methods into
            # this locked call; its emitted frame includes those locals.
            required_frames += ['Qwen35RecoveryRunLocked(',
                                'PioneerFirmwareResetTransport<OwnerIO, Guard>::DisposeAfterVerifiedReset(']
        else:
            required_frames += ['PioneerFirmwareRecoveryOwner<OwnerIO, Guard>::Start(',
                                'PioneerFirmwareRecoveryOwner<OwnerIO, Guard>::Retire(']
        for required in required_frames:
            if not any(required in name for name, _, _ in selected):
                raise RuntimeError('missing firmware recovery stack frame: ' + required)
        if any(kind != 'static' for _, _, kind in selected):
            raise RuntimeError('unbounded firmware recovery stack frame')
        bound = sum(frame for _, frame, _ in selected) + 4096
        if bound > 18432:
            raise RuntimeError(f'firmware recovery stack envelope is {bound} bytes')
        print(f'qwen35_firmware_recovery_stack_envelope_bytes={bound}/18432 '
              '(sum of selected frames plus 4096-byte allowance; not a whole-program proof)')

    if qualification:
        markers = ('Recovery', 'PostReset', 'AsicReset', 'BlackholeArc', 'FirmwareReset',
                   'Q38ArcQueue', 'Q38BankControl', 'BeginResidentSession', 'ProbeControl',
                   'PcieRoot', 'QualificationTrace')
        selected = [(name, frame, kind) for name, frame, kind in entries
                    if any(marker in name for marker in markers)]
        required_frames = ['Qwen35RecoveryRequest(', 'Qwen35RecoveryRunLocked(',
                           'Qwen35RecoveryFinishLocked(', 'Q38BankControl(', 'ProbeControl(']
        required_frames.append('PioneerFirmwareReset<IO>::Start(' if firmware_recovery else 'BlackholeAsicReset(')
        for required in required_frames:
            if not any(required in name for name, _, _ in selected):
                raise RuntimeError('missing recovery qualification stack frame: ' + required)
        if any(kind != 'static' for _, _, kind in selected):
            raise RuntimeError('unbounded recovery qualification stack frame')
        bound = sum(frame for _, frame, _ in selected) + 4096
        if bound > 18432:
            raise RuntimeError(f'recovery qualification stack envelope is {bound} bytes')
        print(f'qwen35_recovery_qualification_stack_envelope_bytes={bound}/18432 '
              '(sum of selected frames plus 4096-byte allowance; not a whole-program proof)')
        return

    dataflow = [(name, frame, kind) for name, frame, kind in entries
                if 'Dataflow' in name or 'Qwen35Dataflow' in name
                or 'PioneerQ35BuildProjectionProgram' in name]
    if not dataflow:
        raise RuntimeError('driver stack report has no Qwen35 dataflow entries')
    too_large = [(name, frame) for name, frame, _ in dataflow if frame > 8192]
    if too_large:
        raise RuntimeError('Qwen35 frame exceeds 8192 bytes: ' + repr(too_large))

    required = {
        'control': 'status_t Qwen35DataflowControl(',
        'generation': 'DataflowGenerationSequence::Run(',
        'decoder': 'DataflowDecoderSequence::Run(',
        'dense': 'DataflowDenseLayerSequence::Run(',
        'recurrent': 'DataflowRecurrentLayerSequence::Run(',
        'ffn': 'DataflowFfnSequence::Run(',
        'authority_build': 'DataflowProjectionAuthority::BuildProjection(',
        'program_build': 'DataflowProgramBuilder::Build(',
        'schedule_build': 'PioneerQ35BuildProjectionProgram(',
        'service_projection': 'DataflowServiceOwner::RunProjection(',
        'owner_projection': 'DataflowProjectionOwner::RunProjection(',
        'audit_projection': 'PioneerSessionIO::AuditProjection(',
        'publish': 'PioneerSessionIO::PublishCommand(',
        'invocation': 'PioneerSessionIO::InvocationMatches(',
        'bind_core': 'DataflowProgramBuilder::BindCore(',
        'delta_build': 'DataflowDeltaProgramBuilder::Build(',
        'delta_bind': 'DataflowDeltaProgramBuilder::BindCore(',
        'delta_matches': 'DataflowProjectionAuthority::DeltaMatches(',
        'owner_delta': 'DataflowProjectionOwner::RunDelta(',
        'audit_delta': 'PioneerSessionIO::AuditDelta(',
        'delta_invocation': 'PioneerSessionIO::DeltaInvocationMatches(',
    }
    sharded_delta = (driver_directory / 'qwen35-dataflow-service-image' / 'delta-shard.marker').is_file()
    resident_marker = driver_directory / 'qwen35-resident-service-image' / 'delta-shard.marker'
    if resident_marker.is_file() != sharded_delta:
        raise RuntimeError('resident and graph images disagree on sharded delta')
    if sharded_delta:
        # The direct, single-command delta owner is deliberately disabled in
        # this witness. Its two graph-record frames are checked below.
        for key in ('delta_bind', 'owner_delta', 'audit_delta', 'delta_invocation'):
            del required[key]
    sizes = {}
    for key, marker in required.items():
        matches = [(frame, kind) for name, frame, kind in entries
                   if marker in name]
        if not matches and key == 'generation' and 'control' in sizes:
            # The compiler may fully inline this template into the ioctl
            # handler. Its stack usage is then included in the measured
            # Qwen35DataflowControl frame, and no separate .su record exists.
            sizes[key] = 0
            print('qwen35_legacy_generation_frame_absent_or_inlined')
            continue
        if not matches or any(kind != 'static' for _, kind in matches):
            raise RuntimeError('missing or unbounded Qwen35 stack frame: ' + key)
        sizes[key] = max(frame for frame, _ in matches)

    # Deliberately sum the projection build and service execution branches,
    # even though they run one after the other. Leave 2.5 KiB for ioctl entry
    # and smaller helpers inside Haiku's 20 KiB kernel stack.
    request_stack = (sizes['control'] + sizes['generation'] + sizes['decoder']
                    + max(sizes['dense'], sizes['recurrent']) + sizes['ffn'])
    projection_branch = (sizes['authority_build'] + sizes['program_build']
                         + sizes['schedule_build'] + sizes['service_projection']
                         + sizes['owner_projection'] + sizes['audit_projection']
                         + sizes['publish'] + sizes['invocation']
                         + sizes['bind_core'])
    recurrent_request_stack = (sizes['control'] + sizes['generation']
                               + sizes['decoder'] + sizes['recurrent'])
    delta_service_branch = 0 if sharded_delta else sizes['owner_delta'] + max(
        sizes['publish'], sizes['delta_invocation'],
        sizes['audit_delta'] + max(sizes['delta_matches'], sizes['delta_bind']))
    delta_build_branch = max(sizes['delta_build'], sizes['delta_matches'])
    delta_branch = recurrent_request_stack + max(
        delta_service_branch, delta_build_branch)
    bound = max(request_stack + projection_branch, delta_branch)
    if bound > 18432:
        raise RuntimeError(f'Qwen35 conservative kernel-stack bound is {bound} bytes')
    print(f'qwen35_kernel_stack_conservative_bytes={bound}/18432')

    # The recorder is compiled only in the non-loadable dispatch witness.
    # Add measured frames from mutually exclusive branches deliberately: this
    # is an envelope for the witness, not an audit of a future ioctl caller.
    if any('PioneerQ35StreamRecordingCompileWitness(' in n for n, _, _ in entries):
        groups = [
            ('recording entry', ('PioneerQ35StreamRecordingCompileWitness(',)),
            ('owned recording entry', ('PioneerQ35GraphOwnerRecord(',)),
            ('recording build', ('DataflowStreamRecording::Build(',)),
            ('recording decoder', ('DataflowDecoderSequence::Run(', 'DataflowStreamRecorder<')),
            ('recording ffn', ('DataflowFfnSequence::Run(', 'DataflowStreamRecorder<')),
            ('recording service', ('DataflowStreamRecorder<Sink>::Run',)),
            ('recording emit', ('DataflowStreamRecorder<Sink>::Emit(',)),
            ('authority method', ('DataflowProjectionAuthority::',)),
            ('program builder', ('DataflowProgramBuilder::Build(',)),
            ('projection schedule', ('PioneerQ35BuildProjectionProgram(',)),
            ('invocation binder', ('ProgramBuilder::BindCore(',)),
            ('invocation validator', ('Invocation::Valid(',)),
            ('stream page stage', ('DataflowStreamPageSink<IO>::StageRecord(',)),
            ('stream reservation', ('PioneerQ35StreamPageIO::Reserve(',)),
            ('stream plan validation', ('DataflowStreamPlanBuilder::Validate(', 'DataflowBoundWorkspace')),
            ('page transfer', ('PioneerResidentStorePage(', 'PioneerSessionIO')),
            ('page command', ('PioneerResidentStoreCommand(', 'PioneerSessionIO')),
        ]
        if sharded_delta:
            for phase in ('DataflowDeltaShardUpdate', 'DataflowDeltaShardNormalize'):
                if not any('DataflowStreamRecorder<Sink>::Emit(' in n and phase in n and kind=='static'
                           for n, _, kind in entries):
                    raise RuntimeError('missing or unbounded sharded delta recorder frame: ' + phase)
        # Paired recurrence adds a non-inlined bridge and compatibility
        # check on the recording path; include both even though the latter
        # does not overlap every other conservative branch above.
        if any('DataflowRecurrentLayerSequence::TryPair(' in n and 'DataflowStreamRecorder<' in n
               for n, _, _ in entries):
            groups.extend([
                ('paired projection bridge', ('DataflowRecurrentLayerSequence::TryPair(', 'DataflowStreamRecorder<')),
                ('paired projection compatibility', ('DataflowProjectionPair::Compatible(',)),
            ])
        total = 2048  # unmeasured small helpers and future caller are not certified
        recorded_frames = {}
        for label, markers in groups:
            matches = [(frame, kind) for name, frame, kind in entries
                       if all(marker in name for marker in markers)]
            if not matches or any(kind != 'static' for _, kind in matches):
                raise RuntimeError('missing or unbounded stream recorder frame: ' + label)
            recorded_frames[label] = max(frame for frame, _ in matches)
            total += recorded_frames[label]
        if total > 18432:
            raise RuntimeError(f'stream recorder witness stack envelope is {total} bytes')
        print(f'qwen35_stream_witness_stack_envelope_bytes={total}/18432 '
              '(includes 2048-byte allowance; real ioctl caller not audited)')
        if any('DataflowGraphGeneration::Run(' in n for n, _, _ in entries):
            # INSTALL calls recording directly from the control handler. The
            # reserve/plan-validation branch precedes the decoder branch, so
            # bound them separately rather than adding both call paths.
            f = recorded_frames
            prefix = sizes['control'] + f['owned recording entry'] + f['recording build'] + 2048
            record_path = (prefix + f['recording decoder'] + f['recording ffn']
                           + f['recording service'] + f['recording emit']
                           + f['authority method'] + f['program builder'] + f['projection schedule']
                           + f['invocation binder'] + f['invocation validator']
                           + f['stream page stage'] + f['page transfer'] + f['page command'])
            reserve_path = prefix + f['stream reservation'] + f['stream plan validation']
            install_bound = max(record_path, reserve_path)
            if install_bound > 18432:
                raise RuntimeError(f'graph install stack envelope is {install_bound} bytes')
            print(f'qwen35_graph_install_stack_envelope_bytes={install_bound}/18432 '
                  '(includes 2048-byte helper allowance; not a whole-program proof)')

        if any('PioneerQ35GraphOwner::InstallResident(' in name for name, _, _ in entries):
            # Bound the new ioctl -> resident setup branch separately from the
            # recorder above. Sum even mutually exclusive staging, verification,
            # mapping and fleet-audit frames; retain a helper allowance. This
            # remains an envelope, not a whole-program proof.
            resident_groups = [
                ('context install', ('PioneerStoreContext::InstallDataflowServiceResources(',)),
                ('resident owner', ('PioneerQ35GraphOwner::InstallResident(',)),
                ('service install', ('DataflowServiceOwner::Install(', 'BeforeStart = PioneerQwen35::DataflowResidentTemplateSetup<PioneerQ35GraphOwner')),
                ('template install', ('DataflowResidentTemplateInstaller::Install(', 'PioneerQ35GraphOwner::InstallResident')),
                ('template verification', ('DataflowResidentTemplateInstaller::VerifyBeforeRelease(', 'PioneerQ35GraphOwner::InstallResident')),
                ('template access', ('ResidentTemplateAccessAllowed(', 'PioneerQ35GraphOwner::InstallResident')),
                ('owner identity', ('ResidentTemplateOwnerAllowed(', 'PioneerQ35GraphOwner::InstallResident')),
                ('sealed stream', ('PioneerQ35StreamRecordingWitness::SealedMatches(',)),
                ('graph descriptor build', ('DataflowGraphRunBuild(',)),
                ('graph input validation', ('DataflowGraphRunInputsValid(',)),
                ('template read', ('DataflowResidentTemplateSetup<Scope>::Adapter<IO>::ReadCoreMemory(', 'PioneerQ35GraphOwner::InstallResident')),
                ('held fleet wrapper', ('PioneerSessionIO::SharedFleetHeldAndQuiescent(',)),
                ('held fleet', ('PioneerQwen35SharedRoleBinding::FleetHeldAndQuiescent(',)),
                ('role hold', ('PioneerQwen35RoleFleetPciIO<IO, Scope, Views, Selector>::AllRolesHeld(',)),
                ('core hold', ('PioneerQwen35DataflowPciMemory<Raw>::CoreHeld(',)),
                ('core map', ('PioneerQwen35DataflowPciMemory<Raw>::SelectMapped(',)),
                ('PCI write', ('PioneerQwen35DataflowPciMemory<Raw>::WriteCoreMemoryInternal(',)),
                ('PCI read', ('PioneerQwen35DataflowPciMemory<Raw>::ReadCoreMemory(',)),
            ]
            resident_total = sizes['control'] + 4096
            for label, markers in resident_groups:
                matches = [(frame, kind) for name, frame, kind in entries
                           if all(marker in name for marker in markers)]
                if not matches or any(kind != 'static' for _, kind in matches):
                    raise RuntimeError('missing or unbounded resident install frame: ' + label)
                resident_total += max(frame for frame, _ in matches)
            if resident_total > 18432:
                raise RuntimeError(f'resident install stack envelope is {resident_total} bytes')
            print(f'qwen35_resident_install_stack_envelope_bytes={resident_total}/18432 '
                  '(includes 4096-byte helper allowance; not a whole-program proof)')

    have_resident_run = any('DataflowResidentOwnedRunner<Handle>::Run(' in name
                            and 'PioneerQ35ResidentDriverOwner::Policy' in name for name, _, _ in entries)
    if tree_settlement and not have_resident_run:
        raise RuntimeError('missing resident run stack frame for tree settlement')
    if have_resident_run:
        # The selected resident driver reaches the ioctl through the resident
        # owner rather than the legacy graph-run witness below. Include both
        # metadata preparation and fleet audit frames even though they are
        # separate branches, with an allowance for smaller helpers. Physical
        # scope is currently denied and must be re-audited when enabled.
        resident_run_groups = [
            ('generation', ('DataflowGraphGeneration::Run(',)),
            ('generation callback', ('DataflowGraphGeneration::Run<', '::<lambda(')),
            ('graph owner', ('PioneerQ35GraphOwnerRun(',)),
            ('resident driver bridge', ('PioneerQwen35ResidentDriverOwner.inc:', 'constprop(')),
            ('owned resident runner', ('DataflowResidentOwnedRunner<Handle>::Run(', 'PioneerQ35ResidentDriverOwner::Policy')),
            ('resident session', ('DataflowResidentGraphSession::Run(', 'PioneerQ35ResidentDriverOwner::Policy')),
            ('resident metadata transaction', ('DataflowResidentModelTransaction::Prepare(', 'PioneerQ35ResidentDriverOwner::Scope')),
            ('metadata decoder', ('DataflowDecoderSequence::Run(', 'DataflowGraphMetadataService')),
            ('metadata ffn', ('DataflowFfnSequence::Run(', 'DataflowGraphMetadataService')),
            ('metadata service', ('DataflowGraphMetadata.h:', 'DataflowGraphMetadataService::')),
            ('authority method', ('DataflowProjectionAuthority::',)),
            ('program builder', ('DataflowProgramBuilder::Build(',)),
            ('projection schedule', ('PioneerQ35BuildProjectionProgram(',)),
            ('host candidate audit', ('PioneerQwen35ResidentHostPci<Raw, Scope>::AuditCandidate(', 'PioneerQ35ResidentDriverOwner::Scope')),
            ('fleet audit', ('DataflowResidentFleetAudit::Audit(', 'PioneerSessionIO')),
            ('stable core read', ('PioneerQwen35ResidentFleetAuditPci<Raw, Scope>::ReadStableResidentCore(', 'PioneerSessionIO')),
            ('PCI read', ('PioneerQwen35DataflowPciMemory<Raw>::ReadCoreMemory(',)),
            ('PCI map', ('PioneerQwen35DataflowPciMemory<Raw>::SelectMapped(',)),
        ]
        resident_run_total = sizes['control'] + 2048
        for label, markers in resident_run_groups:
            matches = [(frame, kind) for name, frame, kind in entries
                       if all(marker in name for marker in markers)]
            if not matches or any(kind != 'static' for _, kind in matches):
                raise RuntimeError('missing or unbounded resident run frame: ' + label)
            resident_run_total += max(frame for frame, _ in matches)
        tree_audit = [(frame, kind) for name, frame, kind in entries
                      if 'DataflowResidentTreeAudit::Audit(' in name and 'PioneerSessionIO' in name]
        if tree_settlement and not tree_audit:
            raise RuntimeError('missing resident tree audit stack frame')
        if tree_audit:
            if any(kind != 'static' for _, kind in tree_audit):
                raise RuntimeError('unbounded resident tree audit frame')
            # Conservatively add this mutually exclusive branch rather than
            # substituting it for the existing full fleet-audit estimate.
            resident_run_total += max(frame for frame, _ in tree_audit)
        if tree_settlement:
            for label, marker in (('compact submit', '::Submit('), ('compact batch', '::StoreBatch('),
                                  ('compact reserved check', '::ReservedPayloadClear('),
                                  ('candidate poll', '::PollCandidate(')):
                matches = [(frame, kind) for name, frame, kind in entries
                           if 'PioneerQwen35ResidentHostPci<Raw, Scope>' in name
                           and 'PioneerSessionIO' in name and marker in name]
                if not matches or any(kind != 'static' for _, kind in matches):
                    raise RuntimeError('missing or unbounded ' + label + ' stack frame')
                resident_run_total += max(frame for frame, _ in matches)
        if resident_run_total > 18432:
            raise RuntimeError(f'resident run stack envelope is {resident_run_total} bytes')
        print(f'qwen35_resident_run_stack_envelope_bytes={resident_run_total}/18432 '
              '(includes 2048-byte helper allowance; physical scope currently denied)')

    # The model transaction is a distinct non-loadable witness. Sum the
    # metadata decoder's measured call frames and conservative branches;
    # this still does not certify a future session/ioctl caller.
    if any('PioneerQ35GraphModelPrepareCompileWitness(' in n for n, _, _ in entries):
        groups = [
            ('model entry', ('PioneerQ35GraphModelPrepareCompileWitness(',)),
            ('metadata decoder', ('DataflowDecoderSequence::Run(', 'DataflowGraphMetadataService')),
            ('metadata ffn', ('DataflowFfnSequence::Run(', 'DataflowGraphMetadataService')),
            ('metadata service', ('DataflowGraphMetadata.h:', 'DataflowGraphMetadataService::')),
            ('authority method', ('DataflowProjectionAuthority::',)),
            ('program builder', ('DataflowProgramBuilder::Build(',)),
            ('projection schedule', ('PioneerQ35BuildProjectionProgram(',)),
        ]
        total = 2048
        for label, markers in groups:
            matches = [(frame, kind) for name, frame, kind in entries
                       if all(marker in name for marker in markers)]
            if not matches or any(kind != 'static' for _, kind in matches):
                raise RuntimeError('missing or unbounded graph model frame: ' + label)
            total += max(frame for frame, _ in matches)
        if total > 18432:
            raise RuntimeError(f'graph model witness stack envelope is {total} bytes')
        print(f'qwen35_graph_model_stack_envelope_bytes={total}/18432 '
              '(includes 2048-byte allowance; real ioctl caller not audited)')

    if any('PioneerQ35GraphSessionRunCompileWitness(' in n for n, _, _ in entries):
        # Sum mutually exclusive preparation, publication and audit branches
        # as an intentionally conservative envelope for the complete witness.
        groups = [
            ('session entry', ('PioneerQ35GraphSessionRunCompileWitness(',)),
            ('owned session entry', ('PioneerQ35GraphOwnerRun(',)),
            ('session run', ('DataflowGraphSession::Run(',)),
            ('model preparation', ('DataflowGraphModelTransaction::Prepare(',)),
            ('metadata decoder', ('DataflowDecoderSequence::Run(', 'DataflowGraphMetadataService')),
            ('metadata ffn', ('DataflowFfnSequence::Run(', 'DataflowGraphMetadataService')),
            ('metadata service', ('DataflowGraphMetadata.h:', 'DataflowGraphMetadataService::')),
            ('authority method', ('DataflowProjectionAuthority::',)),
            ('program builder', ('DataflowProgramBuilder::Build(',)),
            ('projection schedule', ('PioneerQ35BuildProjectionProgram(',)),
            ('publisher', ('DataflowGraphPublisher::Start(', 'PioneerQ35GraphBackend')),
            ('completion', ('DataflowGraphCompletion::Finish(', 'PioneerQ35GraphBackend')),
            ('core audit', ('PioneerSessionIO::AuditGraphCore(',)),
            ('invocation binder', ('ProgramBuilder::BindCore(',)),
        ]
        total = 2048
        for label, markers in groups:
            matches = [(frame, kind) for name, frame, kind in entries
                       if all(marker in name for marker in markers)]
            if not matches or any(kind != 'static' for _, kind in matches):
                raise RuntimeError('missing or unbounded graph session frame: ' + label)
            total += max(frame for frame, _ in matches)
        includes_ioctl = any('DataflowGraphGeneration::Run(' in n for n, _, _ in entries)
        if includes_ioctl:
            for label, markers in [
                ('graph ioctl', ('Qwen35DataflowControl(',)),
                ('graph generation', ('DataflowGraphGeneration::Run(',)),
                ('generation dispatch', ('DataflowGraphGeneration::Run<', '::<lambda(')),
            ]:
                matches = [(frame, kind) for name, frame, kind in entries
                           if all(marker in name for marker in markers)]
                if not matches or any(kind != 'static' for _, kind in matches):
                    raise RuntimeError('missing or unbounded graph generation frame: ' + label)
                total += max(frame for frame, _ in matches)
            generate_handler = [(frame, kind) for name, frame, kind in entries
                                if 'status_t Qwen35DataflowGenerate(' in name]
            if any(kind != 'static' for _, kind in generate_handler):
                raise RuntimeError('unbounded Qwen35 graph generate handler frame')
            if generate_handler:
                total += max(frame for frame, _ in generate_handler)
        if total > 18432:
            raise RuntimeError(f'graph session witness stack envelope is {total} bytes')
        scope = 'includes measured ioctl and 2048-byte helper allowance' if includes_ioctl else \
                'includes 2048-byte allowance; real ioctl caller not audited'
        print(f'qwen35_graph_session_stack_envelope_bytes={total}/18432 ({scope})')

    if any('DataflowGraphFaultAudit::Settle(' in n and 'PioneerQ35GraphBackend' in n
           for n, _, _ in entries):
        # Count sequential audit and close branches together, plus an explicit
        # helper allowance. Inlined frames are included in their callers; this
        # is an envelope, not a whole-program stack proof.
        groups = [
            ('control', ('Qwen35DataflowControl(',)),
            ('fault audit', ('DataflowGraphFaultAudit::Settle(', 'PioneerQ35GraphBackend')),
            ('fault core audit', ('PioneerQ35GraphBackend::AuditGraphRecoveryCore(',)),
            ('NoC1 audit', ('PioneerQwen35DataflowNoC1Audit(', 'PioneerQ35GraphBackend::AuditGraphRecoveryCore')),
            ('runtime audit', ('PioneerQwen35DataflowCoreRuntimeStable(',)),
            ('queue audit', ('PioneerQwen35DataflowQueuesBalanced(',)),
            ('snapshot', ('PioneerSessionIO::ReadWorkerSnapshot(',)),
            ('mailbox', ('PioneerQwen35DataflowReadStableMailbox(',)),
            ('service close', ('PioneerStoreContext::CloseDataflowRewrite(',)),
            ('fleet stop', ('PioneerQwen35DataflowPciMemory<Raw>::StopAndAuditAll(', 'PioneerSessionIO')),
            ('fleet resume', ('PioneerQwen35DataflowPciMemory<Raw>::ResumeRestoredFleet(', 'PioneerSessionIO')),
        ]
        total = 2048
        for label, markers in groups:
            matches = [(frame, kind) for name, frame, kind in entries
                       if all(marker in name for marker in markers)]
            if not matches or any(kind != 'static' for _, kind in matches):
                raise RuntimeError('missing or unbounded graph fault frame: ' + label)
            total += max(frame for frame, _ in matches)
        if total > 18432:
            raise RuntimeError(f'graph fault recovery stack envelope is {total} bytes')
        print(f'qwen35_graph_fault_stack_envelope_bytes={total}/18432 '
              '(includes 2048-byte helper allowance; not a whole-program proof)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('haiku_build', type=Path)
    parser.add_argument('new_output', type=Path)
    parser.add_argument('--compile-recovery-prototype', action='store_true',
                        help='compile the experimental recovery body into a non-loadable driver')
    parser.add_argument('--compile-root-recovery-gateway', action='store_true',
                        help='connect recovery to the non-loadable SG2042 controller gateway')
    parser.add_argument('--compile-device-dispatch-prototype', action='store_true',
                        help='compile incomplete device dispatch into a non-loadable driver')
    parser.add_argument('--compile-role-write-prototype', action='store_true',
                        help='compile fail-closed role write mappings into a non-loadable driver')
    parser.add_argument('--compile-shared-role-owner-prototype', action='store_true',
                        help='compile the fail-closed shared-role owner into a non-loadable driver')
    parser.add_argument('--compile-exclusive-takeover-prototype', action='store_true',
                        help='compile dedicated role staging with release disabled')
    parser.add_argument('--compile-snapshot-takeover-prototype', action='store_true',
                        help='compile held dedicated fleet with role snapshot/restore; non-loadable')
    parser.add_argument('--compile-resident-service-prototype', action='store_true',
                        help='embed the resident service in the non-loadable snapshot witness')
    parser.add_argument('--compile-tree-settlement-prototype', action='store_true',
                        help='enable experimental device-tree settlement in the non-loadable resident witness')
    parser.add_argument('--compile-delta-shard-prototype', action='store_true',
                        help='compile the fused two-stage delta graph in the non-loadable resident witness')
    parser.add_argument('--compile-fabric-guard-prototype', action='store_true',
                        help='compile an explicitly non-loadable fabric-recovery guard witness')
    parser.add_argument('--compile-supported-span-prototype', action='store_true',
                        help='compile the supported-counter Q35 preload into the non-loadable rewrite')
    parser.add_argument('--build-device-dispatch-trial', action='store_true',
                        help='build the dedicated graph qualification driver without ASIC reset')
    parser.add_argument('--build-recovery-qualification', action='store_true',
                        help='build the restricted idle-session ASIC recovery qualification driver')
    parser.add_argument('--build-span-qualification', action='store_true',
                        help='build the restricted supported-counter preload qualification driver')
    parser.add_argument('--build-model-preload-qualification', action='store_true',
                        help='build the restricted actual-model preload qualification driver')
    parser.add_argument('--build-firmware-recovery-qualification', action='store_true',
                        help='build the restricted idle-session firmware reset qualification driver')
    parser.add_argument('--build-fleet-inspection-qualification', action='store_true',
                        help='build the restricted held-fleet inspection driver without reset or inference')
    parser.add_argument('--build-risc-reset-sample-qualification', action='store_true',
                        help='build the restricted read-only RISC reset-unit observation driver')
    parser.add_argument('--build-role-stage-qualification', action='store_true',
                        help='build the restricted held-role image staging and restoration driver')
    parser.add_argument('--build-role-prestack-qualification', action='store_true',
                        help='build the restricted one-core B/NC pre-stack startup and cancellation driver')
    parser.add_argument('--build-role-local-restore-qualification', action='store_true',
                        help='build the restricted one-core B/NC local-RAM restore probe driver')
    parser.add_argument('--build-role-service-qualification', action='store_true',
                        help='build the restricted one-core resident role startup/stop/restore driver')
    parser.add_argument('--build-role-service-fault-qualification', action='store_true',
                        help='build the restricted one-core resident role fault/stop/restore driver')
    parser.add_argument('--build-role-fleet-service-qualification', action='store_true',
                        help='build the restricted 120-core resident role startup/stop/restore driver')
    args = parser.parse_args()
    if args.build_model_preload_qualification:
        if args.build_span_qualification:
            parser.error('model preload qualification selects its own span qualification')
        args.build_span_qualification = True
    if args.build_span_qualification:
        if (args.build_recovery_qualification or args.build_firmware_recovery_qualification
                or args.build_fleet_inspection_qualification or args.build_role_stage_qualification
                or args.build_role_prestack_qualification or args.build_role_local_restore_qualification
                or args.build_role_service_qualification or args.build_role_service_fault_qualification
                or args.build_role_fleet_service_qualification
                or args.compile_supported_span_prototype):
            parser.error('span qualification selects its own restricted build flags')
        args.build_recovery_qualification = True
        args.compile_supported_span_prototype = True
    if args.build_risc_reset_sample_qualification:
        if args.build_fleet_inspection_qualification:
            parser.error('reset sample selects its own fleet inspection')
        args.build_fleet_inspection_qualification = True
    if args.build_role_fleet_service_qualification:
        if args.build_role_service_qualification or args.build_role_service_fault_qualification:
            parser.error('fleet service qualification selects its own service qualification')
        args.build_role_service_qualification = True
    if args.build_role_service_fault_qualification:
        if args.build_role_service_qualification:
            parser.error('fault qualification selects its own service qualification')
        args.build_role_service_qualification = True
    if args.build_role_service_qualification:
        if args.build_role_local_restore_qualification:
            parser.error('service qualification selects its own local-restore qualification')
        args.build_role_local_restore_qualification = True
    if args.build_role_local_restore_qualification:
        if args.build_role_prestack_qualification:
            parser.error('local-restore qualification selects its own pre-stack qualification')
        args.build_role_prestack_qualification = True
    if args.build_role_prestack_qualification:
        if args.build_role_stage_qualification:
            parser.error('pre-stack qualification selects its own role staging')
        args.build_role_stage_qualification = True
    if args.build_role_stage_qualification:
        if args.build_fleet_inspection_qualification:
            parser.error('role staging selects its own fleet inspection')
        args.build_fleet_inspection_qualification = True
    if args.build_fleet_inspection_qualification:
        if args.build_recovery_qualification or args.build_firmware_recovery_qualification:
            parser.error('select only one qualification')
        args.build_recovery_qualification = True
    if args.build_firmware_recovery_qualification:
        if args.build_recovery_qualification:
            parser.error('select only one recovery qualification')
        args.build_recovery_qualification = True
    if args.build_recovery_qualification:
        if (args.build_device_dispatch_trial or args.compile_recovery_prototype
                or args.compile_device_dispatch_prototype or args.compile_role_write_prototype
                or args.compile_shared_role_owner_prototype or args.compile_exclusive_takeover_prototype
                or args.compile_snapshot_takeover_prototype):
            parser.error('recovery qualification selects its own restricted build flags')
        args.compile_recovery_prototype = True
        args.compile_device_dispatch_prototype = True
        args.compile_role_write_prototype = True
        args.compile_shared_role_owner_prototype = True
        args.compile_exclusive_takeover_prototype = True
    if args.build_device_dispatch_trial and (args.compile_recovery_prototype or args.compile_device_dispatch_prototype):
        parser.error('graph qualification cannot enable a recovery compile witness')
    if args.compile_role_write_prototype and not args.compile_device_dispatch_prototype:
        parser.error('role write witness requires the non-loadable device dispatch prototype')
    if args.compile_shared_role_owner_prototype and not args.compile_role_write_prototype:
        parser.error('shared-role owner witness requires the non-loadable role write prototype')
    if args.compile_exclusive_takeover_prototype and not args.compile_shared_role_owner_prototype:
        parser.error('exclusive takeover requires the non-loadable shared-role owner prototype')
    if args.compile_snapshot_takeover_prototype and (not args.compile_shared_role_owner_prototype
            or args.compile_exclusive_takeover_prototype):
        parser.error('snapshot takeover requires shared-role owner and excludes destructive takeover')
    if args.compile_resident_service_prototype and (not args.compile_snapshot_takeover_prototype
            or args.build_recovery_qualification or args.build_device_dispatch_trial):
        parser.error('resident service requires the non-loadable snapshot takeover witness')
    if args.compile_tree_settlement_prototype and not args.compile_resident_service_prototype:
        parser.error('tree settlement requires the non-loadable resident service witness')
    if args.compile_delta_shard_prototype and (not args.compile_resident_service_prototype
            or args.build_recovery_qualification or args.build_device_dispatch_trial):
        parser.error('sharded delta requires the non-loadable resident service witness')
    if args.compile_fabric_guard_prototype and (not args.compile_snapshot_takeover_prototype
            or args.build_recovery_qualification or args.build_device_dispatch_trial):
        parser.error('fabric guard requires the non-loadable snapshot takeover witness')
    if args.compile_supported_span_prototype and (not args.compile_device_dispatch_prototype
            or (args.build_recovery_qualification and not args.build_span_qualification)
            or args.build_device_dispatch_trial):
        parser.error('supported span requires the non-loadable device dispatch prototype')

    if args.compile_root_recovery_gateway and (not args.compile_snapshot_takeover_prototype
            or args.build_recovery_qualification or args.build_device_dispatch_trial):
        parser.error('root recovery gateway requires the non-loadable snapshot prototype')

    build = args.haiku_build.resolve(strict=True)
    output = args.new_output.absolute()
    output.mkdir(mode=0o700)
    env = {key: value for key, value in os.environ.items()
           if not key.startswith('PIONEER_')}

    driver = output / 'driver'
    driver_env = dict(env, PIONEER_Q38_PANEL_DRIVER='15',
                      PIONEER_DRIVER_RELOAD='1',
                      PIONEER_DRIVER_BUILD_TAG=(MODEL_PRELOAD_QUALIFICATION_TAG if args.build_model_preload_qualification
                                               else SPAN_QUALIFICATION_TAG if args.build_span_qualification
                                               else ROLE_FLEET_SERVICE_QUALIFICATION_TAG if args.build_role_fleet_service_qualification
                                               else ROLE_SERVICE_FAULT_QUALIFICATION_TAG if args.build_role_service_fault_qualification
                                               else ROLE_SERVICE_QUALIFICATION_TAG if args.build_role_service_qualification
                                               else ROLE_LOCAL_RESTORE_QUALIFICATION_TAG if args.build_role_local_restore_qualification
                                               else ROLE_PRESTACK_QUALIFICATION_TAG if args.build_role_prestack_qualification
                                               else ROLE_STAGE_QUALIFICATION_TAG if args.build_role_stage_qualification
                                               else RISC_RESET_SAMPLE_QUALIFICATION_TAG if args.build_risc_reset_sample_qualification
                                               else FLEET_INSPECTION_QUALIFICATION_TAG if args.build_fleet_inspection_qualification
                                               else FIRMWARE_RECOVERY_QUALIFICATION_TAG if args.build_firmware_recovery_qualification
                                               else RECOVERY_QUALIFICATION_TAG if args.build_recovery_qualification
                                               else GRAPH_TRIAL_TAG if args.build_device_dispatch_trial else TAG),
                      PIONEER_Q38_SPAN_PRELOAD='1',
                      PIONEER_Q38_SPAN_FAST='1')
    if args.compile_recovery_prototype or args.compile_device_dispatch_prototype:
        driver_env['PIONEER_QWEN35_RECOVERY_COMPILE_ONLY'] = '1'
    if args.compile_root_recovery_gateway:
        driver_env['PIONEER_QWEN35_ROOT_RECOVERY_GATEWAY_WITNESS'] = '1'
    if args.compile_device_dispatch_prototype:
        driver_env['PIONEER_QWEN35_DEVICE_DISPATCH_COMPILE_ONLY'] = '1'
    if args.compile_role_write_prototype:
        driver_env['PIONEER_QWEN35_ROLE_WRITE_COMPILE_ONLY'] = '1'
    if args.compile_shared_role_owner_prototype:
        driver_env['PIONEER_QWEN35_SHARED_ROLE_OWNER_COMPILE_ONLY'] = '1'
        driver_env['PIONEER_QWEN35_SHARED_ROLE_GRAPH_COMPILE_ONLY'] = '1'
    if args.compile_exclusive_takeover_prototype:
        driver_env['PIONEER_QWEN35_EXCLUSIVE_TAKEOVER_COMPILE_ONLY'] = '1'
    if args.compile_snapshot_takeover_prototype:
        driver_env['PIONEER_QWEN35_SNAPSHOT_TAKEOVER_COMPILE_ONLY'] = '1'
    if args.compile_resident_service_prototype:
        driver_env['PIONEER_QWEN35_RESIDENT_SERVICE_COMPILE_ONLY'] = '1'
    if args.compile_tree_settlement_prototype:
        driver_env['PIONEER_QWEN35_TREE_SETTLEMENT_WITNESS'] = '1'
    if args.compile_delta_shard_prototype:
        driver_env['PIONEER_QWEN35_FUSED_RECURRENCE_COMPILE_ONLY'] = '1'
        driver_env['PIONEER_QWEN35_ACTIVATION_SHARING_COMPILE_ONLY'] = '1'
        driver_env['PIONEER_QWEN35_DELTA_SHARD_COMPILE_ONLY'] = '1'
    if args.compile_fabric_guard_prototype:
        driver_env['PIONEER_QWEN35_FABRIC_GUARD_COMPILE_ONLY'] = '1'
    if args.compile_supported_span_prototype:
        driver_env['PIONEER_QWEN35_SUPPORTED_SPAN_COMPILE_ONLY'] = '1'
    if args.build_span_qualification:
        driver_env['PIONEER_QWEN35_SPAN_QUALIFY'] = '1'
    if args.build_model_preload_qualification:
        driver_env['PIONEER_QWEN35_MODEL_PRELOAD_QUALIFY'] = '1'
    if args.build_recovery_qualification:
        driver_env['PIONEER_QWEN35_RECOVERY_QUALIFY'] = '1'
    if args.build_firmware_recovery_qualification:
        driver_env['PIONEER_QWEN35_FIRMWARE_RESET_QUALIFY'] = '1'
    if args.build_fleet_inspection_qualification:
        driver_env['PIONEER_QWEN35_FLEET_INSPECTION_QUALIFY'] = '1'
    if args.build_risc_reset_sample_qualification:
        driver_env['PIONEER_QWEN35_RISC_RESET_SAMPLE_QUALIFY'] = '1'
    if args.build_role_stage_qualification:
        driver_env['PIONEER_QWEN35_ROLE_STAGE_QUALIFY'] = '1'
    if args.build_role_prestack_qualification:
        driver_env['PIONEER_QWEN35_ROLE_PRESTACK_QUALIFY'] = '1'
    if args.build_role_local_restore_qualification:
        driver_env['PIONEER_QWEN35_ROLE_LOCAL_RESTORE_QUALIFY'] = '1'
    if args.build_role_service_qualification:
        driver_env['PIONEER_QWEN35_ROLE_SERVICE_QUALIFY'] = '1'
    if args.build_role_service_fault_qualification:
        driver_env['PIONEER_QWEN35_ROLE_FAULT_QUALIFY'] = '1'
    if args.build_role_fleet_service_qualification:
        driver_env['PIONEER_QWEN35_ROLE_FLEET_SERVICE_QUALIFY'] = '1'
    if args.build_device_dispatch_trial:
        driver_env['PIONEER_QWEN35_DEVICE_DISPATCH_TRIAL'] = '1'
    subprocess.run(['sh', HERE / 'build_qwen38_banks_driver.sh', build, driver],
                   check=True, env=driver_env)
    if args.build_recovery_qualification:
        verify_qualification_excludes_projection_witness(build, driver / 'tenstorrent_pci_probe-driver60')
    verify_kernel_stack_budget(driver, args.build_recovery_qualification,
                               args.build_firmware_recovery_qualification or
                               (args.compile_recovery_prototype and not args.compile_device_dispatch_prototype),
                               args.build_fleet_inspection_qualification, args.build_role_stage_qualification,
                               args.build_role_prestack_qualification, args.build_span_qualification,
                               args.compile_tree_settlement_prototype)
    if args.compile_snapshot_takeover_prototype:
        verify_snapshot_witness_refuses_load(build, driver / 'tenstorrent_pci_probe-driver60')
    if args.compile_fabric_guard_prototype:
        verify_fabric_guard_witness_refuses_load(build, driver / 'pioneer_p100a_reload_guard')
    if args.compile_resident_service_prototype:
        driver_bytes = (driver / 'tenstorrent_pci_probe-driver60').read_bytes()
        resident = driver / 'qwen35-resident-service-image'
        for section in ('text', 'dispatch'):
            image_bytes = (resident / (section + '.bin')).read_bytes()
            if driver_bytes.count(image_bytes) != 1:
                raise RuntimeError('resident ' + section + ' image is not embedded exactly once')
        print('resident service images embedded exactly once; non-loadable witness only')

    cxx = build / 'cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
    objects = build / 'objects/haiku/riscv64'
    sdk = objects / 'packaging/packages_build/regular/hpkg_-haiku_devel.hpkg/contents/develop'
    release = objects / 'release'
    llama = ROOT / 'src/libs/llama.cpp'
    includes = ['-I' + str(sdk / 'headers'), '-idirafter',
                str(sdk / 'headers/posix'),
                '-I' + str(ROOT / 'headers/private/libroot'),
                '-isystem', str(llama / 'include'),
                '-isystem', str(llama / 'ggml/include')]
    os_headers = sdk / 'headers/os'
    for include in (os_headers, *sorted(os_headers.iterdir())):
        if include.is_dir():
            includes += ['-isystem', str(include)]
    libraries = ['-L' + str(path)
                 for path in sorted(build.glob('build_packages/gcc_syslibs-*/lib'))]
    client = output / 'pioneer-q35-dataflow'
    subprocess.run([
        cxx, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror',
        '-ffp-contract=off', *includes,
        '-B' + str(release / 'system/glue') + '/',
        '-L' + str(release / 'system/libroot'), *libraries,
        HERE / 'pioneer_qwen35_dataflow.cpp',
        HERE / 'PioneerModelWeightCache.cpp', '-o', client],
        check=True, env=env)

    if args.build_span_qualification:
        span_client = output / 'pioneer-q35-span-qualification'
        subprocess.run([
            cxx, '-std=c++17', '-O2', '-Wall', '-Wextra', '-Werror', *includes,
            '-B' + str(release / 'system/glue') + '/',
            '-L' + str(release / 'system/libroot'), *libraries,
            HERE / 'pioneer_qwen35_span_qualification.cpp', '-o', span_client],
            check=True, env=env)
        print('span_client_sha256=' + hashlib.sha256(span_client.read_bytes()).hexdigest())
        print('span_client=' + str(span_client))

    driver_image = driver / 'tenstorrent_pci_probe-driver60'
    print('driver_sha256=' + hashlib.sha256(driver_image.read_bytes()).hexdigest())
    print('client_sha256=' + hashlib.sha256(client.read_bytes()).hexdigest())
    print('driver=' + str(driver_image))
    print('client=' + str(client))


if __name__ == '__main__':
    main()
