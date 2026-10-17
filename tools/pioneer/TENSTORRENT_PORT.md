# Tenstorrent P100A: RISC-V Haiku bring-up

## Status

### October17: exact half-product attention reduction (08:02 UTC)

Native whole Qwen remains too slow. The whole-decoder-only Dot path now
uses one standalone SFPMAD product instead of37 exact-FMA instructions:
both operands are IEEE half,so their product is EXACT normal FP32,even
for half subnormals. General matrix dot*scale+accumulator is unchanged.
Offline tests cover every finite half paired with edge values including
subnormals,plus the existing million random arithmetic trials. Complete
linked whole-decoder repeated-KV/fault tests pass. Legacy resident
attention still rebuilds byte-exact to`ac1c7a4a...c05f`.

Native retest PASSES22 EXACT full-logit positions/four greedy tokens,
4334 projections/616 attention/22 full results,no warm weight uploads.
Short generation3/2.314752s=1.296035tok/s versus1.251689:only3.54% faster,
NOT a repeated CPU speedup gate or Assistant qualification. Preparation
93.033716s;position21 wall.785753s,GPU721.086ms. END2/PCI-host1-1/cold0/
cleanup1. Log:`qwen-whole-v13-halfdot-verify.log`.
Image SHA`4d97a8de31d1f7ea0099209c42a68e3ec2bd72aa5e86e8bad15c42114c8a2e84`;
installed experimental driver
`63670f19233436e412fc709059fdb982c798b11cc965de89df2f1d08fec80f4d`.
Assistant/default boot/CPU/RAM/firmware/SD remain unchanged. Next work is
separately tested concurrent attention groups;no concurrency claim yet.

### October17: native whole-decoder precision corrected (07:56 UTC)

Fast Qwen is NOT yet qualified. The first native whole-position run safely
completed18 positions with one GPU job each,197 projections/28 attention
calls per position and zero intermediate host transfers. Cold preparation
was91.552827s. Positions0..15 had identical full-vocabulary logits at
0.359034..0.623095s/position. Position16 L2=.015730012 passed;position17
L2=.029254553 FAILED the unchanged .02 gate,despite matching argmax.
The run stopped without generation/Assistant promotion. END2/PCI-host1-1/
cold0/cleanup1:the numerical failure did NOT require a power recovery.
Log:`qwen-whole-v13-verify.log`,original driver`c9f84652...313b`,verifier
`cedc29dc...dc07`.

Pinned clean Haiku CPU disassembly proves RoPE uses FMSUB/FMADD after
rounding the other product. The original offline oracle incorrectly
assumed both products were rounded separately. A new integer exact-RNE
FMA matches2M arbitrary finite cases,near cancellation,subnormals,overflow
and signed zero. Four12-bit products avoid unqualified MULHU;explicit
32-bit variable shifts avoid runtime helpers. No BRISC FMADD is assumed.
512 fused RoPE vectors match independent `fmaf` in/out-of-place,and the
actual scalar/whole-decoder instruction tests and ownership faults pass.
The oracle now matches the actual pinned CPU;quality gates are unchanged.

New experimental image SHA
`5b0ad6e845d3ffc65dd7476ff864bea7d8efe23fb6132119f86459c2564768b6`
(17880 bytes),driver
`6596f09ca05d4f4c2cb7791edf4eba48d2eff8cdad1dded4deba3e50a7ba30c5`,
separately named profiling verifier
`f962a9de15fd212906c8821fa71286b81e686593f88dc153610fab5b59528487`.
The previous v13 and v12 drivers remain backed up. Native retest PASSED all
22 positions with EXACT full-vocabulary logits and four greedy tokens:
4334 projections,616 attention calls,22 full results,no warm model upload.
Cold preparation93.129110s;short generation3/2.396762s=1.251689tok/s,
about2.43x the preceding narrow path. This is NOT a repeated CPU speedup
gate or Assistant qualification and remains too slow. Kernel timings show
position21:input10.875ms,metadata20.081ms,GPU749.161ms,completion24.398ms,
snapshot0.653ms. END2/PCI-host1-1/cold0/cleanup1.
Log:`qwen-whole-v13-fma-verify.log`. Existing Assistant/default boot,
CPU/RAM,SD and firmware remain unchanged. Next:reduce redundant GPU
attention instructions,then parallelize its still-serial group execution.

### October17: experimental native whole-decoder contract13 prepared (07:40 UTC)

NOT yet native-qualified or installed at this checkpoint;fast Qwen remains
unachieved. The dedicated controller,immutable model registry and REAL
client pass ASan/UBSan tests. A separate explicit mode9 seals310 validated
immutable offsets,private KV ownership and all119 persistent workers.
All legacy page/stream/runtime mutation paths refuse the sealed mode.
END still live-stops every worker BEFORE restoring either code owner.

The controller verifies independent code/stack/attention/bulk/data/wide
owners,full metadata readback,all119 whole-position sequence advances,
fresh physical-counter summary and two parked PCs BEFORE reset. It uses
the appropriate released-core source/maps/PCI guard at completion rather
than weakening the held-only bulk staging guard. Unknown completion,
source/map/sequence/metadata/host data quarantines ALL owners without
reset/retry/restore/free. Full output is published only after the dedicated
proof;no CPU learned arithmetic validates native production results.
Tests cover all result words,15 unknown-state faults,all code image/padding
regions,failed restore,late page maps,KV commit/reset and old-contract
refusal BEFORE BEGIN. Model tests cover all310 first/last pages,64-bit
offset overflow,alignment,uninitialized data and private KV-tail separation.

Experimental driver SHA
`c9f84652d6bb373ff330687ac08d5d1db85fac59e184aa36f8c4ac03037a313b`.
Separate whole-model verifier SHA
`cedc29dc2714e583188dd58ef4c307ec6e4a95186dc186aaaa871399ee75dc07`.
The verifier's READ-ONLY native inspection passes all fixed architecture,
197 Q8 matrix and113 FP32 norm contracts. This model has tied output and
token embeddings;the binding matches the pinned Qwen3 loader exactly and
a present malformed output tensor remains refused. Pinned clean runtime
libraries only;unrelated llama.cpp changes remain excluded.

Whole-position zero/nonzero/two-position instruction tests and pinned GGML
regressions pass. The existing native v12 driver/Assistant/default boot,
CPU/RAM,SD and firmware remain unchanged at this checkpoint. Next:preserve
v12,controlled v13 native quality/coverage/clean-END test,then measure speed.

### October17: OFFLINE whole-position decoder candidate (07:18 UTC)

Fast native Qwen is NOT achieved. Installed native driver remains contract12
and its measured short decode rate~0.51tok/s. This new candidate is NOT in
the driver,Assistant or native inference. No model-speed claim is made.

`blackhole_decoder.cpp` compiles a fixed28-layer Qwen3-0.6B whole-position
program into16376 bytes,within the separate32KiB code region. Candidate SHA
`3a70bd5a3c3c10afd8a128d83945485850e4c783d6fd699c63a9b267e6f7843f`.
Private data at146000 fits4KiB without overlapping NoC scratch147000.
Hidden/residual,Q/K,norm/gate,input and attention scratch lifetimes are
explicit. It combines RMSNorm,learned norm scales,Q/K/V,NEOX RoPE,device KV
attention,O,residual,FFN and final norm/classifier;only the final full
vocabulary uses the owned host buffer. No intermediate host transfer.

Actual linked master/scalar/attention/SFPU instruction tests pass all28
layers with ZERO and independent sparse NONZERO matrix-peer fixtures.
The sparse test passes TWO sequential positions,changed embedding/RoPE,
exact full-vocabulary outputs and every cached K/V word against an
independent CPU reference. All197 projections,28FFNs,224 heads,10871 peer
executions,119 sequence budgets,physical counter wrap and all151936
returned words are checked. These are modeled sparse peers,NOT execution
of the entire native learned model;unchanged real worker bytes are tested
separately by existing matrix/FFN/wide suites. Stack low6fa0c is inside the
owned8KiB stack. Malformed descriptors,first-read timeout/counter errors,
dropped first/gate copies and dropped/partial/paired-corrupt final host
transfers are rejected without valid output publication.

RMS/sqrt prerequisites pass2M exact IEEE sqrt cases and40000 CPU-reference
rows;linked scalar RMS/NEOX code also passes actual-instruction tests.
512 nontrivial RoPE vectors match in-place/out-of-place CPU arithmetic.
No unsupported integer DIV/REM or floating divide/sqrt is linked. One
test oracle incorrectly used the OLD half converter that flushes very
small values;it was corrected to independent host IEEE binary16,NOT by
changing GPU arithmetic or relaxing checks. A coefficient-read fence was
added before any CPU-equivalent scalar consumption.

Existing resident-attention image still rebuilds to its original
`ac1c7a4a079fffb36223e1a4b4a10eaaf48505544c851b782109a6b214d4c05f`;
contract12 layer image still rebuilds byte-for-byte to`b9dae241...39b1e`.
The next REQUIRED gates are a dedicated fail-closed kernel ownership/code
controller and fault tests,immutable model registration,then actual native
whole-Qwen numerical/coverage/timing qualification. The candidate must NOT
be installed merely because these offline tests passed.

### October17: narrow worker protocol/full Qwen contract v12 (06:59 UTC)

Fast Qwen is STILL NOT achieved. Native small-matrix timings and exact
results pass,but full-model generation did NOT improve:3/5.834276s=
0.514203tok/s versus v11's0.517088tok/s. No repeated CPU speedup claim,
no Assistant promotion. Keeping this evidence prevents confusing a
qualified primitive/protocol improvement with useful model performance.

Full Qwen passed all22 exact full-vocabulary positions/four greedy tokens,
197 tensors/670482432 bytes/163692 pages. Exact coverage per position:
197 learned projections,28 FFNs,zero standalone GLUs,28/224 attention,
112 narrow matrix jobs and one wide output. Totals786 span uploads,
2464 narrow matrices,22 wide results;no warm weight uploads.141 matrix/FFN
plus28 attention host calls STILL remain. END2/PCI-host1-1/cold0/cleanup1,
total151.091s,weights86.552216s,matvec34.476631s,attention11.578397s.
Log:`qwen-narrow-v12-verify.log`,binary SHA
`870bfe84cf39d9c70390189c24c8eccb5036598becc890617a560560aefba87e`.

Private`54df` selects the fixed layer image's trusted-idle narrow matrix,
K1024/2048/3072/5120/6144,N32..4096. It retains fresh descriptor readback,
published worker source/ALL-ID/math/counter proof,gather and one owned16KiB
host write/read/EVERY-bit comparison. Kernel last-proved idle ownership is
valid until its next doorbell;END live-audits ALL STOPs. Dedicated narrow
summary predicate;the strict legacy/debug protocol is unchanged. Unknown
source/map/counter/completion never authorizes reset/retry/code restore.
Native sweep passes changed inputs,all N,exact input reuse,wide transition
and clean END. Warm1024/2048-row calls~6.2/6.7ms. Actual linked image,
independent controller,REAL client,pinned GGML and FFN/GLU/span/full-vocab
regressions pass. A simulator-only fault-test step bound was raised to
cover the complete existing bounded lost-doorbell loop;native limits
were NOT extended. Driver SHA
`9e6dc3b28a38afa5a7f0076c27d13ec18df497211aab96843fcb568094f464ae`,
layer image`b9dae241610337249b6877cea698bf9754b0a39b07a3e5b7124e283817e39b1e`,
native utility`8dec54932f6a6ac1c5acc5071f351b6bba98b951cbd2767089a280a9d7ac4e61`.
v11 rollback retained;natural driver turnover,no firmware/reboot/SD/default
boot/CPU/RAM/model/preferences or unrelated llama.cpp changes.

### October17: native NEW-weight span transport contract v11 (06:50 UTC)

Fast Qwen remains NOT achieved;no Assistant promotion. Native span transfers
passed4096,12288,16384,20480,65536 and1048576 bytes with complete frozen-CPU
transport echo. Warm1MiB=117.453ms;initial4KiB347ms includes code/owner
preparation. Subsequent changed-input6144/32/6144-row matrices were exact,
END2/PCI-host1-1/cold0/cleanup1. Log:`span-v11-native.log`,utility SHA
`f5b63d6f6258ca595ae500ed0b0b282a33cc403edc66003b71a087ffbc5e84e6`.
Driver`f6e91d956e5bd7f29e53f9f1866565fc5f163be66137bc5a55b787bce2a8aa63`,
layer image`ecc68864d3c6599a4ec87edbba26c3a2ad4bce3079382eb413b5522d2037bb95`.
Full-Qwen verification subsequently PASSED all22 exact full-vocabulary
positions and four identical greedy tokens. Coverage197 matrices,28FFNs,
zero standalone GLUs,28/224 attention and one wide output per position.
END2/PCI-host1-1/cold0/cleanup1. Weight preparation/upload fell from
326.170284s to83.408894s(3.91x);total398.153s to155.683s. Short warm decode
remains0.517088tok/s(3/5.801718s),so upload acceleration is NOT a generation
speedup. Log:`qwen-span-v11-verify.log`,binary SHA
`9dd268eda8ede7bbea762f8f259ffbb1092ec81ef1e80d00b538223dcd8a3c27`.

Private`54de` freezes an exact-length1..256-page inline request in one
kernel copy. ONLY new immutable below-KV pages in mode8 may be written.
The separate1MiB host owner is staged/fully read back,then fixed GPU code
proves EVERY GDDR word and ordered complement/original host echoes. The
kernel independently compares EVERY final echo word with frozen input;
only the whole successful proof commits all initialized-page metadata.
No partial bits,retry,checksum or CPU learned-math validation. Unknown
completion/source/map/echo disables BME and retains ALL owners without
reset/code restore. The packed-input shadow is always invalidated because
the upload's private original mirror occupies80000.

Explicit wide-output clients on contract11 can pack consecutive row tiles
into up to256-page batches;legacy transports retain their four-page
boundary. Packing tests compare complete legacy/new byte layouts at
K1024/3072/6144. Actual linked image,independent controller and REAL client
ASan/UBSan fault tests pass;FFN/GLU/full-vocabulary regressions also pass.
The existing v10 driver was preserved and natural driver turnover used.
No reboot,SD/firmware/default boot/CPU/RAM/model/preferences changes.

### October17: full Qwen wide-output contract v10 (06:43 UTC)

Fast Qwen remains NOT achieved; no Assistant promotion. Full verification
passed all22 positions with exact full-vocabulary logits and four identical
greedy tokens. Coverage remains197 learned projections,28 fused FFNs,
zero standalone GLUs and28/224 attention operations/tiles per position,
now with exactly ONE wide-output operation per position. No CPU fallback.
END2/PCI-host1-1/cold0/cleanup1. Log:`qwen-wide-v10-verify.log`,binary SHA
`c56bb39d7218dc4960e980bc4586c881936cb0feda9017855285bf17999815d5`.
Diagnostic warm generation3/5.775715s=0.519416tok/s,not a repeated warm CPU
comparison or speedup claim. Total398.153s,weight preparation326.170284s,
matrix34.319189s,attention11.563695s.141 matrix/FFN plus28 attention host
calls and host norms/RoPE/KV still prevent zero-round-trip decoding.

Private`54dd` uses a separately owned,1MiB-aligned pinned/noncacheable
1MiB host buffer. All256 physical pages are checked independently and
separated from the older input and16KiB owners. A single fixed layer-image
job gathers each worker's complete result,performs an owned host write,
ordered host read and EVERY-bit comparison before reusing its scratch.
Full151936-row vocabulary values return without truncation or argmax-only
substitution. Exact source,physical counters,fresh sequence and PC proofs
precede any reset. Unknown completion retains/quarantines ALL three owners.
Independent summary/proof predicates cover this protocol;legacy/debug
predicates are not weakened. Exact activation reuse remains valid because
the wide-output job does not overwrite the packed input.

Native changed-input primitives passed6144 rows in11.85–11.90ms and all
151936 rows in72.32–72.41ms,including shrink/expand reuse and clean END.
These are primitive measurements,NOT model-generation rates. Driver SHA
`e2cb3ad240cb401480592b7555124f6b186766d54b7ee2edafb56838e8e7aec4`,
layer image`246a4df34e1983c70719dda0864e4ee4386485bdc627435dd595bfccb1972b7e`.
Actual-instruction tests,all-page owner/controller fault injection,real
client and pinned GGML ASan/UBSan tests pass. No firmware,SD,default boot,
CPU/RAM,model/preferences or unrelated llama.cpp edits were changed.

An additional1MiB upload-span prototype is OFFLINE ONLY,behind a default-off
image build flag,with no driver/client ABI. Actual-instruction tests cover
all-word GDDR comparison,complement/original host echoes,packet/store faults
and malformed bounds. Its independent ASan/UBSan controller tests also pass
frozen-input echo,all proof words and unknown-completion quarantine. This
checkpoint does NOT claim native upload acceleration or initialized pages.

### October17: full Qwen FFN graph verification contract v9 (06:15 UTC)

Fast Qwen is STILL NOT achieved; no Assistant promotion. The full verifier
passed all22 positions with exact full-vocabulary logits and all four
CPU-identical greedy reply tokens. Every position had28 fused GPU FFNs,
zero separately scheduled GLUs,197 learned projections and28/224 attention
operations/tiles.197 learned projections now use141 matrix/FFN host calls,
plus28 attention calls. The old summary line in the log still printed197
matrix host calls;source reporting is corrected,not the underlying counters.

Log:`qwen-layer-v9-verify.log`,binary SHA
`b6fb6fdef354b5aaad1ac4a160a6be529bc6d650ff67270c8bd92f67f029c964`.
Retained197 tensors/670482432 bytes/163692 pages,4334 projections,616 fused
FFNs,1232 skipped repeated input uploads. END2/PCI-host1-1/cold0/cleanup1.
Diagnostic warm generation3 samples/6.337664s=0.473361tok/s versus v7's
0.401768tok/s. This remains a short diagnostic,NOT a repeated warm CPU
speedup benchmark. Total402.996s,weight prep331.118875s,matvec38.270141s,
attention11.553895s. The result is still too slow for practical use.

The explicit GGML FFN opt-in requires contract9 BEFORE BEGIN. A four-node
gate/up/SWIGLU/down matcher checks exact shapes,immutable Q8 weights,shared
input,operator semantics and GLOBAL use counts preserved by graph views.
An outside-slice consumer,view or requested intermediate output prevents
elision. The separately scheduled private`54dc` GPU GLU handles that case;
there is no CPU nonlinear fallback. Failed jobs publish no accepted result,
and the client remains close-only. Diagnostic/trace contexts refuse FFN
fusion. The separate warm benchmark option also requires complete28-layer
FFN/matrix/attention coverage and unchanged weights for every request.

`ffn-v9-native.log` passes all3072 standalone GLU and all1024 fused FFN
words exactly on four changed generated inputs. Warm standalone GLU28.45ms
includes deliberately simple full PIO input verification;the normally fused
FFN remains12.19–12.25ms and avoids those intermediate transfers entirely.
Native primitive CLI SHA`5a670c5005f6b88af78924d48c978d4e61f63661ffc44f154e0846e05d80b103`.
Driver SHA`25bbf7b8e343bfd7193c58d54cdfdc591eb0db650617c1fb67a92e91bbe553e3`.
Layer image is unchanged from v8. Independent GLU ownership/controller,
REAL client and pristine-pinned GGML ASan/UBSan tests pass,including global
outside-slice consumers,explicit outputs,old-contract refusal,proof mutations,
source/map/host/timeout faults and no reset after uncertain completion.
No native session is left running. Assistant/SD/default boot/CPU/RAM/firmware,
model/preferences and unrelated dirty llama.cpp edits remain unchanged.

### October17: native fixed feed-forward graph contract v8 (05:54 UTC)

Fast Qwen remains NOT achieved. The new fixed FFN primitive passed natively:
all1024 final FP32 words match independently executed split-card operations
for four changed inputs. First call64.043ms includes code preparation;
warm calls12.156/12.231/12.228ms. This is NOT full-model tok/s,CPU speedup,
or Assistant qualification. Log:`ffn-v8-native.log`,END2/PCI-host1-1/cold0,
cleanup1. Three generated fixture weights were uploaded once; reference
SiLU/quantization/comparison work was OUTSIDE the fused timer.

Private`54db` accepts ONLY fixed1024->3072->1024 Qwen0.6B FFN shapes and
three initialized immutable below-KV weight extents. One immutable job runs
gate/up,fast SiLU,Q8 quantization and down,then one owned host write/ordered
read/ALL-bit comparison. It uses up to96 gate/up and32 down workers;all
unneeded peers remain idle. Fresh sequence increments are derived privately
and bounded BEFORE launch. No intermediate activation crosses PCIe.
The graph always invalidates the ordinary matrix input shadow.

A separate backed-up32KiB code lease at64000..6c000 must coexist with the
existing coordinator stack lease. All code is fixed,fully read back and
restored only after known completion and all-peer STOP at END. The faster
peer protocol relies on the fixed worker's source/ALL-ID/counter proof
BEFORE fresh finished publication;that worker cannot issue another NoC
operation until our next doorbell. Full proof/descriptor readback/physical
counters remain. The graph has a SEPARATE summary predicate,not the stricter
single-matrix predicate. Unknown completion disables BME and quarantines
both owners WITHOUT reset/retry/code restore.

Actual linked RV32IMF/master + unchanged persistent worker bytecode tests
cover32/64/119 cohorts,fast/debug SiLU,Q8 quantization,full physical counter
wrap,all summary mutations,malformed bounds,dropped writes/doorbells,
source faults,partial/paired-corrupt host results. Independent ownership
controller and REAL native-client ASan/UBSan tests cover all proof fields,
old-contract refusal BEFORE BEGIN,shared matrix/graph nonces and close-only
after error. Compact activation/strict probe/actual compact image regressions
also pass. Unrelated llama.cpp edits remain untouched.

Installed driver SHA`65f2f2788ec6229951d09a313fa03ccc69f5c48f88100e8dc23df4cdab943451`,
native CLI`c1f22012c0fcc84b6aebc9e36b633fde9a544045e5963e919807024d696c80e7`,
fixed layer image`24e8bf0cc3f7fed1bc3057dafa30f6738edc158032123e209c7deafb1b4e67eb`.
v7 rollback retained at a unique path. Natural module turnover,no forced
unload/reboot/firmware/SD/default boot/CPU/RAM/settings change. Assistant
remains closed for isolation. GGML graph fusion is the NEXT gate: it is not
implemented by this checkpoint,so the full-model v7 rate below is unchanged.

### October17: compact activation / exact reuse contract v7 (05:40 UTC)

Fast Qwen is NOT achieved. The native full-model verifier passed all22
positions with exact full-logit agreement and a clean END, but its diagnostic
warm decode was only0.401768tok/s. This is NOT a CPU speedup benchmark or an
Assistant promotion. The v6 three-request warm benchmark also completed:
all quality/no-reupload checks passed, but all three speed gates FAILED
(P100A about0.33tok/s,CPU32 about0.64/1.57/1.78tok/s). Its END was clean.

Installed v7 driver SHA-256:
`c12f73b16bb4bb31008236e810f7ff1594a0efcd7c9d89d4b79ce58833821281`.
The private metadata contract is checked before BEGIN. Fast mode transfers
exactly K*2+K/8 activation/scales bytes, not16KiB padding; the fixed image
relocates and verifies every scale on card. An exact kernel-private input
shadow skips a repeated upload only after a fully proven prior job. Every
hit still checks exclusive ownership,mappings,source,PCI/ALL-ID idle and
completion. Changed K/input/scales/signed zero misses; uncertain jobs
invalidate the shadow. Explicit debug mode retains the old full transfer.
There are no learned CPU sums or validation arithmetic in the timed path.

Native evidence at `/boot/home/pioneer-resident-kv-20261017-a/`:

- `coordinated-v7-shapes.log`:32..6144 rows exact,changed/identical pairs,
  14 upload reuses/28 calls,clean END.4096-row hit about8.7ms vs10.0ms miss.
- `coordinated-v7-reuse-attention.log`:all8 heads,layers0/27,n1..4 exact,
  seven upload reuses across KV jobs,clean END.
- `coordinated-v7-vocabulary.log`:all151936 rows exact,119 peers,250.46ms,
  cohort shrink/expand,clean END.
- `qwen-coordinated-v7-verify.log`:197 tensors,670482432 resident bytes,
  4334 matvecs,1848 redundant input uploads skipped,all22 full-logit
  comparisons exact,END2/PCI-host1-1/cold0/cleanup1.

The actual compact image is byte-reproducible at
`50fed70cfabf916d8c0bcb246c7fe9060bcedaa89744f2f824e5990dd89074f0`.
Actual-instruction,controller,client,ownership and pinned-GGML tests pass.
Cold graph tracing confirms197 matrix and28 attention host calls per
position; CPU norms/RoPE/GLU still split the graph. Those round trips now
dominate, so the next work is a fixed card-side feed-forward graph.

`PioneerGraphMath.h`,the scalar instruction probe and the separate layer
coordinator are OFFLINE prerequisites ONLY. No driver exposes or selects
the layer image. Actual RV32IMF scalar tests cover fast/debug SiLU,Q8
quantization,integer IEEE division and malformed/nonfinite rejection.
They do not establish native performance or full-model quality. The layer
prototype still needs executed graph/worker tests and independent ownership
and result predicates before any native exposure. Qualified compact and
legacy images remain unchanged by its compile-time branch.

Assistant remains closed for isolated tests; its installed build/settings,
model,SD-free default boot,64 CPUs/full127.6GiB and firmware are unchanged.
Unrelated dirty llama.cpp edits remain untouched and are not in the runtime.

### October17: fused output / explicit debug contract v6 (04:59 UTC)

Fast Qwen is NOT achieved; no Assistant promotion. Current driver
`ecbaaf5dfbb0339368cccb25676199951873167dbbf6f1f4a92868ba58fcd669`
reports metadata contract6 before BEGIN. Request1 selects an immutable fused
result image; request2 explicitly keeps the slower separate-launch debug
comparison. The image choice cannot change within an ownership lease.
Legacy mode5/6/7 and the strict read-only probe remain separate.

For <=4096 outputs, kernel-derived job words bind ONLY the existing distinct,
pinned/noncacheable16KiB host owner. The immutable GPU validates its exact
bounds, writes the gathered result, performs an ordered fresh host READ,
and compares EVERY original/returned bit before its original completion.
No CPU learned sums, checksum or extra host-controlled comparison launch.
Bus mastering is bounded to that owned job; both PCI command words must be
402 again before a known master stop and host snapshot. All physical NoC
counters include the extra write/read/flits; full source,ALL-ID idle,two-PC,
fresh nonce/shape/summary proof precede ANY reset. Unknown completion disables
BME but NEVER resets/rewrites/retries the running image, and quarantines both
host owners. Larger outputs retain PIO and BME-off device-only execution.

The actual linked RISC/worker interpreter tests all K/routes,finite exact
math,owned host bounds,full original/readback equality,partial/dropped/paired
corruptions,control/NoC/timeouts. Its executed summaries feed the production
acceptance predicate. Controller/real-client ASan/UBSan tests cover both
explicit modes,all proof fields,refusal of old contracts BEFORE BEGIN,and
no CPU output/reset/restore after uncertainty. Other ownership/attention/
persistent/pristine-pinned-GGML tests pass; unrelated llama.cpp edits remain.

Native evidence in the same unique-path directory:

- `coordinated-v6-small.log`: 32/64 rows exact,7.35–7.37ms,clean END.
- `coordinated-v6-attention.log`: all8 heads,layers0/27,n1..4 exact L2=0,
 interleaved fused output jobs exact,clean END.
- `coordinated-v6-shapes.log`: every row exact,32..6144 outputs,clean END.
1024/2048/3072/4096 rows about8.2/9.0/10.0/10.8ms;6144 PIO17.6ms.
3072-row host snapshot is<0.7ms; activation staging still about3.6ms.
- `coordinated-v6-vocabulary.log`: subsequently completed,all rows exact,
 clean END. Full warm benchmark subsequently FAILED all three speed gates;
 see the v7 checkpoint above. No fast-target promotion.

Current tool46922c85,shape tool e0459322,full-Qwen candidate92c5b9a8 live at
unique paths only. GPU image SHA c6c0ed0b36faaddef14911ae1876a1c1796924c99f613499cc5cbdc5ab3d9e33.
Previous debug5/v4 copies remain available. Default boot,SD,64 CPUs/full
127.6GiB usable RAM,firmware/model/settings and installed Assistant unchanged.

### October17: coordinator transport follow-up (04:46 UTC)

Fast Qwen is still NOT achieved. The contract-v2 warm benchmark completed
with exact quality and no weight reupload, but FAILED all three CPU speedup
gates: P100A0.306/0.305/0.310tok/s versus CPU32 2.890/1.578/1.579tok/s.
END2,PCI/host1/1,cold0,cleanup1. Log:`qwen-coordinated-short-benchmark.log`.

Contract v4 combined one owned16KiB activation/scales READ with a separate
immutable serial-ID0 coordinator. Per-packet command readback, counters,
ID0 idle and fences remain; full ALL-initiator/32-ID entry/exit proofs stay.
The strict debug image is byte-identical. Small/wide/vocabulary/shared-KV
tests all passed natively. Wide6144 rows17.49–17.52ms,vocabulary251.3–251.9ms.
`qwen-coordinated-v4-verify.log` passed all22 full-logit comparisons exactly,
197 resident tensors,119 math peers,clean END; diagnostic3 samples/8.806s
=0.340672tok/s, NOT a warm CPU speedup benchmark. Driver2d474e32,CLI d2430a0a,
full-model verifier aa7ec7d6 remain at unique rollback paths.

Native cohort profiling (`coordinated-v4-sweep.log`) identified about9ms of
payload PIO and3.4ms of activation staging in a17.4ms wide call.32–64 peers
slightly beat119 for that shape; fewer peers are NOT generally faster.

Contract v5 is a correct but SLOWER debug output transport, NOT a fast-target
promotion. It uses the existing distinct pinned/noncacheable16KiB owner,
finite poison replacement,frozen CPU snapshot,ordered host READ to144000,
then BME OFF before a separate immutable all-word original/readback compare.
Both chunks support8192 hidden outputs. No checksum or learned CPU sum.
Full physical counter/PC/nonce/source proof precedes reset; uncertainty
quarantines both owners and refuses reset/retry/restore. The executing
image's summary is fed into the production predicate in offline tests.
Every output-word corruption,paired flips,dropped/partial writes,stale
proofs,map/control/PCI/timeout faults and wrapping counters are tested.

Current driver8e251c8a,CLI33ed3b7b: native32/64-row tests PASS at10.2–10.6ms
and6144-row two-chunk tests PASS at20.61–20.67ms,all exact with clean END.
This is slower than v4's6.2ms/17.5ms; the extra host-controlled comparison
launch defeats the DMA saving. Assistant remains the earlier qualified
mode5 build,currently closed for exclusive diagnostics. No settings/model,
firmware/default boot,SD,CPU/RAM or unrelated dirty llama.cpp change.

Next: fuse owned output transfer and exact comparison into the already
running coordinator, so the fast path does not pay another host-controlled
launch. This is NOT implemented or qualified by this checkpoint.

### October17: persistent dispatcher / shared KV native checkpoint

Fast Qwen is **NOT achieved**. Installed Assistant remains the qualified
mode5 target measured at0.222tok/s below. The new explicit coordinated
candidate is native-qualified for primitives; a fresh full-Qwen/CPU32
comparison is running. No new model rate or Assistant promotion is claimed.

Private`54d9` mode8 uses master0 as a held-between-jobs dispatcher plus
persistent math peers1..119. The image verifies ALL prior idle sequences
before any launch, writes/readbacks shape-derived descriptors, issues fresh
doorbells, validates every peer's source/idle/counter/status/PC/sequence,
and gathers <=8192 hidden rows over device NoC. Large classifiers retain
full host logit readback. Exact Q8 arithmetic is unchanged. Host per-peer
launch/poll loops are removed. Explicit attention v3 shares the existing KV
image while peers wait in their immutable NoC-free mailbox loops. Below-tail
lazy upload accepts NEW pages only; existing learned weights cannot mutate.
Every master stop still needs physical counters/source/two-PC proof. END
live-audits all peers and proves ALL STOPs before ANY reset/restoration.

Metadata-only`54da` reports contract v2,119 math peers,KV-v3,immutable1,
gather8192 before BEGIN. v2 omits unused descriptor padding: every word
consumed by the actual image is staged/read back (64+p words,max732 bytes).
Legacy mode5/6/7 remain separate. No user code/address/route is accepted.

Evidence in `/boot/home/pioneer-resident-kv-20261017-a/`:

- `coordinated-matrix-native.log`: initial d45e1917,two persistent workers,
 changed vectors/all64 rows exact,7.328–7.363ms,clean END. Superseded driver.
- `coordinated-wide-native.log`: REJECTED119-worker attempt. The dispatcher
 completed, but CPU accounting omitted ONE gather read/peer and falsely
 rejected odd cohorts. No result/retry/reset/reload after cold latch.
 Normal shutdown/relay cold cycle/hash-verified SD-free handoff restored
 Haiku64 CPUs/full127.6GiB. The actual executed image now feeds the actual
 production acceptance test, reproducing the bug before correction:
 minimum11p reads for gathered output,10p ungathered; extra polls in pairs.
- `coordinated-wide-fixed-native.log`: corrected75d0d285,all6144 rows exact,
119 peers→one peer→119,19.890/7.280/19.936ms,clean END.
- `coordinated-vocabulary-native.log`: all151936 rows exact,119 peers,
 shrink/expand,252.939/252.527ms including full logits; reference outside
 clock; explicit END2/PCI-host1/1/cold0/cleanup1.
- `coordinated-attention-native.log`: all8 heads,layers0/27,n1..4,L2=0,
 interleaved changed-input matrix jobs exact,clean END.
- `coordinated-short-wide.log` / `coordinated-short-attention.log`: current
779fc4eb,contract v2,all numeric/cohort/attention/cleanup gates pass again.
6144-row calls18.495/18.497ms,32-row call5.824ms. NOT tokens/s.
- `qwen-coordinated-short-benchmark.log`: full-Qwen and three warm CPU32
 requests IN PROGRESS. Immutable executable`qwen-coordinated-short-8bc20db9`.
 Do not replace that executable or driver while it owns the card.

Current driver SHA256:
`779fc4eb906fb6bc36ce4df3311cebea383a43b5cf17ca18cc9e2aef2601d5ac`.
Current primitive CLI SHA256:
`2da6e2a636ae7433b9e3d064d346635fde3f755c183586ffd88a4789f4c5b7e2`.
Benchmark SHA256:
`8bc20db977021be18e0903f7f3cf72a19fd02bb30171ab5c67232d029eea8559`.
Corrected previous driver75d0d285 and all rollback copies remain available.
Pristine pinned GGML tests pass all five backend modes; real-client fake
syscalls cover the variable inline ABI, every proof field, no partial output,
close-only on errors and capability/contract refusal BEFORE BEGIN. Offline
controller/image tests are NOT native performance evidence. Host norms/RoPE/
KV mirror,197 matrix+28 attention calls/position,single-bank weights,PIO
logits and2GiB arena remain. Full device graph execution and27B/4-bit models
are separate unmet requirements; firmware/default boot/CPU/RAM are unchanged.

### October17: device-resident attention KV and wide SRAM candidate

Fast self-hosted Qwen is **still not achieved**. This follow-up replaces the
repeated host attention-history transfers, not the whole host-controlled
decoder. The fresh, uniquely named full-model/warm-CPU comparison passed
correctness but FAILED the CPU speedup gate. Do not promote primitive gains
to a practical-speed claim.

The separate `54d6` data-only ABI accepts eight queries plus ONE new K/V token:
8192+4096 bytes per layer, independent of prefix length. Its immutable 5200-byte
RV32IMF image reuses the existing exact attention arithmetic. For each of eight
groups it writes the new packed-half K/V to a private bank0 tail, fetches the
prefix over device NoC in <=16KiB bursts, runs both query heads, and collects
all outputs. One RISC start/stop covers the entire layer. All newly written K/V
words are compared on-card after the GDDR readback, and every fetched prefix
still passes the original finite-input checks. No host DMA or caller-supplied
address, program, routing coordinate or arbitrary command is added.

The28MiB tail is kernel-derived from the session arena, with disjoint fixed
slots for28 layers /8 groups /256 tokens / K and V. Reservation requires that
NONE of its pages was uploaded as weights, then refuses every overlapping
legacy/stream/bulk transfer. Below-tail lazy weight upload remains allowed.
Each layer records its initialized prefix length; gaps cannot be read. Host
prefix-coherence bookkeeping detects changed/restored conversations and
explicitly rebuilds the affected prefix on the card, never with CPU attention.
Qwen's host KV mirror, norms, RoPE and activation preprocessing still exist.

Every accepted result requires complete count/marker/two-PC/idle/source/hold
proofs, finite output and exact wrapping READ and WRITE counters. At n tokens:
read requests=16*ceil(n/64), read flits=64*n, acknowledged writes=16, write
flits=64, shared accepted requests=reads+16. A partial/unknown transfer or stop
quarantines both host allocations and the owned fleet. END retains the full
prepared-fleet live audit and reverse restoration. Under the exclusive team/FD
lock, master-only handoffs now check all peer ownership/quiescence metadata
and live-audit the master/shared PCIe source; each matrix still live-audits
every participating worker before launch and verifies every completion.

Capability`0x4000000` selects resident KV. The additional explicit v2 choice
requires`0x8000000` and`0x10000000`: phase-fenced, aligned64-bit SRAM DATA
transfers, with scalar32-bit alignment/tails and no over-read. All control,
NoC, reset, code upload/restore, ownership and completion fences remain on
their existing paths. The private HOST input page uses the already qualified
three-fence/full-readback staging policy. Legacy checked/Fast/Attention and
persistent qualification modes retain their previous paths. The intermediate
32-bit phase-fenced experiment was correct but had negligible native benefit.

Offline qualification includes actual linked RV32IMF/SFPU execution for all
eight groups, changed sequential tokens, layer27, n31/64/65/128/256, wrapping
NoC counters, invalid descriptors/values and uncertain transfers. It strictly
bounds the GDDR/L1 transactions. Controller tests include every3072 dropped
input store, both halves of wide transfers, all43 result-proof fields, prefix
restart/rewind/replay, all120 peer-state refusals and no reset/restore after
uncertainty. Real GGML integration tests cover the original84 public captures
in legacy, fleet, resident-KV and payload-block modes. The old attention image
still rebuilds byte-identically (`0d46bb39...`). The new image SHA256 is
`ac1c7a4a079fffb36223e1a4b4a10eaaf48505544c851b782109a6b214d4c05f`.

Native evidence in `/boot/home/pioneer-resident-kv-20261017-a/`:

- `full-qwen.log`: initial resident-KV driver43a1925e, all22 positions / all
 151936 logits identical to the independent CPU context (cosine1, L2=0),
 four greedy outputs,197 tensors /670482432 bytes /163692 pages. END2,
 PCI/host1/1, cold0. Three diagnostic generation samples took20.556989s
 (0.145936tok/s); this is NOT the unchanged Assistant's seven-sample rate.
- `native-optimized.log`: bounded master handoff, all eight groups on layers0
 and27 for n1..4, exact independent results, warm calls20–22ms, clean END.
- `native-blocks.log`: 32-bit block experiment, same attention proof plus
 interleaved changed-input two-worker64-row projections, all exact; clean END.
- `native-wide.log`: aligned64-bit candidate271c4abf, the same complete
 interleaving/numeric/cleanup checks, attention warm calls12–14ms. This is
 an attention-operator result, not tokens/s.
- `benchmark-wide.log`: fresh full-Qwen and repeated warm CPU32 comparison,
 binary`qwen-wide-275aa8b3`. All22 bootstrap positions /151936 logits exact,
 then three independent public requests of16 output tokens each. All quality
 and no-reupload checks passed, explicit END2/PCI-host1/1/cold0/cleanup1.
 Generation was0.209836 /0.208870 /0.211259tok/s versus CPU32's0.596272 /
 0.760404 /1.249012tok/s. P100A TTFT130.855 /135.595 /121.747s versus
 CPU1.908 /2.171 /2.012s. All three5%-speedup gates FAILED (test exit1),
 not a hardware-completion failure. Entire1111.791s qualifier: weights
 321.374s, matrix/stage/result591.626s, attention/stage/result107.742s,
 activation preprocessing2.573s. CPU-reference diagnostics outside timing.
- `assistant-wide-public.log`: actually installed service`d26de810...`,
 public question "What is the capital of France?" answered Paris, success1,
 CPU validation OFF / reference decodes0,26 device positions,728 attention
 calls, peak120 workers. Seven generation samples31.472s,0.222tok/s;
 cold request444.1s. This is about twice the old0.108111tok/s, NOT fast.
 The idle model then unloaded with explicit END2/PCI-host1/1/cold0/clean1.

`benchmark-optimized.log` is **REJECTED**: its executable was mistakenly
overwritten while the first warm request was running, then the test stopped
in the user debugger. It is not valid benchmark evidence. Only the identified
test team1472 was terminated; its driver-owned close passed the cold/ownership
gate without a reboot, reload or SD swap. Subsequent independent native
qualifications again proved explicit END2/PCI-host1/1/cold0. All further
native executables use new content-specific filenames; never SCP over an
executing file, even when a new build has the same original basename.

The installed Assistant opts Throughput into the new explicit path and requires
all capabilities before BEGIN; original debug targets remain. GUI/service/
Hardware/control were installed atomically with rollback copies, native
hardware-policy tests passed without settings writes, and host-only lifecycle
tests passed with zero device operations. No model, pinned CPU runtime,
firmware, default boot, CPU/RAM profile,
SD state or user's58-line llama.cpp work was changed. Practical speed, full
device-resident graph execution, sampling without full-logit PIO, multi-bank
weights and wider/4-bit Qwen models remain separate unmet requirements.

#### On-card dispatcher prerequisite: held-peer PC transport

The separate diagnostic`54d7`/capability`0x20000000` reads only the fixed
TRISC debug-PC register`ffb13140` of one kernel-selected peer1..119 through
master0's NoC. Both RISCs stay held; no peer write, core release/reset or
host DMA is performed by the diagnostic. Two4-byte reads must each prove
all four wrapping counters+1, all initiators/IDs idle, stable source/PCI/TLB,
and match four independent CPU PC samples. The sole local scratch word is
within the saved bulk lease. Unknown completion/hold/source quarantines the
session without retry/reset; a known data mismatch is not accepted.

Driver`103f16d99bc6bda9212b920c4ef6b3b38617744ffae235fdd4fd1d3e5424ada2`
was installed atomically ONLY AFTER Assistant's explicit clean unload and
closing its empty GUI/idle service. Loaded capability1073737727 confirmed
the asynchronous watcher, not merely the file hash. Qualifier
`peerpc-f00ef195` (SHA256
`f00ef1956133bfa696b29ac45c0664a16e3001fd2bb52aaa707378241ec53e3f`)
then passed ALL119 peers, both NoC reads / all CPU observations exact,
explicit END2/PCI-host1/1/cold0/cleanup1. Evidence`peer-pc-transport.log` in
the same native directory. ASan/UBSan tests cover every route, proof word,
counter wrap, dropped command/poison stores, changed/stale PC, mapping and
ownership faults. No on-card launch/polling dispatcher or speedup is claimed
yet; this proves its bounded read transport prerequisite.

#### Dispatcher images: native read-only qualification, math still offline

`blackhole_coordinator.cpp` now contains an immutable master0 dispatcher
candidate. Its separate code/stack lease is`6c000..70000`, disjoint from
the math/attention images. The proposed math path uses peers1..119 as
persistent row workers, on-card descriptor readback/monotonic doorbells,
full peer idle/two-PC/reset-state/source/counter/metadata checks, and NoC
gather into the already-owned master result SRAM for <=8192 rows. Larger
classifiers retain full host logit readback. This path is **OFFLINE ONLY**;
it is NOT selectable by any installed ioctl or Assistant target yet.

The coordinated worker variant of`blackhole_fleet_start.S` keeps the
qualified bulk Q8 math and records exact source/NoC-idle/counter proofs
before and after each job, with fresh math metadata and sequence before
publishing completion. Its helper avoids the math's71910..71a20 scratch.
All code/data fit the already-backed-up71000 page. The legacy persistent
image remains byte-identical and its native contract unchanged.

Actual linked-RISC/SFPU interpreter tests pass all119 routes, all supported
K, N4096 per worker, changed inputs without reset, independent global-row
IEEE results, up to119 workers, hidden-output gather through N8192, sequence
preflight BEFORE any launch, input/arena bounds, every proof-word corruption,
dropped descriptors/doorbells/commands, nonfinite output and uncertain NoC
completion. The first worker candidate fell into alignment padding after
completion; offline execution caught it, an explicit jump fixed it, and
it was never installed. Image hashes now:

- worker1508B:`437e56eea3362fbe6c495569bdc0943ee347eda6687c90a51e4bba30ebae35ea`;
- coordinator3440B:`359fb6e5e5af9fdcd248afe8e329391b4218ba4e87bc4ebc1fd67762b39bb100`.

Only the separate fixed READ-ONLY operation`54d8`/capability`0x40000000`
is exposed. The kernel fixes operation0, supplies all routes/expected held
PCs and a fresh per-lease nonce, verifies every staged word, and forbids
caller-supplied commands/programs. The master reads each peer's reset state
and PC twice (four4-byte reads/peer), never writes a peer or releases its
RISC. Acceptance requires the complete fresh physical-counter summary,
two master PCs, exact marker/count, independent CPU before/after peer PCs,
known stop, previous entry restoration, and complete code/stack restoration
at END. Unknown completion/source/hold means quarantine/no retry/reset.

Driver`69675ae3118da0972073d0abf8c0428c54438bd9e712726f5330bcaa994ccccc`
was installed atomically with no active owner; loaded capability2147479551
confirmed the watcher. Native qualifier`coord-probe-d3b9cbc0` (SHA256
`d3b9cbc09980fee7c87e1a1407f579e0c7f40558956f6384a48986f64e00b7b1`)
passed peers1,2,119,1 with fresh nonces1..4, master PC6c038, all
launch/complete/stop1/1/1, cold0. Explicit END2/PCI-host1/1/cleanup1.
Evidence`coordinator-readonly-native.log` in the same native directory.
ASan/UBSan controller tests cover code AND stack restoration, stale summary/
nonce, all result proof words, source/map/PC/NoC/stop faults and no reset or
restoration after an unknown completion. This is a working native control
channel, NOT an improved Qwen timing. Matrix-dispatch driver integration,
native persistent-worker qualification, shared-KV handoff and full-model
performance qualification remain to be implemented.

### October17: persistent-worker foundation, NOT usable inference speed yet

The requested fast self-hosted Qwen decoder is **not finished**. The installed
Assistant's result at that checkpoint was0.108111tokens/s, which is unusable.
The following work qualifies a separate matrix runtime; it does not change
Assistant hardware targets, establish a new model tokens/s result, or qualify
all Qwen models. More small primitive gains are not the acceptance criterion.

New explicit ioctl`54d5` has bounded inline operands/results, no supplied code,
coordinates, host/device pointers or arbitrary NoC commands. Version1 selects
the existing exact bulk math, version2 a separate direct-instruction image.
Capabilities`0x1000000`/`0x2000000` distinguish these choices. The first runtime
selection fixes the session's format; all weights must be uploaded BEFORE it.
Later weight mutation/other compute ABIs are refused, not silently retried.

Workers boot once and wait in a local mailbox loop between math jobs. The
trusted bulk math's final drained park branches to a private completion
dispatcher instead of resetting the coprocessor. Mailboxes at`71300` contain
issued/finished monotonic sequence numbers, STOP request and ready marker.
Every accepted job still needs all participating workers' new-sequence,
complete-row/tile, finite-result, exact NoC-counter, source/idle and two-PC
proofs. The PC loop range alone is NEVER a completion proof. Inputs/scales
remain immutable until all participants finish; nonparticipating workers
receive no command. A complete-fleet audit is required for final teardown.

The result ABI deliberately reports idle workers and`stopped=0`: completed
math is NOT a stopped RISC. END sends a separate STOP command and proves
every worker at the fixed STOP park before resetting even the first peer.
Only then may the existing reverse-order L1/code/NIU/PCI restoration run.
Missing/partial job or STOP proofs quarantine the whole owned fleet and both
host allocations. There is no forced reset, replay or reload recovery.

The separate direct-instruction candidate uses Tenstorrent's documented
[rotate-left-two `.ttinsn` encoding](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/BabyRISCV/PushTensixInstruction.md).
It keeps exact compensated IEEE arithmetic and existing NoC/completion PCs;
the static SFPU sequence executes directly instead of being copied instruction
by instruction into the FIFO. Conservative SFPNOP gaps follow FP arithmetic,
including the documented scoreboard exceptions. No replay/SFPLOADMACRO,
firmware, clock or configuration-policy change is used. Measured gains from
this change ALONE are small; it is not a practical-inference speed result.

Persistent sessions additionally use boundary-fenced fills/full scans of the
ordinary private noncacheable HOST input page. All1024 words are still read
back. Only three phase fences replace per-word host-RAM fences; PCI, MMIO,
NoC, device L1, DMA ownership and completion ordering are unchanged. Legacy
checked/Assistant modes retain their original per-word host-page path.

The first native candidate`c8f4054b...` correctly latched cold recovery when
its early marker sample was zero even though a later sequence/PC sample had
already observed completion. That result was rejected. The fix snapshots ALL
math metadata AFTER the idle/sequence proof; a dedicated regression reproduces
the earlier sample. No subsequent card operation/reset/reload was performed
after the latch. Ordinary Haiku shutdown, relay cold-off/on, default NVMe
Linux and the unchanged hash-pinned SD-free full-memory profile recovered
Haiku with64 CPUs/127.62GiB usable RAM. No SD, firmware or boot-file edit.

Native real-model tests subsequently passed independent IEEE FMA comparisons
on changed public input vectors, all participating-worker proofs, zero
relaunches after the first job, and explicit END2/PCI-host1/1/cold0:

| bounded projection | workers | representative warm whole operation |
| --- | ---: | ---: |
| Qwen0.6B K1024/N64, persistent bulk |2 |4.595ms |
| same, direct instructions only |2 |4.600ms (no material gain) |
| same, direct + host staging |2 |4.317ms |
| real Qwen1.7B FFN-down K6144/N64 |2 |7.239ms |
| full Qwen0.6B FFN-down K3072/N1024 |2 |18.213ms |
| Qwen0.6B classifier K1024/N3840 |120 |118.692ms (earlier reset-per-job192.122ms) |
| full Qwen0.6B classifier K1024/N151936 |120 |563.822–566.429ms |

These include guards, staging, control/proof and PIO; CPU golden computation
and first-use weight upload/code preparation are outside each warm call.
They are small diagnostic samples, not matched full-model benchmarks.
All151936 full-classifier rows were independently IEEE-bit-identical on four
changed inputs; each warm call had zero worker relaunches and the final
STOP/END was clean. The CPU-wall profile isolates about449–452ms in result
PIO after computation/proof. Thus even this one operator's host readback
alone prevents useful inference speed; changing worker count or arithmetic
feed cannot remove that cost. Device/backend sampling is an architectural
requirement, not an optional tuning detail.
The1.7B row qualifies only that projection, not full1.7B inference. All120
workers compute real classifier rows, but host dispatch still serializes
per-worker commands/collection; sustained all-core utilization is unproved.

Final driver candidate:
`d61a86ea399e0705c8d01b0f924531ae977c2f2babcf2438b7b9957f6980f0f8`,
capabilities`0x3ffefff`; native diagnostic:
`34ae6656154ffbfa230b608ae01111d196874a4f106b9261e7a10392d692c027`.
Persistent prelude SHA256:
`c003c3831db9beaa96959274f0b695087efbb88216c9853032aae3413048f8dc`;
direct math SHA256:
`6d138ce007494af107d212b078c4fe3c1d042790585e147f1539b8f7327c8595`.
Evidence/backups are in`/boot/home/pioneer-persistent-20261017-a/`, including
the original installed`49e082c5...`, qualified persistent`37033b93...` and
qualified inline`d76e0746...` drivers. The original failed trial is retained,
not relabeled a pass. The Assistant binaries and model files are unchanged.
After the final clean END, the unchanged Assistant/hardware dialog was
reopened: service team570,ready1,busy0,one resident host model, saved
`p100a-qwen-throughput` selection and32-thread CPU preference; chat/draft
counts0/0. Device weights are cold and upload on first reply. No new public
request or full-model performance measurement was run. As usual, normally
unload and prove clean cleanup before another independent card experiment.
All22 scoped files were synchronized/checksummed to the plain build mirror;
the unrelated dirty llama.cpp submodule remains excluded.

ASan/UBSan checks execute the actually assembled persistent/direct images,
all120 fixed coordinates, changed inputs across jobs WITHOUT resetting
RISC/coprocessor state, every supported K and4096 local rows. Controller tests
cover sequence/PC/counter/proof corruption, worker-count changes, immutable
image choice, transient early-marker sampling, partial jobs and STOPs, all
four-worker mapping/PCI/source failures, no reset before all STOP proofs,
and exact restoration. Host staging tests reject each of1024 dropped stores
and1024 corrupted readback words. Existing fleet, dual/reuse, bulk transport,
shared attention, profile, tensor-cache and84 public GGML attention fixtures
pass; the dirty user llama.cpp submodule is not rebuilt or modified.

Next architectural gate: device-owned attention/KV prefixes (upload only a
new token, not every prior token for every layer/head), a device-side fleet
dispatcher/collector, then fused decoder intermediates and device sampling.
The pinned inference library has an experimental backend-sampling interface;
using that, rather than fabricating a sparse CPU logit vector, could avoid
full-vocabulary PIO. The matrix-only persistent ABI must NOT be exposed as a
new fast Assistant mode until compatible attention/lifetime handling and
end-to-end quality/speed tests are complete. Multi-bank weight placement and
larger/mixed-quantization models remain separate unqualified work.

### October16: separate multicore throughput path

The new explicit `p100a-qwen-throughput` path keeps the earlier checked,
Fast and Attention modes. It does not create/decode an independent CPU model
in normal use. All device ownership, complete-row coverage, finite-output,
counter/park/stop and cold-recovery gates remain. This is still a hybrid,
not a device-resident decoder or a claim that every Qwen model is supported.

Native read-only identity/source/idle checks passed on all120 fixed compute
cores, including the harvested die's logical-to-physical X translation.
Native real-Qwen projection tests then passed on1,2,4 and120 workers, with
zero independent IEEE-FMA bit differences and END2/PCI1/host1/cold0. The
120-worker3840-row test covered all workers on three changed public vectors.
The immutable existing compensated bulk math image is unchanged. A separately
saved RV32I prelude at`0x71000` lets peers pull the16KiB input/scales region
from owned worker0 over NoC and poisons each local output before computation.
It issues no host DMA. Its binary SHA256 is
`a6bd488515f395cab6ac81961af03dbdc65aa708bed718b4fe2ccab1462f7fae`.

The version2 whole-projection ABI has bounded inline input/output, no caller
pointer/coordinate/program/device address, and copies only the active rows.
It can cover the whole151936-row vocabulary with one call, instead of38.
An additional grouped-attention ABI executes all eight existing head-group
tiles in one call and publishes no output unless every proof succeeds.
Qwen0.6B therefore uses197 matrix+28 attention calls per position, versus
234+224 previously. CPU norms/RoPE/activation quantization, KV storage/prefix
staging and full-vocabulary result readback remain;225 calls is NOT zero.
Small projections use fewer workers; more workers alone can be slower.

The initial native all-core candidate exposed redundant120-core audits on
every matrix after attention was prepared. Its early full-model positions
were CPU-identical but took about17s each, worse than the earlier~11s card
attention path. It is not a performance success. The revised controller
audits the whole prepared fleet at actual program handoffs and teardown,
while each projection retains every participating worker's live local and
completion proofs. Shared PCI/PCIe-source state is checked at phase boundaries
instead of resampled for each worker. Legacy debug paths are unchanged.
The whole projection/attention graph passed all22 independent full-vocabulary
CPU logit checks exactly (cosine1/L2=0), four greedy tokens,4334 matrices,
616 attention ops/4928 tiles and clean END. This initial controller took
769.734s total and0.051809tokens/s warm; it was rejected as a speed result.

The revised driver also allocates/copies the exact bounded inline request,
not a new maximum1MiB heap object for every small projection. Native changed-
vector tests passed again with zero bit differences and clean END:

| revised projection | workers | warm whole operation |
| --- | ---: | ---: |
| K1024/N64 |2 |5.949ms (initial candidate21.683ms) |
| K1024/N3840 |120 |192.122ms (initial candidate267.934ms) |
| real Qwen1.7B FFN-down K6144/N64 |2 |9.197ms |

The1.7B result qualifies only that bounded projection, NOT full1.7B generation
or its Assistant catalog entry. All120 workers participate in large matrices,
but launch/stop and handoff are still host-driven and serialize significant
control traffic. A persistent device dispatcher/barrier, resident KV/decoder
intermediates, multi-bank weights and reduced vocabulary readback remain
necessary before practical acceleration/zero per-op host round trips can be
claimed. The installed Assistant's end-to-end result is recorded separately.

Installed Assistant qualification completed at00:17UTC October17 (October16
local). The same public capital-of-France request produced the correct eight-
token answer ending in`**Paris**`, success1,26 device positions,728 card
attention ops, peak120 workers, CPU validation OFF and zero reference decodes.
Seven generated-input samples took64.748s (0.108111tokens/s), versus76.595s
(0.091390tokens/s) in the earlier installed Attention path: about18.3% higher
diagnostic generation throughput. Total first reply552.1s versus613.3s. This
is a small public-request diagnostic, NOT a matched CPU benchmark, useful
accelerator performance or proof that all host round trips are eliminated.

The earlier checked/Fast/Attention targets remain. The native hardware
policy test passed; the new target is advertised only with both new driver
capabilities. The actual GUI shows the correct Throughput label and button,
enables Apply when idle, disables the CPU-thread control for this target and
preserves the saved32-thread CPU preference. Hardware switch and unload were
rejected while the request was busy. Host-only tests also covered transitions
between Throughput/Attention/Fast/checked contexts with zero device opens or
decodes. GUI chat/draft lengths remained0/0; no existing chat text was read.
At that checkpoint the service was ready/idle with its exclusive session
warm. **Do not probe/reset/reload independently: normally unload and prove
clean END before the next hardware experiment.**

Installed artifacts, with earlier binaries/settings backed up in the native
evidence directory:

- driver:`49e082c5c551688e774c926fc497cd408f53c958439a39656ea239339df84f1d`,
  capabilities`0xffefff`;
- GUI:`dd949f94dd921b1ece355e74c24473449615b47aa34e04ba3dc1951ac999770d`;
- service:`bfa01499703d566d32ecc0a2257985f3eb553052875901a121ed61083f9a52f7`;
- hardware app:`ced56545865ee460d4da30d68e0dab7871322ac027904f91ff25960d463ae1db`;
- control:`6cd158ab8040f1fe6f0f962198eb352cc4f1f2379e807d3090b974b2e3236848`.

Only the final GUI labels were refreshed after the public test; the tested
service and its warm model were not restarted. Evidence includes
`assistant-throughput-public.log`, `gui-throughput.log`, `fleet-opt-*.log`,
`full-qwen-fleet.log` and`host-modes.log`, with ignored local copies under
`generated.pioneer/tenstorrent/fleet-20261016/`. All38 scoped source/test/doc
files were synced/checksummed to the plain build mirror. No power cycle,
SD change, firmware/kernel/boot edit or model download was needed.

Offline ASan/UBSan checks cover120 unique physical workers, all dropped TLB
writes, assembled input fetch/output poison with all supported K widths and
4096 rows, exact arithmetic, counter wrap and fault parking. Runtime tests
cover every worker's completion/PC/counter/stop faults, every four-worker
mapping failure, shared PCI/source failures, global quarantine and no peer
reset after uncertainty. Real-client synthetic-syscall tests reject all
worker/head-group proof faults without partial results, retry or fallback;
missing capabilities refuse before BEGIN. Pinned ggml tests pass both paths
against84 corrected public attention captures. The earlier pre-precision
`public-fixtures-2tokens` captures are not the corrected goldens and fail
the unchanged legacy path too; use`precision-public/public-fixtures`.

Evidence is isolated in`/boot/home/pioneer-fleet-20261016-a/`, with the proven
bulk driver backed up as`original-driver-f29ab031`. Model files, firmware,
kernel, SD-free boot configuration and the user's dirty llama.cpp submodule
are unchanged. Native runtime libraries remain the pinned CPU-baseline build.

### Earlier two-core attention/loading checkpoint

Earlier October16 attention checkpoint: the installed Assistant can execute
Qwen3-0.6B Q8_0 learned matrices AND attention on the P100A. QK/PV run on the
SFPU; reductions and softmax run on card TRISC0. The complete standalone
22-position test has CPU-identical logits and greedy tokens, and the
installed zero-reference-decode Assistant answered the public capital-of-
France question correctly. Shared matrix/attention ownership and clean END
are integrated. Host KV storage/prefix staging remain. Its approximately
0.093 tokens/s public-request diagnostic is **not usable acceleration or a
CPU speedup**; practical generation performance is still unfinished.

Performance work now has an opt-in 1..4-page owned bulk uploader. A native
equal-byte, alternating-order transfer comparison measured 3.221x over the
existing 4KiB stream, with every uploaded word and untouched neighbour
independently PIO-read, exact six-request counters and clean dual-host-owner
release. This is a small transfer benchmark, **not** a full-model loading or
generation speedup claim. Full-model loading/quality qualification passed;
warm generation remained0.094529tokens/s. Details and the separate inference
bottleneck measurements follow below.

Earlier October16 matrix-only checkpoint: standalone Qwen3-0.6B Q8_0
passes its full22-position bootstrap under Haiku on two fixed P100A cores,
with all197 learned matrices, full-vocabulary checks at every position and four
CPU-identical greedy reply tokens. Compensated SFPU accumulation fixed the
reproduced rounding drift. This is **correctness, not speed qualification**:
the three-sample warm diagnostic with exact activation reuse is0.148549tokens/s
(prior uncached two-core0.140525; one-core exact0.115836), not a matched
benchmark or CPU speedup. Assistant now offers this runtime as the explicit
experimental `p100a-qwen` hardware option for the pinned 0.6B model only;
the separate `p100a-qwen-fast` target skips the independent CPU reference,
with a native zero-reference-decode reply and about 0.149 tokens/s in a
short generation diagnostic, not a CPU-beating performance qualification.
27B/mixed quantization and practical acceleration still require further work.
On October16 the user explicitly approved the activation-reuse kernel test,
including possible freeze/power-cycle recovery. The replacement driver is now
installed with the proven two-core driver preserved for rollback. Small and
full-size native projections AND full Qwen generation pass their exact-bit,
reuse, ownership, logit/token and cleanup checks. See the records below.

The following early milestones are chronological; their narrower intermediate
limitations do not supersede the current Qwen correctness/integration results.

### October16 performance investigation: measured limits, not peak claims

The existing exact two-core path is a bring-up runtime, not a throughput-
optimized decoder. It performs234 matrix ioctls and224 grouped-head attention
ioctls per position. At that checkpoint only two compute workers were qualified, weights
use one GDDR bank, and the full attention prefix is host-staged. Neither
"on card" nor disabling independent CPU validation implies fast inference.
No evidence currently establishes1000 tokens/s, support for every Qwen
architecture/quantization, or even faster-than-CPU generation.

Native projection profiles with the original staging path, exact CPU FMA
bits and both independent stop/completion proofs passed:

| Qwen0.6B projection | Whole operation | Compute plus proof | Result PIO |
| --- | ---: | ---: | ---: |
| query1024→64 |13918us |4079us |222us |
| query1024→2048 |26046us |10043us |6095us |
| FFN down3072→1024 |29027us |13864us |3066us |

These are CPU-wall phase profiles, including guards/polling, not bare-device
arithmetic timings. They motivate fewer host round trips, bulk/result
transport, resident KV and larger decoder execution units before broadening
model support. The CPU baseline and independent quality gates must remain
unchanged; faster loading alone does not satisfy the generation objective.

The initial bulk upload driver SHA is
`f29ab0314165fd5e4fc9fe089662af5d0bf9df515b24fe53d2f5c7c695f08c8b`,
capabilities`0x3fefff`. Its diagnostic SHA is
`9c5aec795967a3819f41aa4b426e7f5032764acf73b0d796bfc50b11c8add65d`.
Native evidence is retained at`/boot/home/pioneer-bulk-upload-20261016-a/`:
`bulk-compare.log`, `profile-q64.log`, `profile-q2048.log`,
`profile-down1024.log`. Ten matched warm transfers total163840bytes per path:
legacy0.209904s, bulk0.065166s. One-time allocation/L1 snapshot is excluded.
All1/2/3/4-page shapes, nonzero offsets, foreign FD/inherited-team denial,
invalid request fields and clean END passed. The original upload remains.

The new controller saves/restores two fixed16KiB L1 regions, privately owns
a contiguous high-address noncacheable16KiB host allocation, checks all four
physical pages/no input alias and retains both host owners after uncertainty.
It uses the existing six-step poison/full CPU visibility/ordered-read proof,
with exact1/(64×pages)/1/1 counters. Only host RAM fill/scan fences move to
block boundaries; MMIO fences and transaction/route/PCI checks remain.
Offline tests cover all8192 dropped host stores,4096 late-word corruptions,
8192 dropped scratch restoration writes, counter wrap and transfer/map/PCI
failures under ASan/UBSan. A synchronous tensor-span adapter is explicitly
opt-in at the native client; legacy transports keep their old page path.

Full-model bulk loading passed with verifier SHA
`a5a5f185664d07178102586f3c4dca5921a5b46c2ec16324159aa588a1380406`:
all197 learned tensors/670482432bytes/163692pages, all22 full-vocabulary
logit vectors CPU-identical (cosine1/L2=0), four identical greedy output
tokens,616 attention ops/4928 tiles/5148 matrix calls and END2/PCI1/host1/
cold0. Total587.490s; weight preparation/upload/cache350.930330s versus
876.625116s previously, approximately2.50x faster. Matrix147.476447s,
attention60.832223s, activation preprocessing0.366104s. The three generated-
input warm samples took31.736450s (0.094529tokens/s): **generation speed did
not improve**. Evidence:`full-qwen-bulk.log` in the native directory above
and ignored local`generated.pioneer/tenstorrent/bulk-upload-20261016/`.

A separate block-boundary fence experiment for the old4KiB runtime input
page passed exact outputs but did not consistently accelerate larger shapes:
query2048 was26276us versus26046us, FFN-down1024 was29562us versus29027us.
The64-output sample was11578us, but compute/proof varied too. This is not a
matched speedup qualification. The experiment was removed; the fully
qualified`f29ab031` driver restored normally after clean session ends.
The rejected candidate SHA
`317d247aa31e39958619edf788132837e59313629404fda5fec23b7951e46cb2`
and its`block-profile-*.log` files remain in the isolated native test folder.

Assistant now prefers bulk loading only when its capability is advertised
at the initial open, before BEGIN. A legacy driver is selected up front as
legacy; **no transfer failure can trigger fallback/retry**. The real client
against synthetic syscalls tests both modes, every38 proof-field corruption,
request failure, counter wrap and capability absence. Native idle mode-switch
and malformed/unpinned request tests pass in the updated installed build.
The installed public-generation smoke passed:`The capital of France is
**Paris**`,8 output tokens/26 positions/728 card attention ops, CPU validation
OFF/reference decodes0, success1/exit0. First reply613.3s versus1117.4s in
the previous attention build. Warm generation7 samples/76.595s=0.091tokens/s
is unchanged within this short diagnostic; this is NOT a generation speedup.
The service is left ready/idle with its device weights resident, selected
`p100a-qwen-attention`, CPU-thread preference32 and empty GUI chat/draft.

Installed Assistant GUI SHA
`329f65d6bf9dbe334e4f8e902a005492d2565badfc02688d49914942e571dc46`;
service SHA
`89af00519938cad54cc9d9250ef73379d7a32c47d3c177ae7fb9e6b53b00415b`;
Hardware SHA
`8c30ed6b2a0e56b67e3b3e9f911007edced9584f28b96124d905b6ff3a9ad5cd`.
Native`assistant-bulk-public.log` and`gui-bulk.log` are retained alongside
individual original-binary backups. No firmware/clock/boot/SD changes, forced
driver unload or cold-latch bypass. The RAM/CPU port and user llama edits
were not changed.

Read-only1.7B inventory (not an executable-card qualification) confirms
28 layers,2048 embeddings,6144 FFN,16 query/8 KV heads and width128. Its197
Q8 matrices would occupy1935507456 packed bytes, within the existing2GiB
arena; matrix shapes are2048×151936,2048×1024,2048×2048,6144×2048 and
2048×6144. This does not check signed weight values, full graph compatibility
or correctness, so the Assistant compatibility list is unchanged. The bounded
header parser now reports Qwen3 metadata/matrix inventory without printing or
evaluating remote chat templates. Q4/mixed formats and the27B architecture
still need independent implementations and qualification.

For interpreting performance targets, Tenstorrent distinguishes per-user
generation from aggregate batch throughput in its
[benchmark definitions](https://github.com/tenstorrent/tt-metal/blob/main/README.md).
The [board specification](https://docs.tenstorrent.com/aibs/blackhole/index.html)
lists120 Tensix cores and448GB/s memory bandwidth for p100a; our qualified
two-worker, single-bank prototype does not approach that hardware utilization.
No vendor peak or multi-user throughput figure is an achieved single-chat
Qwen0.6B result for this Haiku port.

Source/feasibility investigation started 2026-09-15. On October 15,
TinyStories-260K generated eight verified tokens under Haiku with every
learned linear projection/classifier computed on the P100A: `Once upon a
time, there was a`. All CPU reference token choices matched; 8224 device
tiles completed/stopped/restored in 123.050 seconds. This is an experimental
fixed-shape hybrid runtime: the host handles norms, attention/KV, activations,
embedding lookup and partial sums. **Full Qwen3.8-27B inference is not working
yet.** Its real Q4_K/Q8_0/Q6_K projection samples now pass independently.
Haiku retains 64 CPUs and 127.62 GiB usable RAM. See the live records below.
The native Assistant now integrates this fixed-shape runtime as explicitly
labeled experimental story completion: a real GUI prompt `Once` produced
eight output tokens, ` upon a time, there was a little`, using 9,252 card
tiles across nine evaluated positions. The model manager installs both pinned
data files from Hugging Face; hardware selection commits the compatible
model/backend pair. Stop is batch-boundary-safe and never falls back to CPU.
An initial DRAM-route preflight refusal and an exception-path failure were
observed; explicit error returns replaced exception unwinding, without
relaxing device guards. This remains experimental, not full Qwen support.
See [Assistant details and validation](../../src/apps/assistant/README.md#p100a-integration-verification-on-pioneer-2026-10-15).
The next performance prerequisite now passes on the real card: the sole
compute tile's NoC0 can read a fixed 64-byte GDDR sample into its own L1,
with a poisoned destination, independent host comparison and complete
L1/NIU/TLB/PCI rollback. This is **not** a resident Qwen backend, a bandwidth
measurement, or a faster-than-CPU result. The Assistant's supported modes
have not changed. See the October 15 device-residency record below.
The next live gate also passed twice: TRISC0 fetched resident GDDR inputs
itself and computed two dense BF16 matrix fixtures with zero numerical
mismatches. No host NoC doorbell write or input write between fixtures was
allowed. This remains a fixed one-tile proof, not Qwen inference or a speedup.
The same card now also passes full on-card accumulation for fixed 16-output
GEMV fixtures with 1,024, 5,120 and 6,144 inputs. It no longer needs a host
request/reset or a CPU partial sum for each 16-element input block. Model
weights in those GEMV fixtures are still re-uploaded. A separate exclusive
storage session now retains model data across requests. It is connected to a
new bounded matrix-vector runtime: real Qwen query and FFN-down matrices
run with retained weights and separate vectors. Full Qwen chat remains
CPU-only at that stage; a standalone model-graph backend now works as above,
but an actual CPU speedup is still missing.
Dedicated, owned high-address host buffers now pass 4 KiB transfers in both
directions. Card-to-host writes additionally require CPU-visible payload and
distinct poisoned-L1 readback proof before releasing their buffer. These
are reversible transfer primitives, not a persistent Qwen backend or a
tokens-per-second improvement.
As of October 14,
Haiku enumerates the installed P100A through the SD-free RAM boot path,
its optional read-only diagnostic works, and a controlled temporary BAR
mapping reads the card's NoC identification register successfully. A fixed
read-only NoC window also reads ARC boot/error status and telemetry pointers.
ARC boot status is stable at `0x5`, and the sampled error-status word is zero.
Bounded, whitelisted telemetry reads now work too: firmware bundle
`18.5.0` (raw suffix `01`), ASIC about 46.6 C, fan 1,882 RPM, seven enabled
GDDR channels with training complete and no reported uncorrected errors.
Repeated samples show an advancing firmware heartbeat. This is a firmware
health snapshot, not a memory stress test or working accelerator runtime.
One stopped Tensix tile also passes a reversible 64-byte L1 pattern test,
including verification of the original bytes after restoration.
The tests restore the card and bridge configuration exactly. The initial
experiments never enabled DMA; later fixed owned-page transfers now do so
only for the duration of the verified operation described below. There is
no general-purpose model compute runtime yet. See the
live-test records below. Samsung's installed OS package, card firmware,
and default Linux boot remain unchanged.

A first optional Haiku diagnostic add-on is now in
`src/add-ons/kernel/drivers/misc/tenstorrent_pci_diag.c`. It binds only PCI
`1e52:b140` and exposes one read-only, versioned snapshot through
`/dev/misc/tenstorrent_pci_diag`. The matching
`tools/pioneer/tenstorrent_haiku_diag.c` prints the snapshot. The kernel
add-on compiled cleanly with `-Wall -Wextra -Werror` and linked against the
existing kernel build. The CLI also compiled and linked against the existing
Haiku runtime. Both used `/Volumes/HaikuBuildLocal`'s RISC-V compiler and are
saved under ignored `generated.pioneer/tenstorrent/haiku-build/`. Jam was
missing for that check, so this was a direct compile/link rather than an
integrated Haiku build. The two artifacts are staged in a separate test BFS filesystem and a
separate full-disk image based on the existing 2026-09-08 `haiku-mmc.image`.
The full image retains its original boot partition; its test BFS partition
contains the add-on at
`system/non-packaged/add-ons/kernel/drivers/dev/tenstorrent_pci_diag` and the
CLI at `system/non-packaged/bin/tenstorrent_haiku_diag`. A read-only BFS
consistency check found no allocation errors across 249 nodes. The source
image and boot partition hashes were verified unchanged. This is a manually
assembled test image, not a fresh integrated build of current source. Its
300 MiB fresh-install filesystem must not overwrite the persistent 4 GiB SD.
Those manually assembled SD images were not deployed. The October 14 live
tests below instead deploy the optional add-on/CLI on Samsung while booting
through the private RAM handoff.
The snapshot reports identity, command/status and raw BAR configuration,
but deliberately does not size BARs, infer usable mappings, touch MMIO,
enable bus mastering, use DMA, or flash/reset the card. The legacy `pci_info`
arrays hold 32-bit words: valid 64-bit BARs use adjacent entries for the high
address and high size. Do not truncate to one element. Neither these cached
values nor raw BAR registers substitute for validated resource allocation
and host-address translation.

On October 12, the unchanged SD booted normally after the P100A was physically
removed. With the card installed, UEFI PCI resource allocation failed and
Haiku rejected the USB controller's zero BAR; an xHCI error-cleanup bug then
panicked. See [the SD/xHCI recovery record](XHCI_RECOVERY_20261012.md).
Jam has now been restored for the USB driver build. The cleanup fix is not
a PCI-allocation fix or evidence of a functioning accelerator. Keep the P100A
out while establishing the Haiku port baseline.

### October 16: native attention kernel qualification (not Assistant integration)

The fixed kernel in `blackhole_attention.cpp` handles two F32 query heads
sharing one F16 K/V head, width128, one query position and a causal prefix
of1..256 keys. Eight such tiles cover Qwen3-0.6B's16Q/8KV heads. All QK
multiplications and weighted-value accumulation use the32-lane Tensix SFPU;
dot reductions, stable max-subtracted softmax and normalization use the
P100A's TRISC0. No host attention result is substituted. The compensated
SFPU sequence preserves the existing partial-FMA correctness constraint.
The first implementation stages the entire used K/V prefix through checked
PIO. It does **not** retain the KV cache in GDDR and is not a speed win.

The instruction contract was checked against official Tenstorrent ISA
revision `8a34e25e04d1ad7dd129ac9231e126b85923fac3`, including
[BabyRISCV FP limitations](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/TensixTile/BabyRISCV/InstructionSet.md).
The image uses no floating divide/sqrt, unsupported rounding-mode assumption,
host pointers, NoC commands, firmware services or allocation. Immutable
fixed-address code and separately saved stack/data regions are bounded by
the linker and actual-image interpreter. Observed stack use is1136/8192bytes.

`TT_PCI_PROBE_PIONEER_ATTENTION_TILE_TEST` is a separate root-only operation,
restricted to an exclusive **dedicated qualification session**. It refuses
mixing with the matrix runtime or uploaded model pages. The driver accepts
only finite fixed-shape inputs, not programs/addresses/coordinates, verifies
all staged bytes and poisoned output, and requires exact marker, count,
two parked-PC samples and unchanged NoC counters before stopping. Missing
completion/stop proof quarantines the host page and latches cold recovery;
no reset/retry/module reload may bypass it. Clean END verifies original
boot/code/descriptor/stack/data and restores the resident lease/PCI route.

Offline ASan/UBSan checks pass elementary math, all finite half patterns,
independent double-precision attention, n1..256 and all public fixtures.
The actual linked RV32IMF bytes execute in a restricted memory/MMIO model
using Blackhole's partial-FMA arithmetic, with arithmetic/lane/input/timeout
fault injection. Maximum synthetic relative-L2 versus double is1.614e-6.
The policy suite drops all descriptor/status/reset/launch/stop writes plus
sampled payload writes (24 cases) and checks restoration/uncertainty faults.
Existing matvec, second-core, dual-core and activation-reuse suites pass.

Native Haiku qualification on October16, all with END state2, PCI/host1/1,
cold0:

- One key: exact output; first-call ioctl wall0.141907s.
- Three keys: relative-L2 versus scalar FP32 `8.24314812e-8`;
  first-call wall0.140170s.
- 256 keys: relative-L2 `2.22631067e-6`; first-call wall0.255883s.
- All56 captured fixed-public-`Hello` Qwen layer/position fixtures, eight
  tiles each:448 operations, strict FP32-reference and captured-ggml-F16
  gates passed. Total ioctl wall3.004265s, whole test3.251825s. These include
  staging/guards, not device-only timings or inference tokens/second.

Build/test commands:

```sh
sh tools/pioneer/build_attention_image.sh CONFIGURED_BUILD FRESH_ABSOLUTE_OUTPUT
sh tools/pioneer/tests/run_pioneer_attention_math_tests.sh [PUBLIC_FIXTURES...]
sh tools/pioneer/tests/run_pioneer_attention_image_tests.sh IMAGE_OUTPUT [PUBLIC_FIXTURES...]
sh tools/pioneer/tests/run_pioneer_attention_tile_policy_tests.sh
```

Explicit native tool options are `--check-support`, `--run-synthetic 1..256`
and `--run-public-fixtures FILE...`; do not run them while Assistant owns
the P100A. The CPU-only shape/capture tool links no Pioneer backend and
captures only the fixed public `Hello` test, never user conversations.

Installed qualification driver SHA256:
`1cf6da7d8397c6c416318b641fb96ab763877ab36f23b1062884b44387bd6e9c`.
Native probe SHA256:
`66bbc891b65e7bc45e41565362fea5a580a21bc0b2a2604d010f953a2c7c4a3d`.
TRISC image SHA256:
`d9dfbf093a23b0beb00e2ea49220af6457fc8286e3c3e868b3f2bc7a3b94786b`.
Evidence/backup: `/boot/home/pioneer-attention-20261016/tile-a/`;
original proven driver preserved as `original-driver-fd1672b3`.
Local ignored evidence: `generated.pioneer/tenstorrent/attention-20261016/`.
Assistant was normally unloaded before device tests; chat/model selection
were retained. Ordinary atomic driver update was discovered after a delay;
no reboot, firmware/boot/SD change, forced module unload or cold-latch bypass
was needed. That first qualification alone did not prove full-model attention.

### October 16: shared attention and CPU-compatible precision

The subsequent shared operation `TT_PCI_PROBE_PIONEER_ATTENTION_TILE_SHARED`
requires the exact two-worker/reused-activation matrix mode and uploaded
weights. Before changing the primary worker's fixed boot entry, both workers
must be held and their PCI/NoC state unchanged. The secondary entry remains
the matrix program. Before the next matrix call the primary entry is restored.
The attention code/data reservations do not overlap matrix code/bulk buffers.
END proves both workers held before restoring attention state, then closes the
matrix leases in order. Mapping, entry-readback, completion or stop uncertainty
quarantines both workers: no reset/retry, forced reload or cold-latch bypass.

The first full Qwen experiment stopped at position1: logit cosine0.996853787,
relative-L2 0.103737042, despite the same argmax. Position0 was exact. END was
clean (state2, PCI/host1/1, cold0); no generated token was accepted. The
repeated-`Hello` qualification fixtures did not expose the precision issue.
A CPU-only control on the actual public ChatML prompt also found that the
pinned CPU flash and nonflash graphs differed by logit-L2 0.0435024467 at
position1 and0.0292053534 at position2. This was not evidence of a card fault.

`tenstorrent_haiku_attention_precision.cpp` links/registers only CPU backends.
It decodes three distinct fixed public tokens (151644,872,198), captures all
28 layers, and replays each captured Q/K/V through a separate tiny nonflash
GGML graph. It can export84 public fixtures with nonflash replay outputs.
It never opens the card, reads chat/settings, or generates an answer.

The corrected card kernel follows that original nonflash numerical contract:

- Incoming F32 queries round to IEEE F16 on the card, including subnormals.
- Half-query/half-key products run on SFPU; a two-float card reduction
  preserves the pinned CPU's double-accumulated dot before rounding to F32.
- Stable softmax uses split-float degree10 exp and a corrected reciprocal
  that retains the denominator's low part. No card double/divide is required.
- Normalized probabilities round to F16, matching the CPU's second matmul.
  Half-value products and compensated high/low accumulation run on SFPU.

The actual linked3660-byte RV32IMF image was interpreted with strict fixed
memory/MMIO bounds, Blackhole partial-FMA arithmetic and fault injection.
Synthetic n1..256 and all672 grouped-head tiles from the84 varied-token
fixtures match the independent CPU-nonflash reference exactly. Observed
stack is1360/8192bytes. IEEE half midpoint/neighbour tests and100001 reciprocal
tests (including low denominator parts) pass under ASan/UBSan. Elemental
exp/reciprocal relative errors versus double are below6.1e-8. The GGML
integration separately replays all84 captures with exact all-head outputs.

Native Haiku on the corrected driver also passed n1/3/256 exactly, then five
matrix/four attention handoffs (including256 keys), with clean END. All56
varied-token position1/2 captures, eight groups each, passed **448 exact
CPU-nonflash output comparisons**: ioctl wall3.172236s, whole test3.427302s.
These are primitive checks, not generation throughput.

The corrected full Qwen run subsequently **passed** with the original
cosine/L2/greedy gates unchanged: all22 evaluated positions (19 public prompt
tokens plus three generated-token inputs) had exact full logits, cosine1 and
relative-L2 0. All four greedy tokens matched CPU: `The capital of France`.
Coverage was616 attention ops/4928 grouped tiles and5148 matvecs; all197
tensors/163692 pages were resident. END was state2, PCI/host1/1, cold0.
Total1104.967s included876.625116s of verified weight preparation/upload.
Three warm decode samples took31.658543s (0.094761 tokens/s), excluding the
CPU reference. This is a short diagnostic, **not** a speed benchmark or a
CPU/Fast speedup. KV-prefix PIO and per-tile proofs remain expensive.
Verifier SHA256:
`cc1723522c17cc410ed9e694c25bf3faa7d81d5da99ddc2f3ed1ec62230efb79`.

Corrected driver SHA256:
`ee60544cfedb7ee4ef4b41b534fa675205572d5b8c462732e4c2b6dfb6d51236`.
TRISC image SHA256:
`0d46bb397ecbfa59b7a9df27263949523d8cd94e86d6bb1b8011ec5509fa46c6`.
Probe/mixed-tool SHA256 respectively:
`a642924daf06760a519d5ee5ae35b37c6733cf82af66e133cc6376c6caaf16ec`,
`1cc124b53b678b5688ce53313514800d5c8bf9cb05654d6afce716a138d81764`.
Evidence/rollback: `/boot/home/pioneer-attention-20261016/rounding-wide-a/`,
`precision-b/`, `full-qwen-wide/`; original shared driver preserved as
`rounding-wide-a/original-driver-053e75c0`. Normal atomic replacement loaded
the corrected driver without reboot. Firmware, boot/SD configuration,
kernel CPU/RAM code and the pinned llama runtime were not changed.

The fused backend is opt-in; it requires28 card attention operations/224
grouped-head tiles and234 learned matvecs at every evaluated position. It
still stages each used KV prefix with checked PIO and retains the KV cache
on the host. It is not a speed claim or support for larger Qwen models.

Assistant now exposes the separate ID `p100a-qwen-attention`, with its own
`qwen-p100a-attention` request tag. It uses the pinned0.6B model, card
attention and no independent CPU reference during ordinary requests. Old
checked/Fast modes remain unchanged and keep CPU attention. Switching
attention graphs retains the host model but proves END and discards device
weights; the next reply must upload again. Host-only mode-cycle tests prove
zero uploads/compute. The installed service passed idle mode round-trip,
malformed/unpinned request rejection and Load/Unload/reload preservation
tests. The GUI and standalone Hardware chooser were both updated; only
empty chat/draft lengths were inspected before their normal restart. The
installed service's separate public-question smoke test also **passed** in
`assistant-wide-a/`: `The capital of France is **Paris**`, eight output tokens,
26 evaluated positions/728 card attention ops, zero CPU reference decodes,
success1/exit0. Seven warm generated-token inputs took75.293s (0.093tok/s);
first-use total1117.4s included the fresh device upload. This is a short
application smoke test, not a speed benchmark. Busy-state Load/Unload
rejection also passed. The service is left idle/ready in Attention mode with
the model/device weights resident; the old CPU model and32-thread preference
are retained. Main chat/draft lengths remained0; no conversation text was
read. GUI/service/control/standalone-Hardware backups and settings snapshot
remain in `assistant-wide-a/`. No additional boot/card/firmware change.

### October 16: P100A Fast target without CPU reference validation

The new explicit hardware ID `p100a-qwen-fast` uses the same pinned
Qwen3-0.6B Q8_0 model and two-core exact dual-reuse device runtime. Its cold
load does not create a CPU reference context. Requests do not clear, decode,
sample or read a CPU reference, and skip the full-logit cosine/L2 and
CPU-greedy comparisons. CPU attention/KV/norms, activation preprocessing and
finite-output checks remain. Exact 197-tensor/670,482,432-byte/163,692-page
coverage and 234 actual matvecs per position, completion/stop/ownership,
quarantine and cold-recovery checks remain unchanged. No automatic CPU
fallback or retry is introduced. Finite but numerically wrong output is no
longer independently detected in Fast; this is an explicit tradeoff.

The old validated target remains available, and requests must pin each
mode's own backend ID and prompt format. Warm fast/validated switches run
on the worker, retain the model/card session, and atomically save settings;
failed changes keep the previous selection. Enabling validation lazily
creates the inert-device CPU context if absent. Fast may keep a previously
allocated reference dormant, but performs zero reference decodes. Both modes
retain the existing 512-byte/192-prompt-token/256-context/eight-GUI-token
limits. Fast is not a larger-model or faster-than-CPU guarantee.

The final native Fast build answers the public France question with
`The capital of France is **Paris**`, eight tokens, success true and exit 0.
Its counters report CPU validation OFF, reference decodes 0 and device
positions 26. Total including cold upload is 1,039.7 seconds; seven generated-
token input samples take 47.077 seconds, approximately 0.149 tokens/second.
Timing includes synchronous decode/logit work and in-decode boundary progress,
not model load, cold upload, prompt processing or subsequent per-token output
delivery. This single short diagnostic is **not a repeated benchmark, a CPU
speedup, 27B support or a numerical parity test with validation disabled**.
All weights remain resident. A native fast/validated selection round-trip
passes both before inference and with the actual warm weights, retaining
one model load and the saved CPU thread setting.

A separately selected warm validated request returns the same eight tokens
with every numerical gate enabled: 26 CPU reference decodes, 26 device
positions, 202.6 seconds total, and seven generation samples in 49.875 seconds
(about 0.140 tokens/second). Fast's corresponding generation diagnostic is
47.077 seconds/about 0.149 tokens/second: a modest single-case difference,
not a repeated A/B, CPU comparison or practical performance qualification.
Mode changes retain the single model load and uploaded weights.

The final ordinary GUI exposes four choices, selects Fast, and its actual
Send action returns the same answer with reference decodes 0 and 26 device
positions. Warm latency is 175.2 seconds; seven generation samples take
47.243 seconds (about 0.148 tokens/second). Comparing 175.2 with 202.6 seconds
is only a single-case latency diagnostic, not a repeated performance claim.
Applying the current Fast pair keeps one model load and all uploaded weights;
the twelve rejection checks still pass. The GUI is left open/ready in Fast,
with this public reply and its exclusive resident session warm.

Cross-build/warnings-as-errors, native model-store and hardware/settings
checks, twelve malformed/cross-mode request rejections in both modes, and
the pinned ggml ASan/UBSan backend regressions pass. A native Fast Stop test
enters the first tensor upload, closes END2/PCI-host1/1/cold0 and stays ready.
A review caught a Stop/fault race: an actual failure must not become benign
merely because Stop is also set. The runtime now distinguishes Stop actually
observed at a safe boundary. An initial public upload was intentionally
stopped cleanly to install this correction, then the final Stop/full-request
tests passed. No uncertain-operation retry or recovery gate was bypassed.

The human approved restarting the saved-chat GUI. Original userland/settings
backups and public logs are retained in
`/boot/home/pioneer-assistant-fast-20261016`, with public local copies ignored
under `generated.pioneer/tenstorrent/assistant-fast-20261016/`. No private
conversation was read. Final service SHA-256 is
`bda87b2aa7378b3fc53896c42baf6e80231d0306524fcf7e0ff350991f91b883`,
GUI `78d1860e00f916f069ef2397fb7a34ac9bf8687a1e9bb0a8ef4bc10e971eaf38`,
Hardware dialog `8a93f54de20a993be2791bcb68c160f9b55ea1f38f2c2334a9fb1b690150527f`,
and control `5699b1f3c1d16f0c299e85054cc86c5c25b8640e70782d211ecfda048a0cae5e`.
Kernel/firmware/power/boot/models/runtime libraries are unchanged; the old
warm session closed cleanly and pre-test card audit passed. The dirty
llama.cpp submodule remains untouched. See
[Assistant Fast verification](../../src/apps/assistant/README.md#fast-target-verification-2026-10-16).

### October 16: explicit Qwen/P100A mode in Assistant

The ordinary Assistant, Hardware dialog and control tool now select the
verified Qwen3-0.6B Q8_0/two-core exact dual-reuse pair as `p100a-qwen`.
CPU and legacy TinyStories remain separate explicit choices. The CPU model
and thread preference are preserved. The new mode is clearly labeled slower
than CPU: one short raw question, no history/tools, eight output tokens in
the GUI, 512 input bytes / 192 prompt tokens and a 256-token context.
No model download, kernel, firmware, boot, power or runtime-library change
is involved. The installed driver remains `fd1672b327f655cb739b7a481367b9ecb5d76a8a304d995c2077c4b35e950d9a`.

Every evaluated position requires all 197 learned matrices, 670,482,432 packed
bytes, exactly 163,692 verified upload pages and 234 real matvecs. The CPU
handles attention/KV/norms and an independent 32-thread full-vocabulary check;
output comes only from card logits, never reference substitution. Existing
cosine/L2 and CPU-identical greedy output gates remain enforced. A device or
uncertain-cleanup fault disables both card options for the service session;
there is no automatic retry or CPU fallback. Stop is honored only between
completed uploads/matvecs. Successful requests retain weights and ownership.

The first complete native Assistant request `What is the capital of France?`
returns `The capital of France is **Paris**`, eight checked output tokens,
success true and exit zero, in 1,075.5 seconds including first-use upload.
All matrices remain resident and the service is ready without a model error.
This is integration/correctness evidence, **not practical acceleration or
27B qualification**. The earlier standalone performance figures remain
diagnostics, not an Assistant benchmark.

The reopened ordinary GUI's real Send action also returns the same checked
eight-token public answer, reusing warm weights without any upload or model
reload. Hardware shows selected/running `p100a-qwen`, three choices and the
experimental limitations. Applying the current pair retains its single model
load; malformed-request checks still pass afterward. The GUI and model are
left ready, with this public reply visible and the resident session warm.
This is not a warm throughput or CPU-comparison benchmark. The public GUI
evidence is `public-gui-qwen.log` in the native folder below; local copies of
public evidence are ignored under `generated.pioneer/tenstorrent/assistant-qwen-20261016/`.

Native settings/capability, busy-loading, malformed-request and model-store
checks pass. CPU switch-back after accelerator registration restores the
saved Qwen3-1.7B/32-thread pair and answers the public READY prompt. An initial
inactive-backend scheduler assertion was caught before card execution: the
inactive device must accept its default host buffer while rejecting every
operation. The corrected native CPU round-trip and a new ASan/UBSan scheduler
assignment regression pass. Boundary-cancellation/failure/cache software
tests and resident session/stream/tensor/benchmark-controller regressions pass.
The one-second native cancellation test stops before opening the device;
it is not evidence of cancellation during a native transfer. The pre-test
card audit passes with cold-recovery zero. No competing card probes are run
while Assistant retains its warm exclusive session.

The human approved reopening the saved-chat GUI. Original userland/settings
backups and public logs are retained in
`/boot/home/pioneer-assistant-qwen-20261016`; no private conversation was read.
Final service SHA-256 is `7f0f199d14ee931a17d44305d2ad715191e2fb40c2235aeaba48cd0d9e10340b`,
GUI `30d19d8a9e7f0b38ecc0916b8988ca6968643b08bff113a2a74707e917bdcf86`,
Hardware dialog `6ef3f18d44797af23ccd51687c98d5b9885c3915298a30bf25b2303f939f4011`,
and control `ae1d8343fdd137012a4f71176465ee3bd6894cb8f4c1d0a01981e4a9cca5921a`.
The full standalone cross-build passes with warnings as errors. Reviewed
sources are synced to the existing build mirror; the user's dirty llama.cpp
submodule is neither built from nor changed. See the
[Assistant integration checks](../../src/apps/assistant/README.md#qwenp100a-integration-checks-on-pioneer-2026-10-16).

### October 15: checked streamed uploads and numerical localization

#### Native scaled Q8 integer blocks (full generation still unqualified)

##### Native signed-byte hardware unpack (alignment-corrected candidate)

The new `TT_RUNTIME_Q8_NATIVE` image replaces the compact scalar lookup loop
with the hardware INT8 unpacker. The 576-byte compact shape and original
FP16 scales are retained, but signed weight bytes have a distinct lossless
sign-magnitude encoding. The external signed unpacker cannot represent -128;
the host packer refuses it without clamping. Activations retain the existing
FP16-overlay path and its full signed-byte range. Cache and prepared-code
formats remain immutable. New ioctl `54c4` uses the existing Q8 request ABI,
CPU profile format6 and capability bit8192 (support12287). Bit4096 is reserved
for a rejected unpublished prototype and is not accepted as current support.

An initial 32x1024 hardware trial completed with exact 64/576/64/64 NoC
counts, 128 math tiles, correct phase/park, clean END2/PCI-host1/1/cold0 and
a passing post audit, but failed all numerical rows. It was not retried as
full inference. Investigation found that the prototype read from aligned
GDDR into `706f0`: these addresses are not congruent mod32, violating the
documented [NoC alignment rule](https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/NoC/Alignment.md).
The corrected fixed destination is aligned64 `706c0`, with INT8 halves at
`706e0/707e0`; saved scales remain `70680`. Header clearing happens only
after scale preservation / previous-half consumption. The entire payload
lies inside the already saved 4KiB code/data page. The runtime and independent
models now explicitly check congruence; exact completion and quarantine
requirements are unchanged. No temporary kernel tracing remains in source.

Independent assembled-image equality and ASan/UBSan arithmetic tests pass
all K widths, N4096, two vectors, alternate bases, exhaustive lossless encode
or refusal, corruption and bounded failure parks. Driver policy tests pass
all65 native dropped-control-write cases and explicit bad PC/flits/timeout/
stop/source failures. Cache/backend tests cover the third immutable format,
no fallback, unsupported -128 refusal, repeated vectors and classifier chunks.
Legacy BF16/FP16/INT8/scaled-Q8/compact image regressions also pass.
The corrected native image passed real Qwen 32x1024, 32x3072 and full
2048x1024 projections, each with two independent vectors and all original
block data/scales. All rows passed the unchanged numeric gates; full-query
relative L2 is `9.13021911e-8 / 9.70029151e-8`, max absolute `4.76837158e-7`.
Exact counters are4096/36864/4096/4096 for8192 math tiles. Nonfinite scales,
expanded/native and raw-byte/native format switches are refused before
start; subsequent reuse is bit-identical. All ENDs are2, PCI/host1/1,cold0.
The updated native foreign/inherited/duplicate/final-close lifecycle passed.

Full-query second CPU wall time is27,545us, including16,122 compute/proof
and6,088 PIO. This removes the compact scalar decoder's measured slowdown
(75,016us total /63,423 compute), and is close to the earlier expanded Q8
projection (29,821/18,360). It is **not** a CPU inference speedup. Full-model
generation and matched real-world latency/throughput remain unqualified.

Frozen aligned artifacts under `/private/tmp/pioneer-native-aligned-tools-20261015/`:
driver `cf495ccfbab435ed44aedd998739e18cefd47ffe4b753a92c2bc8ee3eb49bba0`,
projection CLI `c18c24590af2046472f44a854e155ce57add10b72a61ab3b6d7594ef4053c782`,
lifecycle CLI `cd59393e91bfa6d3ccbf939a337c21beb87e3fd7d0344d1b9ff39697514ebecf`.
The original working7d09 driver is preserved in
`/boot/home/pioneer-q8-native-20261015/tenstorrent_pci_probe.before`.
Support12287 was observed before submitting the corrected native trial.
No forced module unload, cold-gate bypass, firmware or boot change was used.

###### Exact FP32 scale / compensated SFPU (native projections qualified)

A distinct `TT_RUNTIME_Q8_NATIVE_F32` candidate expands each original finite
FP16 weight scale to exact FP32 bits once at packing time. Its 576-byte tile
is64 scale bytes plus512 signed native quant bytes, with no padding. The
on-card reader copies those sixteen scale words instead of repeating sixteen
integer half-to-float conversions for every 32-K output block. Activation
quantization/scales, weight values and total footprint do not change.
This is **not** the earlier FP32 K/V-cache experiment.

The candidate now also replaces partially fused dot/scale accumulation with
a fixed SFPU sequence: 12-bit product decomposition, two error-free sums
and round-to-odd before final FP32 rounding. All learned integer products,
scales and accumulation remain on the card, without host correction or
reference substitution. Both parities spill only to private Dst8..19; scale
rows0..7 and integer matrix output32..35 are disjoint. The trusted instruction
table70ea8..70f53 is read-only data, not executed by the RISC. The helper is
70d80..70e3f. Everything fits the original saved4KiB code/data page; common
completion/park and exact NoC counters are unchanged. Conditional execution
is explicitly disabled again before either parity finishes.

The separate immutable mode has ioctl54c5, capability16384, profile7; candidate
support28671 retains the qualified native8192 bit and all original modes.
It uses the same aligned64 NoC706c0 destination, with input halves70700/70800
and a64-byte saved scale area70680..706bf, all inside the saved code page.
The pack layout is tested for complete, non-overlapping byte coverage. All
65,536 half patterns are checked independently (nonfinite values still must
be refused by packers), including signed zero/subnormals. All K/N4096 shapes,
two vectors, corruption/omission and bounded NoC failure parks pass ASan/UBSan.
The original scale-copy-only candidate used68,457 modeled RISC steps; it is
superseded before deployment. With compensated SFPU the same small case uses
96,617 versus91,113 for native-FP16 scales. This is not hardware timing or a
speedup claim. The actual assembled table passes262,144 independent randomized
finite-Q8 lanes, including severe cancellation. All supported K / N4096
outputs match independent IEEE FP32 FMA bits. A separate formula test passes
millions of finite-Q8 random/cancellation cases. These are offline proofs,
not hardware or universal IEEE FMA claims.
Both native scale formats pass all65 dropped-control-write policy cases,
immutable-format refusal and timeout/bad-PC/flits/stop/source quarantine tests.
Legacy assembled-image regressions, Jam and all31 utilities build.
The compensated driver `4cdc779a21496b56e12962b55ec35d8c2484981ba31a54b37c335592fc5d106c`
is now installed. Qualified cf495... was preserved as
`/boot/home/pioneer-q8-native-20261015/tenstorrent_pci_probe.before-exact-cf495`.
Natural module turnover reported distinct support28671; no forced unload.
Native32x1024,32x3072 and full2048x1024 tests pass both vectors, original
numeric gates, exact NoC counts, all format/nonfinite refusals and identical
reuse. Full-query warm validation-inclusive total30,998us / compute-proof
19,552 / PIO6,053: no CPU speedup. END2 and independent post-audits all pass.
Ownership/foreign/inherited/duplicate/final-close coverage also passes.

Most importantly the real Q13 replay with standalone binary
`56333500720e5a7606c000487116d364bb186a6c25ac9137d279902a44225fc8`
now matches **all2048 independent CPU FP32 output bits**, and its private
CPU norm/RoPE output also matches every bit, with zero half-boundary changes.
Log: `exact-sfpu-q13-replay-first.log` in the native test folder. END2/PCI-host1/1/
cold0 and post-audit pass; Assistant remains unchanged. This establishes the
specific projection fix, not full inference or performance.

Full `--verify-native-q8-exact-host1` completed successfully in
`full-qwen-exact-host1-first.log`, with the same full-vocabulary and strict
greedy gates, CPU32 reference and actual host1. All22 positions pass with
reported logit L2 zero; four real reply tokens match CPU (`The capital of
France`). All197 matrices /670,482,432bytes stay resident after163,692 page
uploads and5,148 real matvec calls. No reference substitution or learned
CPU-product fallback was used. Total1092.333s: preparation880.718824s,
activation preprocessing0.367404s, matvec/staging/result187.035777s. Warm
prompt positions take8.61..8.70s rather than the previous verify32's~47s;
this is not an isolated kernel comparison. Three generated-token input
samples total25.898678s =0.115836tokens/s, explicitly a diagnostic, **not a
matched repeated benchmark or CPU speedup**. END2/PCI-host1/1/cold0 and
independent post-audit pass; Assistant is still team2705 idle, same TinyStories
model/backend and three model loads. Both SDs remain out; no boot change.

###### Sixteen-tile GDDR reader (native projections qualified)

`TT_RUNTIME_Q8_BULK` keeps the same exact-scale packing and compensated
arithmetic, but reads16 contiguous576-byte tiles in one9216-byte NoC request
to fixed L1 `88000..8a3ff`. It uses bounded tile indices0..15 and the same
scale-preservation/consume-before-header-clear discipline, without copying
or decoding raw weights. Pinned tt-metal `d279aa36` Blackhole
`noc_parameters.h` sets the maximum burst to256x64B=16KiB;9216 is below it.
The fixed bulk buffer must be additionally saved/mapped: the existing32KiB
runtime-buffer lease is insufficient. A new distinct54c6 operation, bit32768,
profile8 (candidate support61439) selects it; older operations cannot.

Independent assembled-image tests pass every K / N4096 with exact IEEE bits,
alternate bases/vectors, untouched surrounding data, skipped/corrupted payload
rejection and bounded NoC failure parks. On the same small model fixture,
read requests drop64 to4; RISC steps96,617 to95,757. Neither count is hardware
timing or a speedup. The candidate driver separately maps/saves48KiB, refuses
an insufficient32KiB mapping, uses immutable bulk format selection and
restores the complete extended buffer only after the existing completion /
stop proofs. Each read requires exact144-flit accounting and one response /
request / accepted ID; total data bytes remain identical to the old reader.
All65 dropped post-upload control writes, timeout/bad-PC/flits/stop/source
faults, format switches and extra-buffer rollback pass the policy model.
Jam and31 tools build, with no deployment while the full-Qwen test owns the
card. The updated projection CLI independently checks all NoC deltas and,
for both exact modes, requires every output FP32 bit to match CPU IEEE FMA.
The candidate `a8ab8fafae714c4ae75ddb4c6bb7a010f0730aee294b0b0963ec2d86a8409bf4`
is now installed, with precision driver4cdc... preserved in
`/boot/home/pioneer-q8-bulk-20261015/tenstorrent_pci_probe.before-bulk-4cdc`.
Support61439 loaded naturally. The old exact32-row path passed every CPU
IEEE bit first. Native bulk32x1024,2048x1024,32x3072 and1024x3072 then passed
both vectors, every output FP32 bit, independent exact NoC accounting,
nonfinite/format refusals and identical reuse. Full query uses256/36864/256/
256 NoC deltas for8192 math tiles; full FFN384/55296/384/384 for12288 tiles.
Every END is2, PCI/host1/1,cold0; independent audits and updated foreign/
inherited/duplicate/final-close lifecycle pass. Assistant remains unchanged.

A same-driver full-query control gives a warm validation-inclusive sample
30,645us old versus28,465us bulk (compute/proof19,136 versus17,016; PIO6,068
versus6,054). This is a modest projection-only improvement in a single warm
sample, **not** a repeatable real-world or CPU speedup. Small32-row bulk was
slower/noisy (9,709us versus5,289), so bulk is not presumed universally faster.
Full FFN bulk warm33,661us includes25,137 compute/proof and3,037 PIO.
Logs are in the separate bulk folder; current standalone projection CLI
`c62ad340892171f83c1f67a7808ff8f95569e117a84853d47fa972665afbba81` and lifecycle
`5ac0a1e77bcb372ded31a115dc29cb19fe1301321f749a28cfa8fb671cfc47b6`.
No full-Qwen bulk benchmark or Assistant change has been made.

###### Second-core discovery (native read-only qualification passed)

The next scaling prerequisite is an explicit fixed(1,3) NoC source/idle
probe, separate from the old(1,2) operation. New54c7/54c8 probe/support return
version2; old54a0/54a1 remain version1. No caller coordinates are accepted.
It repeats the same120-core-held / seven-GDDR-route baseline, temporarily
selects only the fixed read-only NIU pages, requires three correct node /
endpoint / source-table / empty-command / outstanding samples and restores
TLB/PCI state. It cannot start a second core, upload code, ring a doorbell,
clear counters or transfer payload. New actual-probe calls require FD creator
team; all legacy-owner conflicts and cold precedence remain. Offline models
pass both fixed rows, every dropped TLB write, invalid coordinate refusal and
identity/source/busy/outstanding/restore faults.

Driver `62badf1bfd8f679a68ee3304d7f3e29802c9f4a3184f89a72c5c12915b17e869`
loaded support version2 naturally. Native `second-core-noc-first.log` in
`/boot/home/pioneer-second-core-20261015` passes: node106110c1, endpoint10003,
three identical sources, four empty command slots and sixteen zero outstanding
counts per sample, all120 cores held / seven routes. PCI/TLB rollback1/1,
cold0, both independent post-audits and foreign/inherited/duplicate/final-close
lifecycle pass. Assistant remains team2705 ready/idle with the same model.
This is identity/route/idle qualification, NOT second-core computation.

A separate fixed54c9 operation, capability65536/profile9, now implements the
next candidate: exact bulk Q8 on (1,3) only, while (1,2) stays held. Bank0
uploads still use the established first-core path. Before touching second-core
code it repeats the read-only probe, validates its own host route, and takes
independent code/48KiB buffer/NIU/8KiB tail backups. All activation and weight
read destinations name (1,3); owned PCIe views return to (1,2) after each call.
Completion status also dirties the tail, which must be restored even though
the native bulk weight reader does not use that tail. The session cannot
switch between first/second runtime modes. Global uncertainty blocks payload
restoration on either core, quarantines the one system-owned host page, and
permits only PCI/TLB rollback. No new coordinates/physical pointers/code API.
The model passes all65 post-upload dropped control writes, destination checks,
tail rollback and timeout/PC/flits/stop/source faults.

Native fixed second-core computation now passes32x1024, full2048x1024 query
and full1024x3072 FFN, both independent vectors with EVERY output FP32 bit
matching CPU IEEE FMA. All NoC deltas, format/nonfinite refusals and identical
reuse pass. Driver636e77bf915599f5505624ede21c9c2440c14e4d1cc927be19934dfac6985122,
CLI5c79c9142d28cbd3abb803641e67de847216d16177e95aea985e8054264d3cee,
support126975. Logs are in `/boot/home/pioneer-second-compute-20261015`.
Warm query total29,037us / compute-proof17,443 / PIO6,065; warm FFN35,255 /
26,850 /3,032. This establishes a second independent worker, NOT parallel
execution or a CPU speedup. All END2/PCI-host1/1/cold0 and audits pass, as does
updated ownership/last-close lifecycle. Assistant is unchanged.

###### Two-core scheduler (full Qwen correctness qualified; not CPU-faster)

New54ca / capability131072 / profile10 partitions rows equally between fixed
(1,2)/(1,3), with N a multiple of64. Both derived half ranges remain page-
aligned and wholly inside the initialized parent matrix. No K reduction is
split, so each row keeps the qualified summation order. The same fixed bulk
image/packing is used on each worker. Both input/scale/result buffers and
commands are staged and context-armed before either launch; BME is off before
any core executes. Both must independently prove exact phase/tile/marker,
two park PCs, all four NoC deltas, idle/source/PCI state before ANY output is
collected or either core stopped. Each stop is verified twice. Timeout,
mapping/route uncertainty or one failed stop forbids resetting the peer or
restoring either payload and quarantines the host page. END prechecks both
cores before restoring either. Only established PCI/TLB rollback is allowed.
The distinct ABI reports both complete per-core proofs plus a checked total;
creator-team/exclusive ownership and immutable per-session mode remain.

Offline ASan/UBSan tests pass130 individually dropped post-upload writes,
all12 success-path mapping boundaries, timeout/PC/flits/stop/host-request/
source faults on either worker, safe numerical refusal, reuse and complete
shape/range partition coverage. Both must be armed before the first launch
and the first must still be released at the second launch. This is not a
native computation, end-to-end model result or measured speedup claim.

Installed frozen driver235079a0c6ff66bcf3cb9e8b1d991e46bf2ab7bdd729e311bff0501a7371e152
loaded distinct support258047 naturally; native-qualified636e is preserved as
`tenstorrent_pci_probe.before-dual-636e` in `/boot/home/pioneer-dual-compute-20261015`.
Native64x1024, full2048x1024 query and full1024x3072 FFN pass both vectors,
EVERY CPU output bit, independent per-worker NoC/phase/PC/stop proofs,
nonfinite/mode refusals and identical reuse. All END2/PCI-host1/1/cold0 and
independent audits pass, including dual-op foreign/inherited/last-close checks.
Warm query25,544us includes10,159 compute/proof and6,077 PIO; FFN29,805 /
15,233 /3,075. Same-driver one-core query control31,893 /20,478 /6,044. These
are directional individual projection samples, not matched repeated inference
or CPU speedups. Small64-row dispatch still costs12,094us warm. The CLI is
da121deae344f5915ad20fb137c7889e1bfdaa6af30f5e31227d6903b32b1ec0.

The standalone ggml/native client now has an explicit immutable two-core flag,
requires the distinct capability before BEGIN, enforces N multiples of64,
and accepts outputs only with BOTH independent proofs (including counter
deltas/PCs/markers), otherwise close-only with no fallback/retry. Offline
pristine pinned-ggml tests cover two vectors, exact FP32 outputs, one upload,
active-mode immutability, unsupported shape, invalid flag combinations and
device failure with output left poisoned. Every critical proof field is
independently corrupted on either worker and refused. Old modes remain.

Full `--verify-dual-q8-host1` passed in `full-qwen-dual-host1-first.log`
with binary8eb46f58f6d5be2995a107549aef6f98914e85bb226cf4b098e622408014d877.
It preserves CPU32 reference creation before card registration, all197 learned
matrix coverage, original cache/flash settings, all-vocabulary gates and four
strict reply tokens. All22 positions report cosine1 / L2 zero and matching
argmax; four real tokens are `The capital of France`. All197 tensors /
670,482,432bytes /163,692 uploads and5,148 actual dual matvec calls are covered.
END2/PCI-host1/1/cold0, independent post-audit and unchanged Assistant pass.
Total1063.167s: weight preparation884.641915s, activation preprocessing0.369106,
matvec/staging/result154.343633 and diagnostics0.000260. Warm positions are
7.08..7.24s. Three generated-token input samples total21.348551s =0.140525tps,
versus prior one-core exact0.115836tps. This is a diagnostic improvement of
about21% in rate across the new bulk+dual path, not an isolated kernel A/B,
matched repeated real-world benchmark, CPU speedup, Assistant or27B claim.
No forced unload, firmware/clock/boot change or Assistant model/backend change.

###### Exact activation reuse (full native Qwen correctness qualified)

Separate54cb / bit262144 / profile11 adds per-core private CPU operand/scale
shadows to the same fixed two-core bulk image. Its session mode3 cannot
switch to uncached mode2. Keys compare EVERY operand and scale byte, including
signed zero; there is no hash-only match. A remembered CPU copy is not valid
until its real compute has completed/stopped with exact NoC/PC/marker proof.
Before either input can be reused, BOTH workers must be held/idle with stable
source/PCI/host mappings. Output poison is NEVER skipped; every learned
product, result collection and both independent completion proofs remain.
Any execution failure invalidates both shadows; uncertainty retains the same
global quarantine/no-reset/no-payload-restoration rule.

The actual assembled bulk image is additionally checked to leave the ENTIRE
16KiB input/scale region unchanged for every K and two vectors. Exact-key
tests change every word independently, signed zero, nonfinite/invalid operands
and every critical proof field. The two-core model proves cold/changed-input
full staging and output-only staging on hits,70 individually lost cached-path
writes, all14 mapping boundaries and source/host-map/PCI changes on either
worker BEFORE any new DMA/start/stop. Legacy uncached130-write tests still
pass. Pristine pinned-ggml fake tests run both client modes, exact outputs,
active flag immutability, invalid flag combinations and device failure/no
fallback; these fake results are NOT model or hardware throughput evidence.

Initially staged driver25454115f888e9bfcff2e1b57315ba35a25d2479c08c896bb789530c181bb97d
and CLI411c1a73d94e9ee2d6d6ede30c15ed2861fbf87167efa59aabfdd7d8b855773e
were built and staged in `/boot/home/pioneer-dual-reuse-20261015`, NOT installed.
That frozen driver is superseded by the stricter output-validation source
below; do not install it. Replacement4ca653a3ff622b0976d1dde86d08d4a49dd8b07d8cd5e7cd61c75f78660d3c0f
has built locally in `/private/tmp/pioneer-dual-reuse-checked-tools-20261015`
with all31 diagnostic tools; CLI/lifecycle hashes above are unchanged.
The full-model run closed with known END/stop/cleanup and independent audit.
Automatic safety review then rejected installing this unqualified activation-
reuse kernel driver as higher risk needing explicit user approval. Do not
work around that rejection: request approval explaining potential Haiku
freeze/power-cycle recovery. The rejected installation did not run.
On October16 the human explicitly approved that installation/test after the
risk/rollback question. Fresh pre-audit passed, Assistant was idle and the
installed235079 identity was verified before any change. Replacement4ca653
and matching tools were staged/hash-verified in
`/boot/home/pioneer-dual-reuse-checked-20261016`. The old driver is preserved
there as `tenstorrent_pci_probe.before-reuse-235079`; the replacement was
installed atomically. Support520191 appeared through natural module turnover,
with NO forced unload/reload, firmware, clock or boot change. A module reload
is never cold recovery.

Native64x1024, complete2048x1024 query and complete1024x3072 FFN-down tests
all pass: BOTH changed vectors require full staging (6/6,6/6,8/8 pages),
every result matches CPU IEEE FMA bits (zero mismatches), all format/nonfinite
refusals pass, and identical reuse needs exactly2/2 output-only staged pages.
Both independent compute/stop/NoC proofs pass on every operation. Each test
closed END2/PCI-host1/1/cold0 and passed an independent post-audit. The new
operation also passes foreign/inherited-FD denial, duplicate-FD retention and
final-FD-close cleanup. These are correctness/transfer-reduction results, NOT
full-inference or CPU-speedup qualification. Public logs are `reuse-*-first.log`
and `lifecycle-first.log` in the fresh folder.

Qwen CLI modes are
`--verify-dual-reuse-q8-host1` / `--benchmark-dual-reuse-q8-host1`, retaining
the same reference/thread/quality gates. Standalone verifier
3f3f674b25a8e8f299d4d9c5c9995d6dd4c1721787de0b74ff4258cd309faa71 is built
locally and hash-verified with the pinned model/libraries on Haiku. Full
`--verify-dual-reuse-q8-host1` job46638 exited0: all22 positions report full-
vocabulary cosine1/L2 zero and matching argmax, and all four strict reply
tokens match CPU: `The capital of France`. All197 tensors/670,482,432bytes,
163,692 uploads and5,148 real dual-reuse matvec calls are covered. END2/
PCI-host1/1/cold0, independent post-audit, installed/rollback hashes and
unchanged idle Assistant all pass. The complete public log and exit marker
are `full-qwen-dual-reuse-host1-first.log` / `.exit`; local copies are under
`generated.pioneer/tenstorrent/dual-reuse-20261016/`.

Total1042.955s: weight preparation864.444809s, activation preprocessing0.368205,
matvec/staging/result145.080888 and diagnostics0.000259. Warm positions take
about6.67..6.80s; three generated-input samples20.195347s =0.148549tokens/s,
about5.7% higher than the prior uncached dual diagnostic0.140525. This is
NOT an isolated repeated A/B or CPU-speedup qualification. The result remains
far short of practical accelerated Qwen and does not qualify27B or Assistant.

###### Exact result transport prerequisites (native baseline; optimized path offline)

An adversarial test exposed a validation gap in the older experimental54be
output-DMA helper: after the ordered read it compared the host page to its
host snapshot, not the actual fresh L1 readback or original device output.
Omitted readback data could therefore pass despite complete NoC counters.
The old native projection comparisons remain historical evidence for those
particular vectors, not proof of this stronger transport property. Qwen's
passing native modes use PIO and reject Q8/integer output-DMA combinations;
their correctness result is unaffected. The stricter helper is included in
the approved4ca653 replacement; that separate experimental output-DMA
operation is neither used by Qwen nor qualified by the activation-reuse tests.

The source helper now compares ALL words of original output, fresh L1 return
and the CPU-visible host snapshot, including padding. Tests reject omitted
readback, every one-word corruption of all1024 returned words and all1024
finite posted-write words, leaving output unaccepted/quarantined. Existing
lost-control/poison, visibility, counter, route and cleanup tests still pass.
This conservative baseline adds2048 payload PIO reads/page. It is intentionally
NOT a speedup; future optimization must preserve this byte-identity gate.

The stricter baseline is now native-tested under4ca653 with32x1024 and full
2048x1024 real query projections. Both independent vectors pass, including
every original/fresh-return/host word and poisoned padding, with zero result
bit differences from separately rerun PIO executions. END2/PCI-host1/1/cold0
and independent post-audits pass. These are the legacy BF16 projection's
existing numerical gates, NOT exact-Q8 or full-Qwen output-DMA qualification.
Public logs are `strict-output-32x1024-first.log` and
`strict-output-2048x1024-first.log` in the dual-reuse folder. At2048 outputs,
the strict DMA phase takes about18.6ms versus6.1ms PIO: safety baseline only,
not a performance win.

Before these tests, the real legacy diagnostic's error paths were hardened:
failed BEGIN/upload/matvec/status requests, incomplete transfer/compute/stop,
cold-session reports and unsafe profiling now cause close-only, with no
explicit END or subsequent card command. The C++11 ASan/UBSan test compiles
the REAL main against synthetic syscalls/model data, rejects all62 failing
ioctl boundaries with/without profiling and eight ambiguous-success cases.
This test is explicitly synthetic, never model/device/performance evidence.
Native CLI7a88fe4b86cc8a0d206c3f65f0936afe5c1304b80af61d283b080078899aebde
was used for the stricter tests; all32 tools now cross-build.

A separate `TT_RUNTIME_Q8_READBACK` image is assembled/model-tested but has
NO driver opcode/capability/native selector. It preserves the qualified bulk
math bytes and original park, with alternate entry7556a06f to a164-byte routine
in previously zero tail70f54..70ff7. Selector0 enters unchanged math; selector1
compares fixed original84000 against fixed fresh return88000,16..2048 words.
It ORs every word's XOR difference (not a collision-prone checksum), records
count/mismatch mask/phase70fb4/markerRVW1 and parks70fe8. Invalid selectors or
counts refuse before any payload read. It cannot write payload, NIU, SFPU,
reset or DMA registers. Future execution must happen only AFTER transfer
completion and BME-off proof, preserving no host DMA during compute.
The metadata predicate alone is NOT a complete acceptance gate: the future
controller must also prove fresh clearing/readback of all FOUR status words,
immutable image/descriptor, owner/mappings/source/PCI/idle state, zero new
NoC traffic, and both verified stops before accepting either result.

ASan/UBSan model tests pass assembled-byte equality, all3184 independent word
corruptions across16/32/64/1024/2048 words, double-bit cancellations that would
defeat XOR checksums, signed-zero differences, nonzero initial registers,
wrong count/selector/marker/PC and complete data/register-write bounds. Normal
bulk math remains IEEE-bit-identical for all K; old BF16/FP16/INT8/native-FP32
and fixed-image regressions pass. These are NOT native or timing results.

`PioneerRuntimeReadback.h` now separately models the bounded two-worker
comparison lifecycle. It requires prior successful math proofs, both workers
held, the private input/output owners mapped stably, fixed TLB/source routes
and PCI bus mastering OFF. It verifies the alternate entry/executable tail,
configures the bounded selector/count, freshly clears/reads back ALL FOUR
metadata words, arms BOTH contexts before either launch, and requires zero
new read AND write NoC traffic, exact metadata and two parked-PC samples.
Both comparisons must complete before either stop; metadata/counters are
rechecked before stopping. An uncertain completion/map/route/stop quarantines
BOTH host owners, forbidding peer reset or payload restoration. Known
comparison mismatch can stop safely but returns failure, never output.

Its ASan/UBSan tests pass all20 lost control writes,12 mapping selections,
48 reset-read boundaries,132 host-map boundaries, each fixed TLB field,
wrong/stale metadata, early/late counter changes and stop failures on either
worker. All4096 independently corrupted returned words fail, padding included;
the comparator performs no payload PIO, DMA, PCI/NIU writes, output acceptance
or buffer release. This remains an OFFLINE prerequisite, not a transport
controller: it cannot establish DMA freshness/order or frozen-host-snapshot
identity by itself. No native adapter, ioctl or capability exposes it, and
selector0 must be explicitly restored before any future math launch.

`PioneerOutputBuffer.h` separately models a private16KiB output area's owner
lifecycle. It requires four contiguous high physical pages, exact whole-area
size, no input-page overlap, noncacheable attributes, four pattern passes and
stable whole/per-page remaps. Pending/uncertain/failed-release areas cannot
be freed. Tests cover every16384 lost store, all four noncontiguous/short-page
cases, input aliases, high-address/overflow limits, map/cache failures and
release/quarantine behavior. There is still NO resident transport adapter,
public address API or output-DMA controller for this buffer. A future
controller must quarantine BOTH owned areas after uncertainty and prove
readback identity before accepting output; CPU-only allocator qualification
below does not establish DMA coherency, freshness/order or acceleration.

A separate CPU-only probe54cc/support54cd v1 now qualifies the native allocator
without sending any address to a device or exposing it to the caller. It
allocates a private4KiB input and contiguous16KiB output above4GiB, verifies
all4 pages/no alias/noncached pattern passes and remaps, then releases both.
No PCI/MMIO, NoC/DMA, core execution, caller address or persistent resident
buffer is available through this operation. ASan/UBSan model tests cover all
18,432 lost stores, each bad/short page, alias/cache/map/allocation/release
failures and retained ownership after failed release.

Driverfd1672b327f655cb739b7a481367b9ecb5d76a8a304d995c2077c4b35e950d9a
and probeea8459b04a3a101c25720786bd822be65981afc00f7bd48bfe99c0fb1af80136
built and were installed in `/boot/home/pioneer-output-buffer-20261016`
under the user's approved kernel-test scope. Full-Qwen-proven4ca653 is saved
there as `tenstorrent_pci_probe.before-output-buffer-4ca`; no forced reload
was used. Native CPU-only probe PASS:4096/16384bytes,33/33 address bits,
2/4 pattern passes,0/0 mismatches, all4 mappings stable, both owners released
and no quarantine. Resident compute/DMA capabilities remain520191, unchanged.

Updated lifecycle CLI14fda8d216582c2e88accbae4868c2d31cbcf9c351c1b590232a8ea2b82549ad
proves both new support/allocation requests are denied EBUSY while a session
is active, including owner, foreign and inherited legacy handles. Existing
creator-team denial, duplicate-FD retention and final-close cleanup still pass.
The complete2048x1024 exact dual-reuse query regression also passes both
changed vectors, every CPU FP32 bit, output-only cache hit, all format gates,
both worker proofs and END2/PCI-host1/1/cold0. These are CPU-only allocator
and unchanged-path regressions, not another full-model or speed result.
Logs are `output-buffer-first.log`, `lifecycle-output-probe-first.log` and
`reuse-query-regression-first.log` in the fresh output-buffer folder.
Independent post-audit passes with PCI commands0/0, TLB/PCI restored1/1 and
cold0. Installed/rollback hashes match, and Assistant remains team2705,
ready/idle, three model loads and the unchanged TinyStories backend.

##### Compact Q8 candidate (native projections qualified, not full inference)

A separate fixed `TT_RUNTIME_Q8_COMPACT` image retains the scaled integer
arithmetic but stores one 576-byte tile per 32-K / 16-output block: 32 bytes
of original FP16 scales, 512 original signed quant bytes, 32 bytes padding.
That is **1.125 bytes/parameter**, exactly half the expanded Q8 layout.
N must be a multiple of 32, making all supported K tensor/chunk sizes 4-KiB
aligned. No scale/quant is rounded or discarded. The tensor cache packs
32 output rows at a time and preserves the existing expanded mode.

The fixed reader fetches into the already saved `17e000` L1 tail only for
even 16-K math tiles; the next tile reuses the other half of that payload.
A fixed 256-entry signed-byte lookup at `70e00..70fff` expands every value,
including -128, into the already qualified integer overlay at `70710`.
Neither payload nor descriptor is executable. Scalar byte/halfword accesses
remain strictly within these fixed regions. Original scales are preserved
before vector staging; SFPU math and final phase/park remain `703ec/70404`.
The NoC fault park is `70ab8`. Each operation requires **exactly half as
many** read responses/requests/accepted IDs and nine 64-byte flits per read,
while still requiring all math tiles. This changes expected byte counts,
not the strength of completion/idle/park proof. L1-tail dirtiness is explicit
and can be restored only after proven completion/stop, never on uncertainty.

The separate ioctl is `54c3`, capability bit 2048 (candidate capabilities
4095), CPU profile format 5. All previous request ABIs and expanded modes
remain unchanged. Compact/expanded prepared formats cannot be switched
inside a session; compact requests refuse unsupported or out-of-range shapes.
The added lifecycle checks exercise foreign-FD and inherited-team denial for
both scaled formats without starting computation.

ASan/UBSan passes independent assembled-image equality, all 256 lookup
entries, all five K widths and multiple N tiles/weight bases, delayed NoC,
missing-response parks, omitted decoding/cast/scale/accumulation, quant/scale
corruption, exact half read counts and full independent numerical references.
All **65** lost post-upload compact control writes and explicit timeout,
bad PC/flits/source/stop tests preserve quarantine/no-reset rules. Immutable
cache tests pass both formats, repeated vectors/one upload, multi-chunk
classifiers, original subnormal/negative/zero scales, capacity/transport and
nonfinite refusal. Pristine pinned ggml tests pass each format and reject
wrong arithmetic without fallback. Jam and all 31 diagnostics build.

Native-qualified compact candidate, also frozen locally:

- Driver `7d090204b85f27aa20bf4ba53636314211ca21cb7a837900d236769ecd38a9cc`
  frozen at `/private/tmp/pioneer-q8-compact-final-tools-20261015/tenstorrent_pci_probe.candidate`.
- Projection CLI `4274e91182c88b02d977c83d76c24ae5a8b3771da628f0cff8fc985abc78e969`;
  use `--run-qwen-q8-compact`, never assume the new capability exists.
- Lifecycle CLI `72b4095d0ccc986859e3a58881926bedfd7486aee9af99b35f008f45c1882986`.
- Standalone compact-capable full verifier
  `f8fb01d372e4d15f42d617d9a1c0a9047bab7b0fdff9261fa25d683c04dd2821`
  at `/private/tmp/pioneer-qwen-compact-q8-20261015/`.

After the expanded full run ended cleanly (failure described below), audit
and idle Assistant checks passed. The `f8b836...` driver was backed up in
`/boot/home/pioneer-q8-compact-20261015/tenstorrent_pci_probe.before`, then the
compact candidate was atomically installed and capability 4095 observed
after natural module turnover, without forced unload.

Native 32x1024, 32x3072 and full-2048x1024 projections each passed two
independent vectors, all original block data/scales, exact half read/flit
counts, nonfinite-scale and prepared-format refusal, bit-identical reuse
and clean END (state 2, PCI/host 1/1, cold 0). The 2048-row results have the
same relative L2 `9.13021911e-8 / 9.70029151e-8` as expanded Q8. Its exact
counters are **4096/36864/4096/4096 for 8192 math tiles**. Lifecycle foreign-FD,
inherited-team, duplicate/final-close checks, expanded Q8, original BF16/FP16,
fixed K1024 GEMV and post PCIe/HDMA audit passed. Assistant remains team2705,
idle with the same experimental TinyStories model; no GUI/model switch.

The compact scalar decoder is **slower**, despite halving weight bytes:
the full query's second CPU wall time was **75,016 us**, including 63,423
compute/proof and 6,062 PIO, versus expanded Q8's 29,821 / 18,360 / 6,044 us.
This is an explicit negative performance result, not a speedup. Compact
storage still helps memory capacity and cold upload, but a direct integer
unpacker or substantially better decoder plus multicore/bulk dispatch is
needed for practical throughput. Completed native logs are in the compact
folder; no whole-model compact result is claimed yet.

The standalone verifier additionally has `--benchmark-int8-q8` and
`--benchmark-compact-q8`. Only after its original four-token bootstrap
passes does it run three distinct public requests with up to 16 natural
greedy tokens each. A CPU32 context with prompt batch/ubatch 128 is created
**before** any P100A registration; the card context uses CPU32/batch1.
Every generated card token must match the independent CPU token and pass
the unchanged all-vocabulary cosine/L2 gates; no reference token is fed
to the card to repair divergence. An early reply shorter than eight tokens
does not become a padded rate. Upload/tensor/byte counts must stay fixed
across all requests. Synchronous decode plus greedy sampling wall-time sums
include host/control work, but exclude model load, cold upload, tokenization,
KV reset, logging and reference diagnostics; these exclusions are printed.
Per-request JSON reports TTFT, generated-token samples (excluding the first
token produced by prefill), total time and both rates. Qualification requires
each of three requests to improve both generation and total time by at
least 5%, not just return correct text. This still does not qualify Assistant
integration, longer-context use or the requested 27B model.

The benchmark controller's ASan/UBSan **fake-only** tests cover CPU batching
vs single-vector card dispatch, three-request accounting, greedy/logit failure,
weight reupload, short output, invalid clock/timing, and a mathematically
correct but slower card. All rates in those tests are synthetic, not device
evidence. The compiled real candidate is
`bdb5706f06f7e71370e28b72b471d4f816fc6e3c07df0267f2f9aeec83ea532c`
under `/private/tmp/pioneer-qwen-warm-benchmark-checked-20261015/`.
It has not yet been run on hardware; native timing remains unproven.

The trusted `TT_RUNTIME_Q8` variant now uses exact integer-8 matrix dots
over each **32-value block**, converts sign-magnitude dots with deterministic
`SFPCAST`, multiplies original weight/activation block scales, and accumulates
FP32 results in SFPU registers on the card. No host partial sums or reference
substitution are used. Sixteen original FP16 weight scales occupy the odd
16-K tile's final 32 padding bytes; integer RISC half-to-float conversion
preserves signed zero and subnormal scales. One FP32 activation scale per
block uses the already saved `83000..83fff` gap. Two separate even/odd-column
accumulators are reinitialized for every output tile. The fixed image fits
the original 4 KiB page, with exact phase/park `703ec/70404`, and unchanged
NoC read counts and four integer fidelity phases per 16-K tile.

Primary semantics/encodings: Blackhole
[SFPCAST](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPCAST_IntFloat.md),
[SFPLOAD](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPLOAD.md),
[SFPMAD](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPMAD.md),
[SFPSTORE](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/SFPSTORE.md)
and their linked instruction diagrams. `SFPMAD` is only partially fused;
native comparisons are required, not assumed from the offline FP32 model.

The new ioctl is `54c2`, capability bit 1024 (capabilities 2047), CPU profile
format 4. Its appended scale request preserves every old request ABI.
Prepared formats are immutable, invalid/nonfinite activation scales are
refused before start, and integer/scaled output-DMA combinations remain
unqualified/refused. All prior owner/team, source, owned-host-buffer,
NoC completion/park/idle, unknown-completion quarantine and cold gates stay.

ASan/UBSan passed independent assembly/image equality, all five K widths,
signed/subnormal/zero scales, omitted cast/scale/accumulation rejection,
bounded NoC failure parks, and **65** lost post-upload control-write cases.
The original BF16/FP16, fixed GEMV, profile and integer operand tests passed.
All **31** userspace utilities and Jam driver build passed.

Native real-model projections passed both independent vectors using **all**
Q8 blocks, not zero padding:

- 32x1024 query: relative L2 `1.16206551e-7 / 1.35299361e-7`.
- 32x3072 `blk.25.ffn_down.weight`: `1.01958204e-7 / 1.55807658e-7`.
- Full 2048x1024 query: `9.13021911e-8 / 9.70029151e-8`, zero of 2048
  numerical gate failures each, maximum absolute error `4.76837158e-7`.

The primitive requires relative L2 below `1e-5` and per-row error below
`1e-6 + sum_abs_block_contributions*1e-6`. The full-model graph/logit/greedy
acceptance gates are unchanged. Native nonfinite-scale/prepared-format
refusals reported started 0, and subsequent results were bit-identical.
All END operations restored PCI/host 1/1, cold 0. Original lifecycle,
BF16/FP16 regressions and final PCIe/HDMA audit passed; Assistant unchanged.
The full query's second CPU wall-time total was 29,821 us, including 18,360
compute/proof and 6,044 PIO. **This is not a CPU speedup.**

Driver SHA-256 `f8b8360583853f43a4cacaa4f662e44047ec9fd4b37cb907e80653a2bf0d85fb`;
projection CLI `1d7c5c05989d3de013889d6ee945883667639edd9fe285f5ff6bc80c0af9a7f1`.
Native logs and original `dcc775...` driver backup are in
`/boot/home/pioneer-q8-scaled-20261015/`.

The standalone ggml adapter now retains original Q8 signed bytes/scales
without weight dequantization, uses pinned GGML activation quantization
once, and dispatches this scaled operation. Its offline pinned-ABI tests
passed two vectors/one upload, original-byte packing, immutable format and
wrong-math/no-fallback rejection. The expanded F16-KV native full-model run
with binary `2430136a355a7cb8e193015dc4df7a81199ead5e7dc9c3f1702d8538df278f72`
**ended in a numerical failure, not a hardware fault**. It retained all 197
matrices / 1,340,964,864 bytes / 327,384 pages. Prompt positions 0..4 passed
all-vocabulary logits with relative L2 `1.6e-8 / 1.7e-8 / 1.1e-8 / 1.7e-8 /
1.6e-8`, matching argmax each. The original layer25 FFN-down failure was
passed (position0 graph input/output L2 `3e-9 / 1.6e-8`).

At prompt position **5**, the first noticeable graph difference appeared
after layer13 attention (`blk.13.attn_output.weight` input/output L2
`0.000020351 / 0.000038141`), then quantization amplified it in subsequent
FFNs. The unchanged per-layer gate stopped at `blk.19.attn_v.weight` graph
output L2 **0.020091322**. The independently checked card primitive still
had L2 `1.06e-7`; applying the same CPU Q8 activation emulation to that
diverged input gave `0.020091430`, so a bad matrix product is not supported
by this trace. It generated **zero** accepted reply tokens. The 1,305 real
matvec calls ended with explicit END2, PCI/host1/1, cold0, followed by a
clean audit and unchanged Assistant. Total 1,787.818 s, of which
1,728.703916 s was weight preparation/upload, 45.548620 matvec/staging/result,
3.270967 primitive diagnostics. Failure vector/log are preserved locally
and in `/boot/home/pioneer-q8-scaled-20261015/`.

The matching pinned native CPU library disassembly confirms
`ggml_vec_dot_q8_0_q8_0` uses a separate scale multiply followed by true IEEE
`fmadd.s` at `0x40194 / 0x4019c`; its quantizer tail-calls
`quantize_row_q8_0_ref`. Blackhole SFPMAD is only partially fused. Tiny
rounding changes crossing half-precision attention/KV boundaries are a
**hypothesis**, not a proven cause. A new explicit precision experiment,
`--diagnose-compact-q8-f32-kv`, sets **both** independent contexts' K and V
cache types to FP32 and prints that choice. Original graph/logit/greedy
acceptance limits remain unchanged. It additionally reports full pre-cache
K/V projection FP16-rounding disagreements; K normalization/RoPE occur later,
so this diagnostic alone cannot prove the actual stored-key difference.

The FP32-cache experiment with binary
`24ae9f4c32222a3856e525d9c44ab9fa5aebeaf291ac8198190e4321ec1ccf1c`,
named `tenstorrent_haiku_qwen_f32_kv`, log
`/boot/home/pioneer-q8-compact-20261015/full-qwen-f32-kv-first.log`,
ended with failure after positions0..2 passed all-vocabulary logits at
relative L2 `1.5e-8 / 1.7e-8 / 1.1e-8`. At position3, the first input jump
was layer15 attention (`0.000021627`), and the unchanged graph gate stopped
at `blk.22.attn_output.weight` output L2 `0.020480192`. Primitive L2 remained
`1.07e-7`, with CPU Q8 emulation on the diverged input `0.020480352`.
It retained197 tensors / 670,482,432 bytes / 163,692 pages, made860 real
matvec calls and generated0 checked reply tokens. Total952.283s, weight
preparation/upload866.928562s, matvec/staging/result75.845823s. END2 restored
PCI/host1/1/cold0 and the independent audit passed. Thus FP32 cache precision
alone does not resolve the drift; the F16-cache hypothesis is not established.
The new optional `--trace-native-q8` diagnostic separately observes named
attention intermediates in CPU/reference and card contexts, keyed by
name/op/shape, and reports Q8 input byte/scale differences. Per-position
references are cleared so stale captures cannot masquerade as a complete
new reference. Observers never replace actual output or weaken graph/logit/
greedy gates; missing/ambiguous trace references stop the diagnostic. The
native trace binary `2413f6aa7ba0607711140028a2e865df5e85be793383afec3fdf78880588f3d5`
completed with a numerical failure, log
`/boot/home/pioneer-q8-native-20261015/full-qwen-native-trace-first.log`.
Positions0..4 passed all-vocabulary logits at L2~1e-8; warm prompt decodes
took8.56..8.60s, not accepted reply-token rates or a CPU speedup. At position5,
Q/K raw projection inputs still quantized identically to the CPU through
layer13. Tiny Q/K norm/RoPE changes preceded `kq-13` L2 `3.52e-7` and
softmax `5.06e-7`. `kqv-13` then changed128/2048 values at L2 `2.035e-5`.
The first learned-projection activation quantization difference was exactly
**one scale block, zero quant bytes** at `blk.13.attn_output.weight`.
Later quantization amplified this; the diagnostic stopped at the original
`blk.19.attn_v.weight` graph L2 `0.020091322`, while the checked primitive
still passed. No learned-product fallback or substitution was used.
This localizes the first amplification but does not prove SFPMAD or one
specific F16 attention conversion is the sole cause. It generated0 checked
reply tokens after1305 real matvecs, retained197 tensors/670,482,432bytes,
total934.885s; preparation/upload877.973895s, matvec/staging/result42.785727s.
END2 restored PCI/host1/1,cold0, and independent post-audit passed; Assistant
remains unchanged. Legacy BF16/FP16/scaled-Q8/compact/fixed-GEMV regressions
and updated lifecycle had passed before this trace.

The separate `--probe-native-q8-reference MODEL [TENSOR]` fast diagnostic
runs only the first six public prompt inputs in an independent CPU context,
then replays one captured learned projection on the card. The CPU model and
immutable weight tensor remain alive; separate host buffers hold the actual
card output. Full independent primitive/graph comparisons and explicit END
still apply. It is not full-model generation or a timing qualification.
The K13 replay passed with exactly matching CPU input:148/1024 changed FP32
values, maximum255ULP near small values, full-output L2 `1.92434544e-9`, and
zero raw projection FP16-rounding changes. END2/PCI-host1/1/cold0. Original
binary `6bf70835d855adbf94b27a3dd97f6bd018ccd1eb9ae21b1102c93a77177cb97b`.
The extended observer additionally rebuilds a private CPU norm/RoPE graph
using actual card output. A CPU-reference-input control must reproduce the
original CPU pipeline bit-identically before its half-boundary diagnostics
are accepted. Neither graph mutates independent reference or Assistant data.
The first extended Q13 trial's **card projection** passed, but its private
CPU-clone control failed; no conclusion about post-RoPE rounding was accepted.
The position input had to be snapshotted during the original callback rather
than reread after the whole CPU graph reused its scratch buffers. After that
fix, `--probe-cpu-rope-reference` passes bit-identically on Haiku, using only
explicit CPU registration (no P100A backend/open/ioctl). The corrected native
Q13 replay, binary7f1ee15e..., passes that bit-identical control. Its actual
projection differs in407/2048 FP32 values at L2 `2.62849577e-8`, with no raw
projection half changes. After private CPU norm/RoPE, exactly one half boundary
changes: Qcur13 index1059, `-0.0724792629/aca4` versus CPU `-0.072479181/aca3`.
END2/PCI-host1/1/cold0 and post-audit pass; Assistant remains idle/unchanged.

The explicit `--probe-native-q8-rounding` diagnostic, binary
`0ac83f85f18f816a7d1648b82d73aed5770853066df29664b0fbd6f80b661120`,
compares a finite-Q8 specialization of Tenstorrent's Apache-2.0
[bit-perfect FMA model](https://github.com/tenstorrent/tt-isa-documentation/blob/main/Miscellaneous/FMA/fma.c).
It predicts **all2048 actual card FP32 bits exactly**, including407 CPU
differences and the same post-RoPE half crossing. Independent IEEE simulation
matches all2048 CPU bits. A CPU-only compensated/round-odd SFPU candidate also
matches every CPU bit and removes that crossing. Partial accumulation is thus
established for this captured projection, not merely assumed from its ISA.
This does not establish a full-model fix. END2 and post-audit pass. Public
log: `focused-native-q13-rounding-first.log` in the native test folder.
`--probe-cpu-q8-rounding` explicitly registers only CPU and never opens P100A;
simulated vectors never enter the real model graph/output. These diagnostics
do not accept generated tokens.

The **existing** `--verify-native-q8` contract was tested separately
with the already qualified native-FP16-scale driver and binary2413f6...,
log `full-qwen-native-verify-first.log` in the native test folder. CPU32
reference is created before P100A registration. Unlike diagnostic mode,
verify has always checked all vocabulary logits (cosine>=.999, L2<=.02) at
every prompt/generated position and requires four CPU-identical greedy reply
tokens, rather than adding the per-layer diagnostic comparisons. Those
thresholds and no-fallback rules are unchanged. The prior diagnostic failure
is not reclassified as passed. This tests whether its intermediate drift
actually violates final-output quality; it is not a speed benchmark. The run
failed at position5: cosine `.999685102` passed, but all-vocabulary relative
L2 `.025594734` exceeded the unchanged `.02` limit. Argmax still matched897/897;
zero reply tokens were accepted. It made1404 matvecs and retained197 tensors/
670,482,432bytes/163,692 pages. Total1169.759s, preparation873.078168s and
matvec/staging47.568583s. END2/PCI-host1/1/cold0, post-audit and unchanged idle
Assistant all pass. Final-output quality is affected; that failure is not waived.

Explicit host1 modes keep the independent CPU reference at32 threads and
change only the card context's host glue to1. Both thread counts are checked
and reported. Warm benchmark JSON records the card host count and preserves
the CPU32/batch128 baseline and existing gates. This targets the large host
thread overhead in verify32, not the rounding failure. Exact-scale backend/
cache wiring is a distinct immutable mode; missing capability, invalid
combinations, -128 weights and nonfinite scales fail without fallback. Its
focused replay additionally requires **every FP32 output bit** to match the
independent CPU. The compensated kernel's full bootstrap verification passes
above; matched faster-than-CPU performance remains unqualified.
The new compensated run above now establishes four checked Qwen reply tokens;
the earlier partial-SFPU failure remains a failure. No CPU speedup has yet
been established.
The runtime still uses one core and bank0 / 2GiB. Compact mixed quantization,
wider models, multicore/bulk dispatch and matched repeated benchmarks remain
required. Assistant is not changed to expose an unverified faster-Qwen mode.

#### Native integer-8 arithmetic candidate (not scaled Qwen inference)

The failed floating-point integer probe below motivated a separate fixed
integer matrix mode, ioctl `54c1`, capability bit 512 (support capabilities
1023). It uses the documented integer-8 source overlay through the FP16
unpacker, sets `ALU_ACC_CTRL_INT8_math_enabled`, and reads the destination
through converted signed-int32 RISC access. The independently assembled
image differs from the FP16 image by exactly two more fixed immediates.
Addresses, NoC commands, K/N loops, all four fidelity phases, completion
markers and park PCs remain unchanged. There is no caller-provided code.
See the integer path in the primary [MVMUL specification](https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/MVMUL.md)
and Blackhole's [RISC destination access](https://github.com/tenstorrent/tt-isa-documentation/blob/main/BlackholeA0/TensixTile/TensixCoprocessor/Dst.md#riscv-access-to-dst).

Operands are explicitly bounded signed-byte encodings; invalid inputs and
mixed prepared formats are refused. This operation returns integer bits,
never interprets negative integers as FP32 NaNs, and refuses the unqualified
integer/result-DMA combination. `--probe-qwen-q8-int8-engine` reads real raw
Q8 integers, but still zero-pads all but the first 32 terms and **does not
apply block scales**. It is neither a quantized model backend nor inference.

ASan/UBSan covers exhaustive 16-bit encoding validity, signed-byte round
trips/refusal, signed-int32 output handling, all lost control writes, format
lifetime, and exact integer-phase execution with stale-Dst refusal.
Independent assembly equals the fixed image; legacy BF16/FP16 images and
GEMV variants still pass. Jam and all 30 utilities build.

The first native 32x1,024 test passed both independent input vectors with
**zero of 32 mismatches each**, cosine 1 and relative L2 0. Exact NoC
128/1,152/128/128, completion/park/stop and clean explicit END passed, cold 0.
The 32x3,072 test of `blk.25.ffn_down.weight` also passed both vectors with
zero mismatches and exact 384/3,456/384/384 counters. The preceding
floating-mode version failed 30 of 32 rows on the same first vector, so its
result has not been relabeled an integer proof. The full 2,048-row query
shape then passed both vectors with zero mismatches and exact
8,192/73,728/8,192/8,192 completion counters. Applying block scales, compact
quantized storage and full-model testing remain required before integration.

Native invalid signed-byte encoding and attempted in-session BF16 format
change were then refused with **started 0**, cold 0. A subsequent integer
operation reproduced all prior outputs exactly. Explicit END, ownership/
inherited/dup-FD/final-close checks, both old floating formats, fixed GEMV
and final PCIe/HDMA audit all passed; the Assistant was idle/unchanged.

Previous integer-only candidate SHA-256:
`dcc775acf96c7c1533f14ae3ca6018a84fc0f6664727862cdf5723e11f264b31`;
CLI: `450b23263e3a8125cdb1662568a921745e34b78e953bb50ca783d578eff6362f`.
The separate refusal/reuse diagnostic is
`2715c4117b1c12a061b8423afb6fd550f6ff2701cf8fa86469326643ea4f6821`
(`tenstorrent_haiku_int8_refusal_check`).
Native logs and the verified previous-driver backup (`b3d271...`) are in
`/boot/home/pioneer-int8-engine-20261015/`.

#### Fixed FP16 precision candidate (projections passed; full model failed)

A native-verified timing interface adds a separate, read-only per-session CPU
phase record, ioctl `54c0`, capability bit 256 (candidate capabilities 511).
It preserves the existing matvec request/result ABI and all compute/DMA
gates. Integer `system_time()` stamps separate one-time preparation, staging,
configuration, compute plus completion proof, PIO collection, verified stop,
optional result DMA and final checks. These include guards/polling and are
**not device-cycle or device-only throughput measurements**. Records include
format, sequence, completed-stage mask and final status; clock regressions
invalidate timing evidence without changing device execution.

The query is root/FD/creator-team-owned, requires the same live exclusive
session and cold-precedence lock, and copies only CPU metadata. It has no
MMIO/DMA/reset/firmware operation and does not bypass a cold latch. The
projection CLI checks complete masks, status, sequence and monotonic stamps;
its lifecycle test adds foreign/inherited-FD refusal and final-close coverage.
ASan/UBSan phase/clock/partial/frozen-record tests and all existing dropped
runtime writes pass, as do the independent compute images, ownership and
output-DMA regressions. Jam and all 30 diagnostics build. **Native timing and
new metadata ownership checks passed** after the exclusive full-Qwen test
ended. Both result paths and FP16 metadata passed their unchanged projection
gates, including exact PIO/DMA comparisons. Ownership/inherited-team/dup-FD/
final-close tests, legacy GEMV and post-test audit passed, cold 0, Assistant
unchanged. The timing-only driver was installed as `b3d271...`, then preserved
before the integer-mode experiment above.

Warm full query-projection timings (CPU microseconds, all guards included):
preparation 1, staging 3,318, configuration 750, compute/completion proof
14,552, PIO 6,052, stop 211, final checks 1; total **24,885 us**. Matched
owned result DMA took 6,326 us and 25,166 us total. Compute and host/control
overhead both matter; result DMA is not a speedup. Evidence is in
`/boot/home/pioneer-runtime-profile-20261015/`.

Candidate driver SHA-256:
`b3d271236719b3fc663e3cc0cf6ee98fa14506a3f10a440517590c06064feace`.
Diagnostics are in `/private/tmp/pioneer-runtime-profile-tools-20261015/`.

The remaining BF16/CPU-Q8 graph drift motivated a separate trusted FP16
operand matvec, ioctl `54bf`, capability bit 128 (support capabilities 255).
It changes exactly six fixed source-format immediates in the independently
assembled runtime image. No code/data address, NoC command, shape, counter,
phase/park PC, loop or FP32 destination format changes. Prepared format is
immutable for a session; a mismatched-format invocation is refused rather
than replacing live code. The original BF16 and optional result-DMA calls
remain available and unchanged by default.

The format fields were checked against pinned Blackhole
[configuration definitions](https://github.com/tenstorrent/tt-metal/blob/d279aa36bebe71367a521449310f6f53100e92be/tt_metal/hw/inc/blackhole/cfg_defines.h).
FP16 carries more input mantissa bits than BF16 but is not an IEEE-exact
matrix path: the documented four fidelity phases consume all ten SrcB
mantissa bits and only nine SrcA bits. The independent reference explicitly
models that omitted bit; see the shared architecture-specific discussion in
[SrcA/SrcB fidelity phases](https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/SrcASrcB.md).
The application packs normal FP16 round-to-nearest-even and explicitly
flushes subnormals, refusing overflow/nonfinite operands. No kernel floating
point is used. The full-model candidate still preprocesses activations with
the independent CPU runtime's Q8 quantizer before operand packing.

ASan/UBSan covers exhaustive finite normal FP16 round trips, nearest-even
ties, signed zero, subnormal flush and overflow; independently assembled
image equality (only six changed words); all five K widths; unchanged legacy
images/GEMV variants; packing/cache reuse; nonfinite/overflow refusal; format
immutability; all lost post-upload control writes; and existing no-fallback
ggml failure/cleanup policy. The driver and all 30 utilities build.

Native FP16 tests passed for 32x1,024, full 2,048x1,024 query, 1,024x3,072
down and 4,096x1,024 classifier projections, each with two independent
vectors and clean END. Exact compute completion/park/counters and original
CPU numerical gates remain unchanged. On the 32-row query sample, relative
L2 against the original Q8 FP32 product improved from BF16's
**0.001926905 / 0.001968701** to **0.000590074 / 0.000520048**.
This is evidence for a precision improvement, not full-model correctness
or a speedup. Stricter 1e-5 primitive discrepancies are still printed, not
discarded. Legacy ownership/final-close/GEMV tests and the hardware audit
passed afterward, cold 0, with the Assistant unchanged.

The standalone backend supports explicit `--diagnose-fp16-cpu-q8` /
`--verify-fp16-cpu-q8`, keeping the CPU reference context created before
device registration. The native full-model diagnostic failed at
`blk.25.ffn_down.weight` with the same primitive/graph/full-logit/greedy gates:
graph input/output L2 **0.022267011 / 0.030230597**, versus primitive
FP16-phase L2 0.000368990. It accepted **zero Qwen tokens** after 182 tensors /
920,125,440 bytes / 224,640 pages and 1,196.102 seconds. Weight preparation/
upload consumed 1,187.743920 seconds, versus 4.777610 seconds for all matvec/
staging/result calls. Individual FP16 improvement did not qualify full
generation. Explicit END and the post-test PCIe/HDMA audit passed, cold 0,
with the Assistant unchanged.
Its log/failure vector are `full-qwen-first.log` /
`full-qwen-failure-first.raw` under `/boot/home/pioneer-runtime-fp16-20261015/`.
Do not deploy this as a faster Assistant mode based on projection evidence.

Driver used for that FP16 test, SHA-256:
`228885f27d567a83ffa034bb97d9e21c728dbdc83e53ad40dd89b50029e11fea`;
projection CLI:
`fb31741d50db7fbbdbf8bd8e34e3e1dd8b2a9c7f9d5675093bc96695e6206726`;
full-model diagnostic:
`18cc5958274b1a8876c892d8020ec5ea45904fa58847a3498aeafcc9d6d7f425`.
The previous driver (`d02da2...`) is preserved as
`/boot/home/pioneer-runtime-fp16-20261015/tenstorrent_pci_probe.before`.

For the next quantized-kernel experiment, the projection CLI has an explicit
`--probe-qwen-q8-integer-block` mode. It reads the first 32 raw signed Q8
integers from each real model row, uses exact BF16 integer operands with
zero padding, and requires every resulting 32-term dot product to equal the
independent integer reference exactly. The remaining original numerical,
completion and cleanup gates still apply. No scales are applied: this is
**not a Q8 backend or full inference**. Offline image execution passes four
mixed-sign integer-vector trials, and all tools cross-build. The native test
completed after the FP16 test: **30 of 32 rows failed exact equality** on the
first vector, maximum absolute error 8, relative L2 0.000205409. Completion
and cleanup still passed, with a clean audit/cold 0. This floating-mode probe
is not an exact integer proof, and is retained as a failure. The integer
arithmetic candidate above is separate, not a relaxed gate.

#### Checked output DMA (historical projection comparison, not stronger readback proof)

**Audit correction:** the originally tested helper did not inspect fresh L1
return/original output words. The source fix and stronger offline fault tests
are documented above; they add conservative PIO and are not deployed. Do not
infer full readback-integrity qualification from the older samples below.

A separate data-only matvec ioctl `54be`, capability bit 64 (candidate
support capabilities 127), collects results through the session's owned
4 KiB host page. The original `54bc` PIO matvec is unchanged and remains
available for independent comparisons. No arbitrary addresses or device
code are accepted, and the request/result ABI is unchanged.

Only after the original phase/tiles/PC/NoC completion proof and both verified
reset holds does the candidate transfer each fixed output page from
`0x84000..0x87fff`. All host words are first poisoned with `0xffffffff`;
an ordered host read establishes that poison in scratch L1. The output
write must replace every host word (unused words must match the pre-staged
`0x7fc00000` padding), followed by a fresh ordered read to a distinct L1
destination and a second full CPU comparison. Each of the three commands
requires exact 1/64/1/1 completion deltas and all existing source/mapping/
route/idle/PCI guards. Posted-write ACK alone is never accepted. Ambiguous
visibility, ordering or completion quarantines the page and latches cold
recovery; no reset, retry or payload cleanup follows uncertainty. Finite
validation still occurs in the caller's existing integer-bit checks.

ASan/UBSan covers all four pages, partial-page padding, counter wrap,
delayed/missing/partial host visibility, late mutation, unsafe core state,
all lost poison/control writes and incomplete commands. Integrated runtime
tests additionally drop every post-upload control write (87 DMA cases),
require a verified stop before output DMA and rejected any PIO fallback at
that stage. The stronger current baseline uses explicit source/readback PIO.
The candidate builds through Jam and all 30 tools cross-build. The new CLI
`--compare-qwen-resident-output-dma` executes two independent input vectors
through both paths, requiring bit-identical outputs as well as the original
independent CPU numerical gates.

Native tests passed for 32x1,024, 2,048x1,024, 4,096x1,024 (the tied
`token_embd.weight` classifier), and 1,024x3,072. Both independent vectors
were bit-identical to separately executed PIO results at every output.
All original CPU numerical gates passed, with exact compute counters and
clean explicit END. Ownership/inherited-team/duplicate-FD/final-close tests,
the fixed K1,024 GEMV regression and the PCIe/HDMA audit also passed; the
Assistant stayed idle/unchanged. The first 4,096-row attempt requested an
absent `output.weight` name and exited before starting a session; the
correct tied tensor was then selected from the existing full-model log.

Warm validation-inclusive PIO / DMA samples were **24.563 / 24.864 ms**
for 2,048 outputs, **46.335 / 46.895 ms** for 4,096 outputs, and
**29.123 / 29.313 ms** for K3,072. The new path is therefore **not a
demonstrated performance gain** and is not automatically selected by the
Qwen adapter. Removing per-word payload reads alone does not remove the
transfer/guard overhead or single-core compute bottleneck. No token-rate
claim follows from these samples. Full-model numerical acceptance remains
unmet independently of this exact result-transfer proof.

Artifacts/logs and the verified previous-driver backup are in
`/boot/home/pioneer-output-dma-20261015/`. Installed driver SHA-256:
`d02da23388c2a55b946d5672535adea06504af01ef13d93a1c0b782f31cf5c57`;
comparison CLI: `5ed87587ac3a077f4eafa0a788e853646aa843baee2011082778eed11118a108`.
The previous driver (`c729f0...`) is preserved as `tenstorrent_pci_probe.before`.

The operational upload now has a separate root/FD/team-owned data-only
ioctl `54bd`, capability bit 32 (support v1, capabilities 63). The original
PIO-checked storage API is unchanged. It still accepts exactly 4 KiB and a
validated arena offset, never supplied host/NoC addresses or code.

The six-step chain is: owned host to L1 staging; acknowledged write to
owned GDDR; read into distinct L1; poison the host page with the complete
bitwise complement and read that poison into the return region; write the
GDDR readback to host; require every CPU-visible word to match the original
before a fresh ordered PCIe read into L1 distinct from the write source.
A second complete CPU comparison follows that read's proven completion.
Unlike the initial transfer diagnostic, this operational chain does not
PIO-scan each payload word in L1; its data-integrity evidence is the complete
GDDR-to-host return loop. It still requires source/route/mapping/PCI/idle
guards between steps, exact counters **1/64/1/1** for every request, BME-off
proof, and quarantine/cold recovery on missing visibility or completion.
No core starts, counter clears, firmware writes or uncertain-payload cleanup
were introduced.

Native testing passed twice with the two pinned Qwen-file chunks, three
uploads (including overwrite), four independent legacy PIO GDDR reads,
zero mismatches, page count two and clean explicit END. The new operation's
out-of-arena/reserved-field refusal, foreign-FD conflict and inherited-team
denial also pass. Existing lifetime/final-close tests and the old K1,024
GEMV regression still pass, with clean post-test PCIe/HDMA audit and the
Assistant unchanged. Warm checked 4 KiB stream ioctls took **5.218–5.224 ms**,
versus **8.115–8.129 ms** for the matched legacy uploads; first-operation
setup costs were 11.597 / 14.527 ms respectively. These are validation-inclusive
samples, not a bandwidth benchmark or Qwen token-rate improvement. Larger
owned bulk transfers remain necessary for practical model loading.

Artifacts/logs and the verified previous driver backup:
`/boot/home/pioneer-stream-upload-20261015/`. SHA-256:

- Driver: `c729f0d526229a3518cef37501e85a1560324b8050a71ef40d2e936afa5256b2`
- Timed stream/lifecycle CLI: `cbb89e4a1dfd90b4d2586277eda24c892affa467203377ec71ea29c2c558fae8`

ASan/UBSan tests cover all lost control writes and both full sets of host
writes, all six missing completions, stale/corrupt payload, changed source/
mapping, pending PCIe state and BME-off failure. They assert no per-word
L1 payload reads during the operational upload and no cleanup/retry after
uncertainty. The driver and all 30 diagnostics build.

The subsequent real-activation primitive trace checked 64 rows per learned
matrix through layer 10: device output remained close to an independently
decoded BF16 reference, and the two independent Q8 decoders agreed. This
has not explained the failed full-model logits below. That diagnostic was
stopped with SIGINT to replace it with a CPU-graph comparison; automatic
final-FD cleanup left a clean audit, cold 0, and the Assistant unchanged.
A more targeted tracer captures every learned CPU reference input/output
before card execution, checks full reference coverage, compares activation
and output drift, and separately emulates CPU Q8 activation quantization.
It exclusively creates a failure-vector file for short reproductions,
never overwrites one, relaxes no numerical gate and substitutes no output.
The first CPU-graph trace localized divergence in layer zero without a
primitive transport/decoder failure. GPU versus independent BF16 L2 was
0.000090148 / 0.000323759 / 0.000211562 for Q/V/K. But GPU versus the CPU
graph was 0.003497376 / 0.010499444 / 0.003004001, with identical original
inputs. Independently quantizing activations to Q8 reproduced the CPU graph
within L2 **2.41e-7 / 7.71e-7 / 7.61e-7**. Thus the CPU's activation
quantization is a concrete arithmetic difference, not an initialization or
DMA failure. By attention-output, input/output drift reached
0.015584076 / 0.020216687 and the unchanged 0.02 graph diagnostic stopped.
The 8 KiB FP32 vector was saved as `graph-failure-input-first.raw`, and END
again restored/released everything with cold 0.

An explicit `--diagnose-cpu-q8` candidate now preprocesses each input with
the same Q8 quantizer before BF16 device input. Its native trace passed
through layer 24, but stopped at `blk.25.ffn_down.weight`: graph input/output
relative L2 reached **0.022495117 / 0.030109908**, above the unchanged 0.02
limits, while the independent BF16 primitive error remained **0.000291272**.
This is not a verified full-model fix. Zero Qwen tokens were accepted after
182 tensors / 920,125,440 bytes / 224,640 pages and 1,199.277 seconds.
Explicit END restored/released everything, cold stayed 0, the post-run audit
passed and the Assistant remained idle/unchanged. The failure log and 12 KiB
input are `graph-cpu-q8-first.log` / `graph-cpu-q8-failure-first.raw` in the
native Qwen test directory. Further precision work remains necessary;
none of the reference values replace device output.
CPU reference capture is single-threaded for diagnostics to avoid repeatedly
creating 32-worker pools at every tapped node; throughput verification remains
a separate task. The reference context is still created before device
registration, and no reference value supplies card output. Strict primitive,
graph, full-logit and greedy-token gates remain in place. Full Qwen generation
and speedup remain unverified.

The standalone CLI also has an explicit `--verify-cpu-q8` path (same full
logit/token gates, without the expensive primitive taps) and reports separate
backend weight preparation/upload/cache, activation preprocessing,
matvec/staging/result and diagnostic wall-time totals. These do not claim
device-only timing or a benchmark. The new options build and the pristine
pinned ggml ASan/UBSan tests pass; they have not bypassed the failed native
diagnostic or qualified generation.

### October 15: standalone Qwen backend candidate, first full forward fails

A standalone experimental ggml ACCEL backend now connects retained weights
to the pinned llama runtime, outside the user-modified llama.cpp checkout.
It accepts immutable learned Q8_0 matrices and single F32 input vectors,
decodes/packs a bounded 16-row strip at a time, retains each uploaded tensor
and splits classifier output into at most 4,096 rows per device call. Input
vectors change without re-uploading the weights. CPU handles normalization,
attention/KV, activations and embedding lookup; selected matrix operations
never fall back to CPU after a device error. Unsupported graph operations
are explicitly scheduled to CPU. This is a prototype, not a verified
faster-than-CPU Assistant mode or support for the 27B hybrid graph.

`tenstorrent_haiku_qwen_resident --verify-qwen MODEL` creates its independent
CPU context **before** registering the P100A device, then creates the card
context over the same immutable model. It compares every forward's full
logit vector (cosine >= 0.999, relative L2 <= 0.02) and requires exact greedy
token agreement at generation, never replacing a card result with the
reference. The final verifier also requires all seven learned matrices per
layer plus the classifier to be cached, with exact uploaded-page accounting.
The separate native test leaves the open Assistant and its conversation
unchanged. Four checked greedy output tokens are the first target, not a
benchmark. The steady decode diagnostic excludes CPU-reference time, but
three samples are insufficient for a speedup claim.

The first complete-model native run **failed numerical verification** on
position zero. It retained all **197** learned matrices (28 x 7 plus tied
embedding/classifier), **1,340,964,864 bytes / 327,384 pages**, and completed
**234 real matvec calls** (196 layer projections plus 38 classifier chunks).
But full logits had cosine **0.972931802**, relative L2 **0.264345575**,
despite matching argmax **1479/1479**. No generated token was accepted or
substituted from CPU. END restored PCI and released the host page, state 2,
cold 0; the subsequent read-only PCIe/HDMA audit is clean, and Assistant
team 2705 remains ready/idle and unchanged. **Do not weaken the logit gate.**
The next numerical test must localize the first divergent primitive/layer
with real activation vectors and independent BF16/Q8 references.

Initial decode took **2,717.271042 seconds**, including the full verified
load; total test time was **2,724.992 seconds**. The old upload's two complete
per-word PCIe scans dominate this first load. There is no generation-rate
result. The separately qualified streamed-upload path above preserves full
CPU payload checks, ordered readback and the existing cold/quarantine gates
while shortening subsequent loads. No other card
operation or driver change was made during the now-finished exclusive run.

Native run directory: `/boot/home/pioneer-qwen-resident-20261015/`.
The first loader attempt lacked `/boot/system/lib` in `LIBRARY_PATH` and
failed before any device access; the next attempt uses the existing
Assistant's library path and is logged as `qwen-first-loaded.log`.
Failed native prototype SHA-256:
`22318635b6e27136e96bcda27766c60375344184f93489032efbf8c4f0d1d7da`.
The final stricter coverage-check binary (not substituted into the running
process) is `d2da38de247766a9d83257b6fe1c26846c0e4a885dc0cd5fc64f17fab90cbe48`.

Offline tests pass tensor layout/reuse, different vectors, a 4,112-row
classifier split, capacity refusal, changed identities, nonfinite data and
transport failures. A test with pristine pinned ggml checks actual backend
registration/support/graph execution, two vectors with only one weight
upload, exclusive instance lifetime and failed graph/no CPU fallback.
That test uses a clearly separate software-only transport: it is not a
card performance result. New code is tested with ASan/UBSan and halt-on-error;
only pointer-overflow instrumentation in upstream `ggml.c` is excluded for
its intentional null-based `ggml_graph_nbytes` size calculation. The pristine
upstream source is archived to a temporary test build, never edited in the
user's checkout. Native production builds never enable the test transport.

Reproduce build/offline tests:

```sh
sh tools/pioneer/tests/run_pioneer_resident_tensor_tests.sh
sh tools/pioneer/tests/run_pioneer_ggml_backend_tests.sh
sh tools/pioneer/build_qwen_resident.sh CONFIGURED_BUILD MATCHING_NATIVE_LIBS FRESH_OUTPUT
```

The native build verifies the pinned revision/API headers and exact hashes
of all four matching Haiku libraries. All 64 CPUs, RAM, boot/recovery,
firmware and the open Assistant remain outside this standalone deployment.
The clean END/audit has now been established, but this candidate is not
numerically functional yet. Continue with scoped primitive diagnostics;
full Qwen generation and speedup remain unfinished.

### October 15: real Qwen projections over retained device weights

The exclusive session now executes a **data-only resident matrix-vector
operation**, ioctl `54bc`, with capability bit 16 (support v1, capabilities
31). Shapes are K = 1,024 / 2,048 / 3,072 / 5,120 / 6,144 and N = 16–4,096
in multiples of 16. The driver checks all referenced weight pages are
initialized, validates arena offsets and finite input, and accepts neither
caller code nor host/NoC addresses. This is a primitive for a model backend,
not full Qwen inference or a faster-than-CPU hardware mode.

Weights use canonical 576-byte blocks: a 16-byte header, 16x16 BF16 weights
and 48 bytes of padding. Inputs are separate BF16 vectors in fixed L1
`0x80000`; FP32 output bits use `0x84000`. TRISC0 itself loops over both K
blocks and output tiles, retaining the accumulator across K, clearing it
between output tiles and reading each weight block from owned bank-0 GDDR.
All four fidelity phases remain enabled. There are no CPU partial sums,
per-input-block host doorbells or weight re-uploads between vector requests.
Only physical tile (1,2), TRISC0, runs; this is not multicore acceleration.

On first compute the session saves its fixed boot/code and 32 KiB vector/
output scratch once, rechecks source/idle state, and verifies the uploaded
fixed image. Before each run it stages/poisons input and output pages using
the owned-host path, verifies BME-off and bounded command setup, then
initializes the idle coprocessor and starts only TRISC0. Completion requires
the exact phase/tile count/MAT1 marker, two parked PC samples at `0x70400`,
empty command/ID state, stable routes and exact device-read counter deltas
`tiles / (9*tiles) / tiles / tiles`. Only proven completion authorizes
output reads and the verified stop/reset sequence. Missing completion,
unexpected PC/counters/source or a failed stop quarantines the host page
and latches cold recovery, without a reset, retry or L1/NIU cleanup.
Explicit END restores the saved code/buffers before storage scratch and
normal PCI cleanup. Firmware and boot configuration are unchanged.

The new read-only bounded GGUFv3 loader selects a Q8_0 tensor, skips typed
metadata without interpreting templates, and checks header/count/string/
alignment/type/dimension/file bounds. It is not a general GGUF authenticator
or all-tensor overlap validator. The installed Qwen3-0.6B-Q8_0 model's
previously recorded full SHA-256 was reverified before this trial. Userspace
decodes and packs weights once, uploads every page with full readback
verification, runs two different vectors and independently computes CPU
BF16 and original Q8/FP32 references. The uploaded-page count must remain
unchanged after each vector. Neither reference supplies device output.

The first 32-row trial **failed the strict numerical check** on 5 outputs,
despite clean execution/restore. Its log is preserved as
`qwen-projection-first.log`. This is not bit-exact IEEE FP32 reduction. The
prior mixed-exponent tile tests already established the explicit empirical
BF16 target `abs(error) <= 1e-6 + sum(abs(products))/1024`; the runtime CLI
now applies that same target while continuing to report strict failures.
This is not a newly fitted or universal error bound, nor a diagnosis of the
exact rounding mechanism. The official
[MVMUL functional model](https://github.com/tenstorrent/tt-isa-documentation/blob/main/WormholeB0/TensixTile/TensixCoprocessor/MVMUL.md)
also cautions that floating-point reduction order and precision need not
match the illustrative model.

Native tests passed both different vectors, with zero failures against the
existing BF16-level target and cosine > 0.9999 / relative L2 < 0.01 against
the independent original-Q8 FP32 result:

| Real Qwen tensor | Shape N x K | Uploaded once | Second-vector ioctl time | Worst Q8 FP32 relative L2 |
|---|---:|---:|---:|---:|
| `blk.0.attn_q.weight` sample, repeated fresh session | 32 x 1,024 | 18 pages | 4.071–5.978 ms | 0.001968701 |
| Complete `blk.0.attn_q.weight` | 2,048 x 1,024 | 1,152 pages | 24.715 ms | 0.001787636 |
| Complete `blk.0.ffn_down.weight` | 1,024 x 3,072 | 1,728 pages | 28.325 ms | 0.001818912 |
| Complete `blk.0.attn_output.weight` | 1,024 x 2,048 | 1,152 pages | 20.928 ms | 0.001762324 |

The complete query matrix made exactly **8,192 / 73,728 / 8,192 / 8,192**
device reads/flits/accepted/sent per vector; FFN-down made
**12,288 / 110,592 / 12,288 / 12,288**. Maximum observed normalized
BF16-reference error for these trials was `4.44479228e-5`. The stricter
1e-5 diagnostic still fails (query: 541/550 rows; FFN-down: 83/83;
attention-output: 168/153); this is
reported rather than hidden. First-vector times include roughly 31 ms of
one-time code/buffer snapshot and upload. All times also include vector/
poison staging, completion/stop checks and PIO output reads. They are
**not tokens/second, isolated kernel throughput or a CPU comparison**.

The native exclusive lifetime test and the old K1,024 fixed GEMV regression
still pass after the new compute session. The latter retains zero
mismatches and complete GDDR/NIU/L1/TLB/PCI restoration. Offline ASan/UBSan
tests check all assembled image bytes, five input widths, multiple output
tiles, independent fractional dot products, stale accumulators, corrupt
weights and missing first/interior/final NoC responses. Policy tests drop
all 50 post-upload control writes, sample lost code writes and cover
timeouts, wrong PC/flits, nonfinite results and permanent restore failure.
The old image/rollback suites and all 30 utility builds pass. The final
read-only PCIe/HDMA audit is clean, with both channels disabled/stopped,
commands restored to zero and no cold latch. Assistant team 2705 remains
ready/idle on its original TinyStories model with the conversation unchanged.

Native artifacts/logs and the untouched previous driver backup are in
`/boot/home/pioneer-runtime-matvec-20261015/`; a local ignored log copy is
under `generated.pioneer/tenstorrent/runtime-matvec-20261015/`. SHA-256:

- Driver: `392b900d7948685975ce89f53d38cb798e8551adf9ebca7ee7f7318fbf94ebb9`
- Final bounded-shape CLI: `a76879fbe74589d956a2231d0902a573dafac9e37dea79e096d43d2980962b11`
- Earlier two-vector accuracy CLI: `f5ea579187d40a332ae2456be51ae61dd0e0f7b3576f4059e56b049ec8475f80`

Next: connect this retained-tensor operation to the actual Qwen graph,
verify generated logits/tokens, then address single-core/PIO overhead and
benchmark the same model against CPU. Full 27B graph support and quantized
multi-bank residency remain unimplemented. Do not label the Assistant's
experimental TinyStories selection as Qwen acceleration.

### October 15: exclusive sessions and persistent Qwen-file chunks

The driver now has **per-open-FD, creator-team-owned sessions**. A session
retains its PCI reservation, validated mappings and a dedicated wired,
non-cached high-address host page until explicit END or final FD close.
Legacy experimental operations, other FDs and other teams cannot use the
session while it owns the card; inherited handles are denied to a different
team, while `dup()` in the owner preserves the session until final close.
The ordinary cold gate still takes precedence, including for the owner.
Final-close cleanup uses a separate locked path so it can quarantine a
possibly live host page even after that gate has latched.

The formerly one-shot PCI mapping policy is split into audited Begin/End
transactions. Existing callers still use the same one-shot wrapper and
retain the original 22-write rollback tests. END is idempotent and a failed
or partially mapped BEGIN must still be ended. Only memory/INTx-disable is
enabled by this policy; bus mastering remains confined to the bounded
owned-page transfer. Additional session preflight checks require a pristine
initiator, disabled IRQ configuration, all four command buffers idle and
both read and outgoing-write transaction-ID arrays empty. Source changes or
failed control rollback retain the cold-recovery requirements.

The first storage API exposes **fixed 4 KiB payloads and validated arena
offsets**, never host pointers or arbitrary NoC addresses. Initial bank-0
capacity is 4 KiB–2 GiB, page aligned, starting above the 64 MiB low
reservation and below the `0xff000000` NIU region. This is not a claim that
the card has only 2 GiB, nor enough residency for the 27B target: multi-bank
quantized storage remains required. Initialized-page bookkeeping refuses
reads from a page that has not been successfully uploaded; per-page FNV-1a
is integrity bookkeeping, not cryptographic model authentication.

On the first data operation the session saves the original 8 KiB L1 scratch,
NIU configuration and two TLBs once, then owns those resources across
requests. An upload stages the supplied bytes in the owned host page,
reads them into L1, proves host DMA completion and BME-off, writes L1 to
owned GDDR with a normal acknowledged NoC write, and reads GDDR back into
distinct L1 for complete byte comparison. All read responses return to the
initiating NIU. Every step requires ordered command readback, exact
response-or-ACK/flit/accepted/sent deltas **1/64/1/1**, idle IDs and stable
routes/mapping before the next step. A read operation fetches an initialized
GDDR page independently, validates its recorded hash, and returns the bytes.
No core is released and no firmware command, counter clear or retry exists.

This is real cross-request residency, not a synthetic fixture: the native
diagnostic used two distinct 4 KiB chunks at file offsets 32 MiB and
32 MiB + 4 KiB from the installed Qwen3-0.6B-Q8_0 GGUF. Its full SHA-256 was
reverified as
`9465e63a22add5354d9bb4b99e90117043c7124007664907259bd16d043bb031`.
Chunk hashes were `a1fdfe1d` / `7a1d46bb`. Within a 12 KiB arena it uploaded
offsets 0 and 8,192 in separate ioctls, independently read both back,
overwrote offset 0 with the second chunk, and checked both pages again.
Reading the uninitialized gap and writing at the arena end were refused
before a device command. The uploaded-page count remained two.

The final guarded driver passed this complete sequence twice: once with
explicit END and once leaving initialized storage to final-FD-close cleanup.
All three uploads per run completed 3/3 requests; all four independent reads
completed 1/1, with zero mismatches and no cold recovery. Host reads took
two polls, GDDR writes/readbacks one each. Explicit END restored original
L1/NIU/TLB/PCI state and released the host page; the automatic close allowed
legacy operations again afterward. The lifetime is the **open session**,
not reboot or a newly opened session. GDDR payload is intentionally retained
during ownership, not restored to pre-upload bytes after each request.
Closing releases logical ownership; another session must initialize its own
pages before reading them.

Native lifecycle testing also passed foreign-FD/legacy rejection, a child
process's inherited-team denial, duplicate-FD retention, explicit END and
automatic close without data. Existing native 4 KiB transfers and
1,024-input resident GEMV regressions pass after session cleanup. The
post-test PCIe audit remains clean, with firmware DMA channels disabled and
stopped. Assistant team 2705 remains ready/idle with its original model and
conversation unchanged.

ASan/UBSan tests cover arena overflow/alignment/NIU bounds, every lost
setup/data/close write, each missing pipeline completion, corruption,
inter-step source/mapping changes, failed BME-off, pending PCIe status and
unexplained flits. Uncertain completion quarantines the wired system-team
page and permits only BME-off and TLB/PCI control rollback, never L1/NIU
payload rewrite or a reset/retry. The existing MMIO/ARC/reset/L1/BRISC/DRAM
and resident-compute suites still pass. The driver and all 29 utilities build.

Final artifacts/logs: `/boot/home/pioneer-resident-store-guarded-20261015/`,
including separate explicit/final-close logs and the prior working driver
as `tenstorrent_pci_probe.before`. Session support v1 advertises capability
bits 1/2/4/8 (exclusive mappings / host buffer / storage / inter-step guards);
the guarded upload diagnostic requires bits 4 and 8 before proceeding.
SHA-256:

- Driver: `bde82a9caec341ae7a437622d8c4086a737387adc12a7d029ee8b1885ae0589e`
- Session/store CLI: `a97f1a2e4f5b1a535a0807bdb2dec40e1570986cee06ceeecb42d42a5c4471f0`

Earlier lifecycle-only and initial storage artifacts remain preserved under
`/boot/home/pioneer-resident-session-20261015/` and
`/boot/home/pioneer-resident-store-20261015/` respectively.

**Remaining:** raw retained chunks are not yet a model-aware tensor layout,
resident Qwen matrix execution, the full Qwen graph, or an Assistant backend.
The next step is compute over persistent weight tensors with input vectors
kept separate, followed by the actual model backend and same-model CPU/GPU
correctness and throughput tests. Current storage readback still uses PIO
for complete verification; do not treat these correctness-test times as
transfer bandwidth or Qwen tokens/second. The full 27B hybrid graph and
quantized multi-bank residency remain in scope and unimplemented.

### October 15: owned card-to-host write and distinct-L1 readback

Both fixed variants, 64 bytes and 4 KiB, now pass twice on the real P100A.
The stopped sole tile's L1 contains a third, distinct known pattern; the
owned non-cached host page initially contains a different pattern, and a
separate L1 readback region is poisoned. Each operation issues exactly one
non-inline, unicast, acknowledged NoC write into the owned host page, checks
CPU-visible target bytes and the full page, then issues exactly one NoC read
back into the distinct poisoned L1 region. No user pointer, arbitrary
physical address/length, DMA-engine message, core start or retry is accepted.

The write's `NOC_CTRL=0x212` sets `WR`, `RESP_MARKED` and
`MEM_RD_DROP_ACK`: the last suppresses only the unused NoC-overlay
notification, **not** the NoC write acknowledgement. The source coordinate
is the initiating tile itself; the destination uses the audited translated
PCIe `(19,24)` route and SII window 0. The pinned official NoC
`MemoryMap.md`/`Counters.md` define the write ACK, non-posted data-flit,
accepted and non-posted request counters at `0x204/0x220/0x210/0x228`.
All are read only after the ordered command readback. Complete proof also
requires every command buffer and read/outgoing-write transaction ID idle,
stable source routes, unchanged host physical mapping and clean PCIe status.
A NoC ACK alone is not sufficient evidence of host-visible PCIe posted data.

Each 64-byte run had write/read deltas **1/1/1/1**, with one ACK poll,
one host-visibility poll and one readback poll. Each 4 KiB run had write
and read deltas **1/64/1/1**, with one ACK poll, one host-visibility poll and
two readback polls. Every payload/source word matched, full-page host checks
passed, bus mastering was disabled/read back, original L1/NIU/TLB/PCI state
was restored and PCI commands ended `0000/0000`. All owned host pages were
released; none were quarantined and no cold recovery was required. For the
64-byte variant the rest of the host page is an independent guard; the 4 KiB
variant verifies the entire page as payload (no claim of guards outside that
allocation). The source/readback footprint is 128 bytes or 8 KiB of L1,
respectively; private heap backups keep the larger image off the kernel stack.

If **any** host write was attempted, freeing its page requires the entire
host-visible plus distinct-L1 readback proof. Missing/delayed/unexpected data,
missing ACK, a dropped write or readback doorbell, changed route/mapping,
unexplained counters or unverified cleanup retains the wired system-team
page and latches cold recovery. No counter clear, payload rewrite, reset,
request retry or module reload may bypass that gate. BME-off and TLB/PCI
control rollback remain permitted after uncertainty.

Both variants pass ASan/UBSan tests covering every lost setup/rollback write,
delayed host visibility, ACK-before-host-data, permanently missing/wrong host
data, bad guards, absent/wrong readback, source/mapping changes, cache/heap
allocation refusal, maximum 48-bit host address, counter wrap and permanent
cleanup failure. The existing read, MMIO, ARC, reset, L1, BRISC, DRAM and
resident-compute policies pass unchanged. All 28 RISC-V utilities and the
driver build. Native legacy 64-byte write, 4 KiB read and 1,024-input resident
GEMV regressions also pass; GEMV has zero mismatches and complete rollback.
The post-test read-only PCIe audit shows Device Status `0000/0000`, firmware
HDMA channels disabled/stopped, commands `0000/0000` and no cold latch.
Assistant team 2705 remains ready/idle on its existing TinyStories model,
without reopening or clearing its conversation.

Final artifacts/logs: `/boot/home/pioneer-host-write-page-20261015/`, with
the prior verified driver saved as `tenstorrent_pci_probe.before`. SHA-256:

- Driver: `c258042fcd7acb5b4b634a41dbb1c4fb2b0ebd49806d02a048dbdc7c1c6cadad`
- Host-write CLI: `e46f8758e357b25109354b0f6bc1d2ee1327bdeac8aea47c25177cbc6238867b`

Initial 64-byte artifacts/logs remain separately preserved in
`/boot/home/pioneer-host-write-20261015/` (driver
`8fc4f93c219e6f1eb750b751f0c048176bde775d81f262e7df7bfe44abadf594`, CLI
`06b60464c74c3a040a32f396add31028759f7f9558315b82ada0c9f2ce128288`).

These tests establish coherent bounded two-way host transfers, **not** bulk
bandwidth, persistent weights, full Qwen generation or a measured speedup.
The next implementation seam is an exclusively owned runtime session:
retain verified device weight storage across multiple input vectors, move
data through driver-owned host buffers, reject foreign experimental requests
while a session owns the card, and prove idle completion before closing it.
Use L1 staging plus normal acknowledged GDDR writes rather than sending read
responses to a different NIU (which corrupts outstanding-counter accounting).
The actual Qwen graph/backend and same-model CPU/GPU correctness and speed
comparisons remain required, including quantized residency for the 27B target.

### October 15: full owned 4 KiB host-to-card read passes twice

The same ownership/completion policy now has a second, separately selected
fixed variant: read the complete owned 4 KiB non-cached host page into the
sole stopped tile's final L1 page (`0x17f000`). Arbitrary addresses, lengths
and core starts remain unavailable. The original L1 bytes are held in a
private heap-owned CPU backup, not a 4 KiB kernel-stack array, and are never
exposed to userspace. The 64-byte ioctl/result layout remains unchanged.

The native 4 KiB variant passed twice: every one of 1,024 payload words
matched, the full host page was unchanged, each run completed in two polls
with response/flit/accepted/sent deltas **1/64/1/1**, all BME-off/L1/NIU/TLB/PCI
rollback checks passed, both PCI commands ended `0000/0000`, and each owned
host page was released without quarantine or cold recovery. The old
64-byte utility/operation also passed against this driver before the larger
test, preserving its exact **1/1/1/1** counter proof. Assistant team 2705
remained ready/idle with the same model. Both variants pass ASan/UBSan fault
injection, including every lost setup/rollback write, heap-backup refusal,
bad payload/flits, maximum 48-bit source addresses, counter wrap and the
unchanged no-retry/quarantine gates. The driver and all 27 tools build.

Native artifacts/logs: `/boot/home/pioneer-host-read-page-20261015/`, with
the previous verified driver saved as `tenstorrent_pci_probe.before`.
SHA-256:

- Driver: `a90e885122d195842e64058b635544b4a2b4127b9de8aa97e69160db565e1a27`
- Host-read CLI: `f475ccd3927995b0217124ee960e40f0c66c177c97644896facef35c6a0f558c`

This is still a reversible correctness test with substantial CPU setup,
PIO verification and rollback, **not a bandwidth or Qwen speed benchmark**.
It proves larger coherent owned-page reads, not persistent model storage.
The next milestones are a similarly bounded card-to-owned-host transfer,
bulk upload into owned device storage, persistent model weights, the Qwen
graph/backend and same-model correctness/performance comparisons.

Design constraint from the pinned official NoC `MemoryMap.md` and
`Counters.md`: sending a read response directly to a different NIU would
increment the requesting tile's outstanding counter but decrement the
destination tile's counter. Do not use that as a shortcut from host memory
to GDDR, or clear counters to conceal it. An intermediate L1 read followed
by a normal acknowledged write retains correct ownership/completion
accounting. Card-to-host writes will additionally need explicit host-visible
payload/guard proof; a NoC write acknowledgement alone must not justify
freeing a host page while a PCIe posted write could still be pending.

### October 15: first owned host-to-card read passes twice

A new, separate read-only preflight first verified PCIe Device Status on
both endpoint and root (`0000/0000`, including no Transactions Pending), and
the read/write HDMA channel 0 registers used by pinned 18.5.0 firmware. Three
samples showed both enables zero and both statuses `3` (stopped). These
registers are accessed through documented device-local NoC DBI window 62 at
`0xf800000000380000`, using the full 43-bit BAR0 TLB address field; BAR2's
1 MiB aperture is insufficient for these offsets. No PCIe-tile/HDMA register
is written. Source: pinned official `tt-system-firmware` 18.5.0
`lib/tenstorrent/bh_arc/pcie_dma.c` and `tt-isa-documentation`
`8a34e25e04d1ad7dd129ac9231e126b85923fac3`, PCI Express Tile address spaces.
This checks the firmware-used channel pair, not undocumented DMA channels.

The first real **host-to-card read** then passed twice on Haiku. Each run
allocated a fresh owned, wired non-cached 4 KiB page above 4 GiB, checked its
mapping/patterns and read exactly 64 bytes from page offset 128 into the
poisoned final 64 L1 bytes of stopped physical tile `(1,2)`. No user pointer,
arbitrary length, host-write operation or firmware DMA command is accepted.
NoC0 initiator 0 uses translated PCIe coordinates `(19,24)` (physical `(2,0)`),
the already-audited SII window 0 and 48-bit root inbound identity mapping.
All 120 enabled compute tiles remain held reset.

The dedicated policy briefly enables endpoint/root bus mastering (`0406`),
after rechecking the firmware engines, SII/inbound mapping, exact idle
initiator, routes and full host-page pattern. Both runs completed in one
poll with exact response/flit/accepted/sent deltas **1/1/1/1**, zero
destination mismatches and zero full-page host-guard mismatches. Completion
requires ordered command readback, all four command buffers idle, all read
and outgoing-write IDs idle, stable physical mapping and unchanged routes.
Bus mastering was disabled/read back before freeing the host page; BME-off,
L1, NIU, TLB and PCI rollback all passed. PCI commands ended `0000/0000`;
pages were released, none quarantined, and no cold recovery was required.
A read-only preflight between the runs again found no pending/error status
and both firmware channels disabled/stopped. Assistant team 2705 stayed
ready and idle with its existing model/conversation untouched. A subsequent
native 1,024-input resident GEMV regression also passed: zero mismatches,
64/1,088/64/64 counters, complete rollback and no cold-recovery requirement.

Timeout, a dropped doorbell, unexplained counter delta, changed route/map or
unverified BME-off/rollback latches cold recovery and quarantines the wired
system-team page. A delayed read must never see reused private memory. No
reset, counter clear, payload rewrite, retry or module reload is a recovery
mechanism. ASan/UBSan tests exercise every lost setup/rollback write, the
maximum 48-bit source address, counter wrap, accepted-but-incomplete reads,
bad data/flits, host-guard corruption, PCIe pending status, changed mappings,
permanent cleanup failures and quarantine ownership. The unchanged MMIO,
ARC, reset, L1, BRISC, DRAM and compute-route suites also pass. The RISC-V
driver and all 27 utilities build successfully.

Native artifacts/logs are in `/boot/home/pioneer-host-read-20261015/` with
the prior read-only driver saved as `tenstorrent_pci_probe.before`. SHA-256:

- Driver: `828dc285510a06860dfb4a32ddbb40da32281d8b27a20f092bad96875d6247c0`
- Host-read CLI: `93bbf2ec60df71661925bd09fc51dae5c884245eb8d42c23d40035438c2625df`

The preceding read-only audit's artifacts are separately preserved at
`/boot/home/pioneer-host-dma-audit-20261015/` (driver
`c41f1b5c4e79709cef13a3580fcf72c214477903d23d1519b442d40c040a7fbf`, CLI
`df8a0a8f62db8d68037751cbcb68eb63a469a1888b29efd955b8aec661ef0db7`).
These results establish a safe, coherent high-address read path for the
owned test page. They are **not** a bulk bandwidth measurement, a persistent
model allocator, full Qwen inference or a faster-than-CPU result. Next:
larger bounded transfers, persistent weight storage and the actual Qwen
graph/backend, followed by correctness and same-model CPU/GPU benchmarks.

### October 15: owned high-address host buffer prerequisite

The native CPU-only host-page diagnostic passed on the running Haiku system.
It allocated a dedicated contiguous, wired 4 KiB system-team area above 4 GiB
(33 physical address bits), validated its full mapping, configured the same
ordinary non-cacheable RAM type used by the port's NVMe queues **before any
payload write**, checked two full-page patterns, revalidated the mapping and
released the area exactly once. Allocation/map/cache/release statuses were
all zero, both patterns matched, and the Assistant remained ready and idle.
This test did not reserve PCI, map device registers, enable bus mastering,
issue a device transaction or establish DMA coherence.

`PioneerHostPage.h` separates owned-page preparation from the CPU-only
prepare/check/release wrapper so a future bounded device-read test can retain
the allocation until proven completion. The native owner can quarantine a
wired system-team area after uncertainty; destruction must not free a page
which a delayed device read could still access. No DMA caller exists at this
checkpoint. Host ASan/UBSan tests cover every lost pattern write, invalid and
overflowing addresses, changed mappings, allocation/cache failures and exact
cleanup ownership. The driver and all 25 cross-built utilities compile.

Native artifacts and the scoped log are in
`/boot/home/pioneer-host-page-20261015/`; the previous driver is preserved as
`tenstorrent_pci_probe.before`. SHA-256:

- Driver: `5317631f2792bb5549172f74065dbda714e5b9b2f9fa7efcf59c7afb6133f907`
- Host-page CLI: `bf92b31bd4e2267bd685f629ad4b34bf95a4c1c3b8f8467bab0282930eb2c2be`

The next transfer gate is a strictly bounded read **from** this owned host
memory, including PCIe pending-transaction/DMA-engine checks, destination
poisoning, complete counter proof and quarantine on uncertain completion.
Neither this prerequisite nor the resident GEMV fixtures are full Qwen
inference or a speedup over the measured Qwen CPU baseline.

### October 14: resume after SD-free baseline

The card-free baseline is now established: Linux → Haiku → Linux → Haiku
without either SD, using the RAM handoff and the tested RTL8125 MSI-only
override. Haiku SSH and Samsung root work; the initial loader still exposes
one CPU and about 3 GiB RAM. The user requested renewed P100A bring-up and
confirmed the card remains physically removed. Normal Haiku shutdown closed
services/SSH, followed by the known seven-second relay power-off hold at
about 18:13 UTC. Physical fans/LEDs-off and AC disconnection must be confirmed
before installing the card. Leave both SD cards out.

Synced the previously absent read-only diagnostic C/header and their sole
Jam target into the reviewed build mirror. An integrated Jam build of
`<driver>tenstorrent_pci_diag` passed; the private artifact/log are under
`generated.pioneer/tenstorrent/haiku-integrated-20261014/`. Nothing was
installed on Samsung, no accelerator driver was loaded, and no card firmware,
PCI configuration or DMA settings were changed by this preparation.

Next: reinstall in the previously used lower slot with the correct fully
seated native power cable while AC is disconnected; restore PSU power but
leave the host off for controlled startup. First cold-boot the unchanged
NVMe Linux, record identity/link/BAR assignments read-only, then assess a
Haiku enumeration test through the new boot path. Bypassing UEFI removes
that particular boot dependency, not the documented BAR4 aperture limit.
The firmware access/backup/compatibility gate below still applies; do not
flash the old experimental bundle or hide an enabled/unassigned BAR merely
to make a driver bind.

### October 14: installed P100A, native enumeration and reversible BAR4 test

The user reinstalled the card. A relay-controlled cold boot reached unchanged
NVMe Linux 6.1.31 with both SD cards out. Linux again found `0002:81:00.0`,
`1e52:b140`, subsystem `1e52:0043`, at 2.5 GT/s x8. All three BAR allocations
still failed (512 MiB, 1 MiB, 32 GiB); command was `0000`, no driver bound.
The old known-good RAM handoff still booted Haiku and SSH with the card in,
but `listdev` omitted it. Serial proved why: the ECAM driver unconditionally
skipped hardware root `0x80`, rather than checking this installed endpoint.

Added an opt-in ECAM gate, leaving the default skip unchanged. It requires
the exact full-width FDT bus range `80..bf`, a four-byte big-endian
`haiku,pioneer-root-probe = <0x1e52b140>` property on `/pcie@7062000000`,
controller address `0x7062000000`, and config aperture `0x4800000000/0x1000`.
Existing unavailable-link fill-pattern guards remain. The tag is a private
enumeration request, not authentication of a card and not permission to
enable endpoint decoding, bus mastering, or firmware writes. Host ASan/UBSan
tests and integrated `<pci>ecam` cross-build passed.

Only the existing `add-ons/kernel/busses/pci/ecam` entry was replaced in a
new native-only copy of Samsung's HPKG. Sorted package path sets matched,
the new module extracted to the expected hash, and the Samsung kernel stayed
`38d4a4245db6a8ec4e2baa44c7705cdcdd5d4e66a8d43da97f3157699449a65b`.
The original package remained unchanged. The fresh 300 MiB RAM BFS passed
check-only consistency and package round-trip verification. Initial default
maximum compression was slow and reordered the TOC; the successful final
build uses fast compression and compares path sets independently of order.

Private native experiment under `/mnt/ssd/pioneer-haiku-kexec-20261013/`:

- `p100a-root-probe-2/samsung-ram.bfs`:
  `f6292d07bc16f41bedca3d1b63edb2da2fa9961065da3b86e66e74b95196b611`
- Its private HPKG:
  `7a830442362ba511bdfc08f6d99e26df5813475861d3583a90cb62cbef1f3cea`
- Candidate ECAM:
  `23e65e2a9a3e886fc7d1429901db6534f2cc7a9077f121d3e334f4cf839cf11d`
- `boot-p100a-root-probe-2.sh`, session
  `p100a-root-probe-2-20261014`, uses those exact pins, the existing loader,
  fresh Samsung sector manifest, UART keepalive, and normal `systemctl kexec`.
  The tracked baseline boot helper's original artifact pins were not changed.

At about 18:39 UTC, serial showed the opt-in marker, root link state
`0x5000ff07`, and endpoint vendor `0x1e52`. Haiku mounted Samsung and SSH
returned. `listdev` identifies Blackhole. The optional read-only diagnostic
was installed in the user non-packaged driver tree and ran successfully:
Haiku virtual PCI `03:00.0`, subsystem `1e52:0043`, command `0000`, status
`0010`, raw BAR words `0000000c 0 0000000c 0 0000000c 0`.
Haiku's virtual bus number is not Linux hardware bus `81`.

Added the separate optional `<driver>tenstorrent_pci_probe` and
`tools/pioneer/tenstorrent_haiku_bar4_probe.cpp`. It accepts only an explicit
root, read/write-open ioctl. It reserves the sole matching P100A during the
transaction, rejects any enabled command, assigned BAR, or unexpected cached
BAR size, temporarily writes BAR4's high address, then restores and verifies
all six BAR words. No arbitrary register/address request, MMIO, bridge
change, command-enable operation, DMA, reset, or firmware access exists.
Simulated refusal, repeated use, narrower address masks, concurrent-command
interruption, and restoration-failure tests passed under ASan/UBSan.
The module cross-built and the CLI compiled natively with warnings as errors.

Live discovery needed the canonical user `drivers/bin/tenstorrent_pci_probe`
plus `drivers/dev/misc/tenstorrent_pci_probe -> ../../bin/tenstorrent_pci_probe`;
adding a file only to the already-scanned category was not noticed. The first
attempt therefore made no PCI writes. The canonical-bin installation was
discovered, and the controlled test then passed: all BARs restored exactly,
command `0000 -> 0000`, sizes `0x20000000 / 0x100000 / 0x800000000`, temporary
BAR4 readback `fffffff8:0000000c`. A subsequent independent read-only ioctl
confirmed the original BAR values and command `0000`.

This establishes hardware address-bit retention, **not** a resource allocation
or safe CPU mapping. A possible next experiment is to keep unused BAR4 in a
verified unforwarded high PCI interval while assigning only BAR0/BAR2 inside
the recorded host window. Before any memory enable, validate the complete
BAR intervals, bridge windows, host translations, interrupt state, exclusive
ownership, and rollback. Do not merely hide BAR4 in software. There has been
no card MMIO/DMA or firmware write. Firmware flashing still requires a
verified backup/recovery plan and separate approval.

### October 14: first working Haiku card-register access

The next optional probe operation audits the actual device-manager ancestry,
exact FDT tag/register layout, two-device root topology, all five outbound
windows (one I/O plus four memory), and domain-aware translation of both ends
of the proposed BAR0/BAR2 intervals. `GetPioneerPciProfile()` and its userspace
tool are read-only. The initial recursive traversal hit the existing kernel
`get_next_child_node()` missing-break behavior; the audit uses the working
`find_child_node()` API instead, without changing the installed kernel.
The exact PCI interface name is `bus_managers/pci/driver_v1`. A first layout
check omitted the I/O range, refused safely, and was corrected to include all
five actual ranges. The final audit passed at stage 6:

- Hardware controller/config: `0x7062000000`, `0x4800000000/0x1000`.
- Haiku virtual card/root buses: 3/2, root buses `003f0100`.
- Initial root command `0000`, memory window `0000fff0`, prefetch window
  `0001fff1`, both upper prefetch words zero (disabled windows).
- BAR0 PCI and CPU interval: `0x4900000000..0x491fffffff` (512 MiB).
- BAR2 PCI and CPU interval: `0x4920000000..0x49200fffff` (1 MiB).

The explicit `TT_PCI_PROBE_PIONEER_NOC_ID` operation requires root and a
read/write-open descriptor. It reserves the card, repeats the live audit,
rejects enabled commands, assigned BARs, unexpected sizes, root-port BARs,
enabled option ROMs, active MSI/MSI-X, malformed capability lists, and a
non-D0 PM state. It assigns only BAR0/BAR2 inside the recorded window and
places the complete unused 32 GiB BAR4 at PCI
`0xfffffff800000000..0xffffffffffffffff`, outside every forwarded memory
range. This is a hardware BAR placement, **not** hiding a live BAR in kernel
metadata and **not** disabling the BAR in hardware. It is not a general
resource allocator or a permanent driver configuration.

The temporary bridge prefetch window is `20010001`, upper base/limit `49/49`.
Only command `0402` (memory + INTx disable) may be enabled on card and bridge;
bus mastering, I/O, firmware writes and resets are never enabled. A single
BAR0 page at CPU `0x491fd04000` is mapped kernel-read-only and explicitly
uncached. Exactly three reads use register offset `0x44`, followed by unmap,
decoder shutdown, complete BAR/window restoration, and configuration readback.
Disassembly caught `__sync_synchronize()` producing only `fence rw,rw`;
the final kernel implementation explicitly uses RISC-V `fence iorw,iorw`.
The offset/coordinates are also documented by the pinned Apache-2.0 UMD
`blackhole_implementation.hpp` / `blackhole_arc_apb.cpp`; no GPL driver
implementation was copied.

Host ASan/UBSan tests cover every dropped configuration write, map/unmap
failure, failed decoder shutdown, unsafe geometry, stale/active BARs,
interrupt/ROM/PM guards, malformed/cyclic capabilities, unstable register
reads, and restoration checks. They assert no DMA-enable operation and no
BAR/bridge-window rewrite while its decoder is active. These are simulated
failure tests, not injected hardware failures.

Before the first MMIO test, SSH stopped responding during banner exchange
and serial showed a userspace "Crashed program" dialog. No new MMIO module
had been installed or run at that point; the cause is not established.
Graceful SSH shutdown was unavailable. The last optional-driver installation
had been synced; a relay power-off hold and cold boot recovered normal Linux
with card command `0000`. The same private RAM package booted Haiku again
without either SD. Native compilation was removed from the hardware-test
dependency: `tools/pioneer/tenstorrent_cross_build_tools.sh` builds all four
diagnostic CLIs against the configured build's SDK/startup objects/libroot,
using a fresh output directory and no device access. Its three C++ binaries
reproduced the earlier cross-build hashes and ran on Haiku.

At about 19:31 UTC, the first hardware transaction returned
`18611002 / 18611002 / 18611002`, with complete PCI rollback. The original
validator wrongly required the entire word to equal 2 or 11; upper register
fields are nonzero. The corrected validator checks the six-bit X/Y coordinate
pair while preserving the full raw value, matching PCIe tile `(2,0)` here.
A regression case covers this real value and rejects a wrong Y coordinate.
At about 19:36 UTC the repeat returned **state 1, stage 5, restored 1** and
the same three words. All six BAR words and all three bridge-prefetch words
matched their originals, card/root commands were `0000/0000`, and an
independent read-only driver snapshot confirmed the card's original disabled
state and status `0010`. SSH remained usable. This establishes bounded
memory-mapped card-register access, not ARC/DRAM health, DMA, or compute.

NoC-ID-tested module at this checkpoint in user `drivers/bin/tenstorrent_pci_probe`, with
the existing `dev/misc` symlink:
`1ec5a894442283e47b31338628a563cff5b4699b21941f298e10424b9c1e60b5`.
The previous binaries are retained under
`/boot/home/pioneer-p100a-20261014/` for recovery. Mac-built tools and module
are privately staged in its `mmio-cross-1/` directory. The baseline tracked
boot helper and installed OS package were not modified. The last boot used
native session `p100a-mmio-20261014-1` with the previously verified RAM bundle.

Next: bounded ARC status/telemetry over a 2 MiB NoC window, with restoration
of the exact TLB configuration. The observed PCIe tile is X=2; the pinned
UMD explicitly routes ARC access over NoC on that tile. Do **not** assume
the direct BAR0 ARC/APB route (available at X=11) works here. No ARC access,
firmware update, DMA test, general memory mapping API, or accelerator runtime
has been implemented/tested yet.

### October 14: ARC firmware responds over a restored NoC window

Added an explicit `TT_PCI_PROBE_PIONEER_ARC_SCRATCH` operation and matching
`tenstorrent_haiku_arc_probe --read-arc-status-and-restore` CLI. It reuses the
tested temporary PCI transaction and NoC-ID validation. Only PCIe tile `(2,0)`
may enter this new path. The ARC target is NoC0 `(8,0)`, reached through the
last 2 MiB BAR0 window, index 201. Its 12-byte configuration at BAR0
`0x1fc0096c` is saved, temporarily configured, verified, and restored exactly.
The independently encoded words are `00000400 / 04004000 / 00000a40`:
ARC local base `0x80000000`, strict unicast-read ordering, static read VC/buddy.
The ARC data page at BAR0 `0x19230000` is kernel-read-only and uncached;
only the TLB control page is writable. There is no ARC data-write method.

The sample reads scratch registers 2, 4, 11, 12 and 13, then repeats register
2, for exactly six ARC reads. These cover boot status, error status, message
queue pointer and telemetry data/tag pointers. Pointers are reported but not
dereferenced. There are no ARC messages, ASIC/core resets, firmware writes,
SPI operations or DMA. The outer operation unmaps the target before shutting
down PCI decoding and verifies complete PCI rollback. A TLB restore failure
gets one bounded retry while decoding is still on; any remaining restore
failure requires cold recovery. Destructors only remove virtual mappings and
never attempt MMIO after decoding has been shut down. Partial map failures
are also cleaned up by the outer transaction.

The fixed layout and ARC status/scratch facts were checked against pinned
Apache-2.0 UMD sources. Host ASan/UBSan tests independently decode the 96-bit
TLB layout and cover dropped setup/restoration writes, unknown saved bits,
nonzero saved configurations, boot/error/telemetry conditions, and bounded
reads. The existing NoC-ID/PCI policy suite also covers failed callback and
partial-map cleanup. Integrated kernel cross-build passed, and disassembly
shows explicit `fence iorw,iorw` around device accesses. The cross-build script
now builds five userspace tools; the original four binary hashes stayed the
same. Their installed operations retain their earlier ABI/behavior.

The new optional module was staged with its previous version retained. A
NoC-ID hardware regression test passed before the new operation was called.
At about 19:51 UTC, the ARC operation returned **ARC state 1, TLB restored 1,
PCI state 1, PCI restored 1**. TLB words were initially all zero, became the
three expected words, then read back as all zero after restoration:

- ARC boot: `00000005 -> 00000005`.
- ARC error status 0: `00000000`.
- Message queue pointer: `100161c4`.
- Telemetry data/tags pointers: `1001902c / 10018f28`.
- Card/root command after: `0000 / 0000`; NoC ID: `18611002`.

The independent read-only PCI snapshot confirmed original BAR words, command
`0000`, and status `0010`. No card firmware was changed. This proves bounded
ARC register access and boot-ready status, **not** a complete card/DRAM health
test, firmware compatibility assessment, interrupt/DMA path or compute stack.
A second ARC run returned the same values, again with initially zero TLB
words and exact restoration. The independent live profile then passed stage
6, including original disabled root windows and both domain translations;
the separate PCI snapshot again showed the original disabled endpoint.

Optional module at this ARC-only checkpoint (superseded below):
`d17d56a3035927d47eed55f0f7161aa0af0dbde117c9f7259afd124d62e149a7`.
ARC CLI:
`4ebae8542bd87a54afc58a897235f3f498d0e7b1beefddcbd73a9fcc69a166bd`.
They are in the existing private `mmio-cross-1/` staging directory. The prior
module is retained as `tenstorrent_pci_probe.before-arc-1` there.

Next: validate the advertised telemetry-table layout and ARC CSM bounds
against the vendor sources before reading whitelisted firmware-version and
health values. Do not assume that a nonzero pointer authorizes an arbitrary
NoC read or a flash operation.

### October 14: native Haiku firmware telemetry and live heartbeat

The next explicit operation, `TT_PCI_PROBE_PIONEER_TELEMETRY`, first performs
the tested ARC status transaction, then reuses TLB 201 for a read-only CSM
window. The CSM NoC base is **`0x10000000` directly**, not that address plus
the ARC/APB XBAR offset. Its words are `00000080 / 04004000 / 00000a40`.
Only the first 128 KiB is mapped, despite the hardware CSM having a larger
aperture. Both the ARC and telemetry TLB configurations and the entire PCI
transaction are independently restored and verified.

The parser requires aligned, bounded tag/data pointers, at most 128 entries,
a supported table major version, non-overlapping table/data spans, valid
offsets and no duplicate requested tags. It repeats the header after sampling
and clears the output mask if the header changed. Values are not an atomic
snapshot: live telemetry can change between individual reads. It reads only
12 whitelisted public health/version values, never board/ASIC serial values
or arbitrary requested addresses. There are no data writes, firmware
messages, SPI operations, reset commands, DMA or bus-master enables.

At 20:10 UTC the real card returned telemetry state 1, ARC state 1, PCI state
1 and all three restored flags 1. Table version was `00000100`, with 63
entries and all 12 requested tags present. A repeat returned the same
structural values and restored flags:

| Public telemetry | First sample | Repeat / interpretation |
|---|---|---|
| Firmware bundle | `12050001` | 18.5.0, raw suffix `01` |
| CM firmware | `000e0000` | unchanged raw value |
| ASIC temperature | `002e8f7a` | `002eb04a`, about 46.56 to 46.69 C |
| Maximum GDDR temperature | `0000002e` | 46, unchanged |
| Fan | `0000075a` | 1,882 RPM |
| Enabled GDDR | `000000f7` | seven enabled channels, consistent with nominal 28 GiB; capacity not tested |
| GDDR status | `00005515` | training complete/no error on all seven enabled channels |
| GDDR uncorrected errors | `00000000` | none reported in these samples |
| Enabled Tensix columns | `00000fff` | raw mask; no compute test |
| Firmware heartbeat | `0000a44a` | `0000b6a6`, advancing |
| Board temperature / shutdown limit | both zero | sensor/limit availability unverified; do not interpret as a healthy 0 C reading or limit |

The GDDR interpretation follows the pinned UMD's per-channel two-bit training
fields (completion bit then error bit), with disabled channel 3 ignored.
This firmware predates 19.7's upper-half BIST reporting: the zero upper half
does **not** mean a failed or completed BIST. This did not run a DRAM test.

The pinned UMD explicitly sets Blackhole's minimum compatible bundle to
18.5.0 and has an 18.5-family feature map. Its `FirmwareBundleVersion`
comparison normalizes away the suffix, so `12050001` meets that numerical
minimum. This does not validate every current Metal/LLM release or prove the
entire software stack compatible. The official versioned
[Metal v0.62.2 installation matrix](https://docs.tenstorrent.com/tt-metal/v0.62.2/tt-metalium/installing.html)
also names Blackhole firmware 18.5.0. No firmware update is needed merely to
continue this bounded native transport investigation. Flashing still requires
a verified backup/recovery plan and separate approval; the old experimental
19.15 bundle remains unflashed.

The independent profile passed stage 6 after the repeat, with original
disabled root windows. The separate diagnostic confirmed card command
`0000`, status `0010` and all six original BAR words. All original TLB words
were zero before and after both samples. Haiku and SSH stayed available.

Host ASan/UBSan tests cover pointer/span/count/version validation, maximum
128-entry tables (144 bounded CSM reads), duplicate/out-of-range entries,
changing headers, missing required fields and dropped setup/restore writes.
All earlier policy/BAR/MMIO/ARC suites still pass. The integrated kernel
module and all six userspace tools cross-built successfully; the previous
five CLI hashes were unchanged.

Optional module at this telemetry checkpoint (superseded below):
`5ee9781e130f6e8ca701c4bb95adecef4ede61f2a2b6cbbecaaabec43a3e0456`.
Telemetry CLI:
`75701df2eb37c67c20516fa3dc6fce668a1848385d732980ac247f63944013c6`.
Both are installed/staged in the existing private `mmio-cross-1/` tree.
The prior module is retained as `tenstorrent_pci_probe.before-telemetry-1`.

Next: audit a bounded device-memory/compute-core smoke-test path, including
physical/harvested tile coordinates, reset state, read/write NoC ordering,
scratch ownership and rollback, before any data or core-control writes.
Persistent mappings, interrupts, non-coherent host DMA, native UMD/runtime
and LLM execution remain unimplemented and unvalidated. Both SD cards stay
out; Samsung's installed OS package/default Linux boot remain unchanged.

### October 14: stopped Tensix core and reversible L1 memory test

Added two explicit private operations, `TT_PCI_PROBE_PIONEER_TENSIX_RESET`
and `TT_PCI_PROBE_PIONEER_L1_SCRATCH`, plus matching userspace tools. They
retain all earlier profile, PCI, ARC and telemetry checks, require the measured
firmware tag `12050001`, and require enabled-column telemetry bit 0. The
vendor's physical harvesting-column order starts at x=1, and its Tensix grid
starts at y=2. The sole permitted tile is therefore `(1,2)`. No arbitrary
coordinate or address is accepted from userspace.

The initial conservative build refused NIU configuration `00006005` because
translation bit 14 was set; that run made no core-register reads. Source
review of UMD's `fill_tensix_noc0_translated_mapping()` established that the
first enabled physical column maps to translated x=1. Since bit 0 requires
physical x=1 itself enabled, this one tile remains `(1,2)` in either mode.
The revised guard permits that specific case, not arbitrary translated
coordinates. One immediate run still used the previous loaded module before
Haiku's asynchronous driver watcher reloaded it; a later run reached the
new code. The canonical-file hash alone does not prove an already-loaded
driver revision has changed.

The reset-state operation maps only the read-only CSR page, temporarily using
TLB 201 words `000007fd / 40840800 / 00000a40`: NoC0 `(1,2)`, local base
`ffa00000`. It reads `ffb121b0` three times, never writes reset controls, and
requires all BRISC/TRISC0/TRISC1/TRISC2/NCRISC reset bits (`00047800`) held.
All three hardware reads returned exactly `00047800`; the TLB and full PCI
state were restored, independently confirmed by the diagnostic.

The L1 operation first passes that same gate. It separately saves/configures
TLBs 200 and 201, using TLB 200 for the last 64 bytes (`17ffc0..17ffff`) of
that stopped tile's 1.5 MiB L1. Its data window is strict **bidirectional,
dynamic VC**, words `00000000 / 40840800 / 00000040`. Do not reuse the static
read VC for mixed reads/writes: the pinned UMD explicitly warns that this can
crash the host. TLB 201 remains the strict read-only reset window.

The only data-write API is sixteen fixed, aligned words. Original values stay
kernel-local, never in the ioctl or logs. The operation checks reset state
before patterns and after testing, writes/reads two independently
address-varied patterns, restores all sixteen original words, and reads them
back. A failed data restoration gets at most one retry before either TLB is
changed. Persistent restoration failure requires cold recovery; no destructor
ever touches device MMIO after PCI decoding is off. Normal traffic is 48 L1
writes / 64 L1 reads, with at most 64 writes / 80 reads on a restore retry.
There is no core start, instruction loading, firmware message, flash, reset
write, interrupt enable, DMA or bus mastering.

At about 20:27 UTC the real card passed both patterns with zero mismatches,
data/TLB/PCI restored `1/1/1`, and reset before/after `00047800/00047800`.
All six TLB words were zero both before and after. Card/root command ended
`0000/0000`; the independent snapshot confirmed all original BARs and status
`0010`. A second full L1 run passed identically. Subsequent telemetry still
reported about 46.69 C, 1,882 RPM, GDDR errors zero and an advancing heartbeat
`0000de7c`. The live root profile again passed stage 6. No panic was captured,
and SSH remained working.

Both new host suites pass ASan/UBSan, including independent 96-bit TLB decode,
wrong firmware/harvest state, translated/non-translated modes, changed or
running core state, setup/restore failures, every dropped pattern/restore
data write, and persistent restore failure. Fault-injection initial TLB words
must differ from trial words: dropping a write of the already-present zero
is not a configuration failure. All seven host suites and the integrated
kernel build pass. All eight CLIs cross-build; the earlier six hashes remain
unchanged.

Current optional kernel module:
`f0ab0ebac81ffd0f2b8ac81ad3dacfb90ecbd1f2e8728b3765543178776b101b`.
Core-state CLI:
`5b3cdeb3b3f5db9c93d3b8f29ba61fbeded11c8159549e53e075c92323ae1813`.
L1 CLI:
`0b4e526fa699c52deb800efe4902b9e5fcabc7c3dffb96a30723cc2ea0a02372`.
Staging remains `mmio-cross-1/`; the previous module is retained as
`tenstorrent_pci_probe.before-l1-1`. The intermediate core-only modules are
also retained. Only reviewed selected sources were synced to the build mirror.

This proves native host PIO access to a stopped compute tile's local memory,
not execution on its RISC/Tensix processors, a GDDR capacity test, performant
transfers or an LLM backend. Next inspect the vendor's minimal core startup,
instruction/cache requirements and compatible runtime before launching code.
The latest vendor Metal installation matrix names newer firmware than this
card has; the older versioned 18.5-compatible matrix and UMD's minimum do
not certify current Metal/LLM releases. No automatic firmware upgrade.

### Minimal execution audit: not yet a hardware execution test

Read only the relevant official Metal v0.62.2 sources in an ignored,
no-checkout, filtered clone. Annotated tag object
`278c18eddaee71ff6127f083a4a5f2934b688866` resolves to commit
`d279aa36bebe71367a521449310f6f53100e92be` (Apache-2.0). No upstream script,
build or initialization command ran. The clone is private generated source,
not an added dependency or a validated native Haiku runtime.

Blackhole's `bh_hal_tensix.cpp` sets BRISC's reset PC to zero and places a
generated JAL at L1 address zero. `hal.cpp`'s startup helper explicitly bounds
the firmware base below `0x7ffff`. `dev_mem_map.h` reserves low L1 locations,
including ARC power-throttling scratch at bytes 16..31. BRISC's normal
`device_setup()` also performs architecture-specific instruction-cache
invalidation and other setup. These are not covered by the tail-of-L1 data
test. A valid CPU-side configuration-register address alone does not establish
that a host NoC write to it is supported or safe.

Do not simply clear BRISC's reset bit after a successful data round-trip.
A separate execution transaction needs audited boot-word/code/mailbox
ownership, executable-image validation, instruction/cache behavior, correct
read/write NoC attributes, bounded completion polling, and verified core stop
**before** restoring any instructions or data. Failure to stop must prohibit
rewriting live code and require cold recovery. No such transaction is
implemented or deployed yet. No reset controls or instruction bytes were
written during this audit. The last hardware operation remains the successful
repeat L1/telemetry/profile/independent PCI verification above.

### Continued execution audit and offline image, October 14 local time

Pinned official ISA documentation at
`8a34e25e04d1ad7dd129ac9231e126b85923fac3` in a private no-checkout clone.
Its [Blackhole soft-reset contract](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/TensixTile/SoftReset.md)
specifies cache invalidation when leaving reset, BRISC entry at zero, and no
new instruction starts while reset is held. It does not promise immediate
abortion of every in-flight operation. Its
[NoC memory map](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/TensixTile/BabyRISCV/README.md)
marks backend configuration inaccessible over NoC: do not attempt a host
write to the CPU-local cache-control register. Local RISC RAM is also
reinitialized on release unless configured otherwise. This is a documented
startup contract, not yet validation on this installed board/firmware.

Added an original, offline-only RV32I checksum/park image and linker bounds.
The sole reset word jumps to code at 0x70000 (under the prior startup bound).
It computes 1+...+32, writes 528 at 0x17ffc0, consumes a readback, fences,
publishes PION at 0x17ffc4, consumes that readback, fences and parks. It uses
no stack, local RAM, CSR, interrupt, NoC command, DMA or Tensix instruction.
The host test assembles/links the real RV32I image, independently decodes it,
allows only its fixed instruction subset, bounds every instruction fetch and
data access, checks both results and observes the terminal self-loop. The
boot section is exactly four bytes; code is at most 128 bytes. Host test
passes with ASan/UBSan; this models instruction/data bounds, not hardware
cache timing or reset quiescence.

No driver/ioctl references this image, no instructions have been installed,
and no reset bit has been written. Integration still requires initial-zero
completion checks, exact code/data preservation, fixed firmware/tile gates,
completion/stop verification, and a cold-recovery-only path on uncertain
stop. Never rewrite live instructions or treat a timeout as restored state.
The original kernel driver's operation set and installed binary are unchanged.

### October 14: first native P100A scalar-core execution

Implemented the separate explicit BRISC operation and a read-only loaded-
module metadata query. All earlier operation ABIs and eight CLI hashes stay
unchanged. The new policy uses dynamic bidirectional VC for both L1 and reset
access, starts only bit-11 BRISC on the already validated tile `(1,2)`, and
requires drained results plus two park-PC samples before stop verification
and code restoration. Original L1 bytes remain kernel-local. Unknown core
state or execution/stop uncertainty leaves the image in place and latches
cold-recovery-required; later operations are refused under the same lock.
Reloading a module is not cold recovery. Local BRISC RAM/register state may
reset; the L1/PCI/TLB restore flags do not promise arbitrary RISC context
restoration. No other RISC, DMA, firmware message or flash operation is used.

New optional module SHA:
`58ffcadcdc3e9ae0804b75ddda17ad55b06da97613a9d8b5b51a1b5b64107174`.
New explicit CLI SHA:
`d8c88121d0b9c194ba4b0c8f7ebdd1629e41e093e4584c85cfbdc7a6a8889c70`.
They are in `brisc-exec-1/`, with the exact previous trace driver retained as
`tenstorrent_pci_probe.before-brisc-1`. A first metadata check correctly saw
the old loaded module; a later check confirmed devfs had reloaded the new
one. All prior topology/telemetry/held-core/L1 checks passed before execution.

At about 21:01 local October 14 (04:01 UTC October 15), the first execution
PASSED: checksum 528, completion 50494f4e, both PC samples 00070048, reset
00047800 -> 00047000 -> 00047800. Started/completed/stopped and all three
L1/TLB/PCI restored flags were 1; card/root commands ended 0000/0000. The
repeat passed identically, using five polls versus one in the first run.
Independent health/core-state/profile checks and the host 64-CPU test passed
afterwards; no recovery or SD change was necessary. This proves bounded
scalar RISC code runs on the real P100A under Haiku. It is not Tensix matrix
compute, a GDDR memory audit, a native runtime port or accelerator LLM inference.

### October 14: first verified read-only GDDR route

Added a separate fixed bank-0/port-0 NIU identification transaction, not a
payload memory operation. Before issuing a NoC request, it reads the PCIe
NIU's translation configuration and proves translated `(18,12)` maps to
physical `(0,0)`. It requires the measured firmware `12050001`, GDDR mask
`f7`, status `5515`, zero reported uncorrected errors and the existing held-
core control result. Only the fixed source-configuration/target-ID registers
are readable through this new ioctl; no arbitrary addresses or input fields.

The initial live check restored all PCI/TLB state but refused its ID because
firmware 18.5 retains the physical/power-on `NOC_ID_LOGICAL` label at this
port, zero for `(0,0)`. This register is a writable software label, not the
routing lookup table. Policy v2 accepts either that observed physical label
or the prescribed translated label; the independent physical
`NOC_NODE_ID`, typed `NOC_ENDPOINT_ID`, source-table translation and repeated
sample checks remain required. No identity field or actual routing-table
check was removed. Host ASan/UBSan tests cover dropped configuration/restore
writes, wrong/unstable IDs, all-zero/all-one reads and source-table changes.

At 21:28 local October 14 (04:28 UTC October 15), loaded policy v2 and the
fixed route PASSED. Three samples were identical:
`NODE=1c611000 ENDPOINT=00080000 NIU=0000e005 LOGICAL=00000000`.
The source NIU was `6005` with unchanged translation words. TLB/PCI restore
flags were 1/1, commands ended 0000/0000, and cold-recovery flag was zero.
Only bank-0 identification was tested, not GDDR capacity or matrix compute.

Installed optional driver SHA:
`d999e495ca4fb73cb577f1558e3f5b515dd19c5700a283802ec455b1701b4bec`.
CLI SHA: `4f7b46c23bcbe5cb513ba25e755fe59a850a08cd7975c68f4c7f4ca0a963cbb4`.
Artifacts and the previous driver are retained under
`/boot/home/pioneer-p100a-20261014/dram-route-2/`; the earlier BRISC driver
also remains backed up under `dram-route-1/`. Metadata-only
`--check-fixed-dram-support` identifies loaded policy v2. Explicit
`--read-fixed-dram-route-and-restore` performs the bounded transaction.

Register facts were audited against official ISA documentation commit
`8a34e25e04d1ad7dd129ac9231e126b85923fac3`,
`BlackholeA0/NoC/{Coordinates,MemoryMap}.md` and
`BlackholeA0/PCIExpressTile/HostToDeviceTLBs.md`, together with the pinned
UMD coordinate manager. Preserve those sources' own license notices; this
policy is an independent implementation of register facts, not copied
documentation code. No DMA, firmware messages/flash or model inference ran.

### October 14: seven-bank GDDR payload samples passed twice

Added two separate operations: read-only identification of the seven enabled
bank/port-0 routes, and an explicit fixed reversible payload-sample test.
Firmware 18.5's live source tables map physical banks 0/1/2 to translated
`(18,12)/(18,15)/(18,18)` and banks 4/5/6/7 to
`(17,12)/(17,15)/(17,18)/(17,21)`. This differs from the newer pinned UMD's
mirror-bank ordering. The policy derives routes only within the eight
documented DRAM port-0 slots, proves each against the source tables, and
then independently verifies physical NODE_ID and typed bank ENDPOINT_ID.
Firmware retains physical labels on the west ports and translated labels
on the east ports; only the matching physical or matching translated label
is accepted, not arbitrary zero on every bank. The initial seven-bank v1
attempt safely refused bank 1's physical label, before any payload access.

The fixed payload footprint is five 64-byte samples per enabled bank at
local addresses `04000000`, `40000000`, `80000000`, `c0000000`, `fe000000`:
2,240 bytes total. All are above the pinned HAL's low reservations (including
its optional profiler reservation, 6,400,064 bytes total) and below
`MEM_DRAM_SIZE=ff000000`; the top 16 MiB is never used as payload. The
driver reserves the otherwise undecoded card, requires exact telemetry,
proves all 120 enabled Tensix tiles' five RISCs are held, and preserves the
original sample bytes kernel-locally. Each pattern is written to the whole
set before reading back, so aliased banks/high addresses cannot silently
pass independent write/read pairs. Before restoring bytes, it re-proves all
120 cores held and unchanged source tables. Unknown ownership or failed
restoration latches cold-recovery-required; cleanup does not blindly rewrite
payload. No arbitrary-address, firmware-message or DMA interface is exposed.

Host ASan/UBSan tests passed for all individual TLB-write drops, payload-write
drops, bank/address aliases, wrong identities, source changes, unknown core
ownership and failed payload/TLB restoration. Existing policy and fixed-image
tests passed; all ten pre-existing diagnostic executable hashes are unchanged.

At about 21:39 local October 14 (04:39 UTC October 15), the explicit sample
test PASSED twice: seven routes, 240 core checks (120 before and 120 before
restore), 35 saved/modified regions, two patterns, zero mismatches. Data/TLB/
PCI restored flags were 1/1/1, commands ended 0000/0000 and recovery flag was
zero. Subsequent telemetry reported firmware 18.5, GDDR f7/status5515,
uncorrected errors zero, ASIC 48.48 C, fan 1,882 RPM and advancing heartbeat.
Haiku still reported 64 CPUs and max memory 137,034,936,320 bytes.

Installed optional driver SHA:
`b3d9b69dca6a300d9a367e1fdbe8e47ed56d9fad5cbebc8c4326fc8905a19ee9`.
New CLI SHA:
`0035dffe29337164e54b27a75c90527addb02c889377c68ceb51fd5ea1c5e042`.
Artifacts and prior-driver backup are in
`/boot/home/pioneer-p100a-20261014/dram-samples-2/`.
`--check-fixed-dram-samples-support` must identify loaded policy v2;
`--read-enabled-dram-routes-and-restore` is payload-read/write-free;
`--test-fixed-dram-samples-and-restore` performs the explicit sample test.

This establishes BAR4-free reversible host access to distinct high-address
samples on all seven GDDR banks. It does NOT test every byte of nominal
28 GiB, provide a runtime allocator/DMA transport, execute Tensix matrix
operations or run an LLM on the accelerator. Kernel and boot defaults and
card firmware are unchanged; no SD change or reboot was needed.

### October 14: first fixed Tensix matrix trial timed out; cold recovery

Added an original 572-byte RV32I TRISC0 fixture and fixed optional execution
transaction. It uses the sole idle tile `(1,2)`, a reset jump at `6000`, code
at `70000`, two 512-byte result arrays at `17f800`, and three status words at
`17ffc0`. The 403-word L1 footprint is saved kernel-locally. The scalar RISC
initializes fixed inputs but does no matrix arithmetic: actual `MOVD2A/B`,
supported `UNPACR_NOP` handoffs and `MVMUL` instructions are queued. Two
8x16-by-16x16 constant fixtures should yield 128 FP32 values of 16.0 and
128 of 96.0. Manual TTSync waits for instruction completion before reading
the destination. No unsupported SETDVALID, NoC command, DMA or ARC message.

This explicitly reinitializes one unused tile's coprocessor context using
documented soft reset; it does not preserve prior Src/Dst or RISC local RAM.
It first proves all 120 enabled compute tiles held, checks source translation
and entry/FIFO/sync controls, bounds completion polling, and requires the
drained completion marker, two park-PC samples, known stop/reset state and
L1 restoration. Uncertain execution leaves image bytes intact and latches
cold recovery; cleanup never rewrites potentially live code.

Offline assembly/link/bounds and independent RV32I/MMIO decoding tests passed
under ASan/UBSan: 5,292 scalar steps, 11 manual drains, two reference matrix
results; disabling modeled matrix execution fails the expected-output check.
This is a software functional model, not a model of hardware timing. Policy
tests cover individual image/TLB/reset-write drops, timeout, incorrect result,
PC, source changes and failed restoration. The RISC-V driver/tools build
passed; all 11 prior CLI hashes remain unchanged. The kernel driver stack
frames inspected were bounded (ioctl 1,808 bytes; compute policy 1,760 bytes).

At 21:58 local October 14 (04:58 UTC October 15), the first live matrix trial
TIMED OUT after 2,000 polls: started/completed/stopped 1/0/1, phase 1,
fixtures 0, marker 0. Reset readbacks were `47800 -> 87ffff -> 46800 ->
47800`, but completion was not confirmed; no matrix result is claimed.
The L1 image was deliberately not restored. TLB restored 1, outer PCI state
5/restored 0, commands 0000/0000 and cold-recovery flag 1. No further card
operation or driver reload was attempted after the latch. Haiku remained
responsive with 64 CPUs and max memory 137,034,936,320 bytes. Requested
ordinary `sync; shutdown -s` over pinned SSH, then a relay cold cycle for
recovery. A shutdown request is not proof of a clean filesystem shutdown.

First-trial installed driver SHA:
`693b2a61d7772682ed12bc7983d33c78a0924572a83272fd8cd209768fc29b66`.
CLI SHA: `1898e05062a97bf91ede1d2968b272732091bb9071c865e66e7c2434f5b34fcf`.
Artifacts and the passing GDDR module backup are retained in
`/boot/home/pioneer-p100a-20261014/matmul-1/`. Metadata-only
`--check-fixed-matmul-support` identifies policy v1. Execution is only through
explicit `--run-fixed-tensix-matmul-and-reset`. The ordinary boot and firmware
are unchanged. Next: record timeout PC snapshots/checkpoints, locate the
Tensix startup/completion stall, and cold-recover before another attempt.

The first trial was followed by an ordinary shutdown request, a relay-off
hold, more than 90 seconds off, and a fresh ZSBL/Linux boot. Linux verified
64 online CPUs, an untainted kernel and disabled P100A PCI decoding. The
next preparation found a clean BFS journal; the dirty-journal profile
correctly refused it. The equivalent pinned clean-filesystem profile
`memory128-spin-idle6-nosd-control` then passed its input checks and prepared
the same private full-RAM kernel without changing the default Linux boot.

The revised candidate records each manual completion wait's return PC in
L1 and samples the TRISC0 PC periodically, including on timeout. The
official ISA's `Bits32_UNPACR_NOP_SETDVALID.svg` places mode 7 in bits 0..2,
not bits 8..10: corrected handoffs are `43000007` / `43800007`. The initial
candidate's `43000700` / `43800700` encodings were incorrect. This source
correction alone does not establish where that initial execution stalled.
The candidate image is now 588 bytes / 147 code words, with a 407-word
preserved footprint and metadata policy v3. The offline test passes 5,336
scalar steps and 11 manual drains; this remains a functional model, not
hardware timing proof. The rebuilt driver SHA is
`e90f98933706b2ec8297391bcc0bf0cd80e8b337577efaf0c91d46ccda284ab2`;
CLI SHA `400f34aecb42d56445c9e4ed90be95b5bcaf31851b31204639d93e482ccf5bc7`.
Neither successful matrix execution nor model inference is established by
these build/source checks.

At 22:12 local October 14 (05:12 UTC October 15), the revised live trial
reached a frontend PC snapshot `70110` (the input-fill loop), then subsequent
PC, L1 status and reset reads returned zero. It refused a stop write from
that unrecognized reset state: state 7, started/completed/stopped 1/0/0,
polls 2, no completed fixture. L1/TLB/PCI restored 0/1/0, commands 0000/0000,
cold recovery 1. The initial PC snapshot is speculative, not proof of
retirement or completion. Haiku remained responsive with all 64 CPUs/full
RAM. No further card operation or module reload followed; an ordinary
shutdown request and a relay-off hold began another cold recovery. The
unexpected readbacks are unresolved; no matrix result is claimed.

### October 14: scalar controls isolate the compute startup failure

Following another shutdown request and relay cold recovery, unchanged NVMe
Linux verified 64 CPUs, taint 0, kexec slot empty and P100A command `0000`.
The pinned clean-filesystem full-RAM Haiku profile then booted again.
The historical BRISC checksum control passed (528 / `50494f4e`, matching
park PCs `70048`, started/completed/stopped 1/1/1, L1/TLB/PCI restored 1/1/1,
cold recovery 0). This establishes a working scalar execution/transport
control after recovery; it does not vindicate the failed matrix image.

Added a separate scalar-only TRISC0 control. Its fixed image has the same
19 checksum instructions, but the jump at TRISC0's reset address `6000`
targets `70000`. Only the TRISC0 reset bit changes (`47800 -> 46800 ->
47800`); no coprocessor instruction, direct-Dst access, NoC command, DMA or
ARC message is issued. A redirected TRISC0 reset entry is rejected. The
loaded metadata is v2; new explicit operations are `5461` / `5462`.
The original BRISC operations and results layout are unchanged. Both reset
jumps are independently assembled/decoded and the shared scalar instruction
body is functionally checked offline. Fault tests exercise both core modes.

At 22:24 local October 14 (05:24 UTC October 15), the TRISC0 scalar control
PASSED on the real card under Haiku: checksum 528, completion `50494f4e`,
both PC snapshots `70048`, started/completed/stopped 1/1/1, one poll,
L1/TLB/PCI restored 1/1/1, commands 0000/0000, cold recovery 0.
Driver SHA `2f201f915a5cdbedcb5c8d7f153a352663ced6281ec9a2291fd3967ab046d2c8`;
CLI SHA `2d8a8d1b373baa1c7aedefb13f9d35dc5a0c8f9ba8d0178efa99d4eff500d0d3`.
Native backups/artifacts are under `/boot/home/pioneer-p100a-20261014/trisc-1/`.

The next independent control uses that identical scalar image, but explicitly
reinitializes the sole unused tile's backends/Auto-TTSync with `87ffff`
before releasing TRISC0, and repeats the broad reset before stopping.
This is a different explicit operation (`5463` / metadata `5464`, v3), not
a silent widening of the scalar-only operation. It does not preserve prior
Src/Dst/register context and still executes no coprocessor instruction.
The versioned CLI flags are `--check-reset-trisc-support` and
`--run-reset-trisc-checksum-and-restore`. The original scalar-only flags
remain available. Uncertain reset/execution still latches cold recovery.
Driver SHA `6469b2c4df1dbe09decfac7400fe76edd2e21f822eed7b1cdbebda135a6b7f65`;
CLI SHA `32bcc4c9348a230ebb4dc07d6858d50863c09457c929fcf28fe4773764bbd16a`.

At 22:28 local October 14 (05:28 UTC October 15), that broad-reset scalar
control also PASSED: started/completed/stopped 1/1/1, three polls, checksum
528, marker `50494f4e`, both PC snapshots `70048`, restoration 1/1/1,
commands 0000/0000 and cold recovery 0. Thus both scalar core launch and
the broad reset sequence have successful live controls. This narrows the
unresolved matrix failure to the additional configuration/coprocessor/Dst
path; it is not evidence that any matrix instruction has completed.

The configuration-only control uses two independently assembled, decoded
same-width substitutions in the fixed matrix image: jump from `700d4` to
`70200`, then publish protocol count 2 instead of looping over fixtures.
This executes SETC16 configuration, backend configuration writes/readbacks,
SETRWC and four manual completion drains, but no direct-Dst access,
MOVD2A/B, handoff or MVMUL. Its output footprint must remain zero; those
zeros are explicitly NOT matrix results. New explicit operations are
`5465` / metadata `5466` (v4), with result version 2. The matrix operation
is not changed into a configuration-only operation. CLI flags are
`--check-fixed-tensix-config-support` and `--run-fixed-tensix-config-and-reset`.

At 22:31 local October 14 (05:31 UTC October 15), configuration-only
PASSED on silicon: started/completed/stopped 1/1/1, two polls, final wait
return PC `7020c`, both park PCs `70224`, marker `4d415431`, protocol count
2, untouched-zero output, L1/TLB/PCI restored 1/1/1, commands 0000/0000,
cold recovery 0. The native artifacts/backups are in
`/boot/home/pioneer-p100a-20261014/config-1/`.
Driver SHA `bf3f740571e8de515992cd3ba36b461d7a8c0993bfe9ba021e3d14ae59e5e447`;
CLI SHA `eb7c1b6839878f7a5c31fea1bc936d61814699d82a51516a967831acfe210e21`.
This narrows the failing matrix path further; the RISC-to-Dst input access,
operand moves/handoffs and matrix execution remain unverified.

Also source-audited the official minimal `tenstorrent/tt-lab` runtime at
`e6baf562a72491ed9d594cddbdf42e0925b1f79e`. Its documented one-chip model
path is useful reference, not a native Haiku runtime: it assumes Linux/x86,
exclusive KMD ownership, mandatory pinned host DMA, and BAR4 DRAM windows.
Its source is retained in an ignored no-checkout audit clone; no upstream
build, firmware-loader or installation script was executed.

### October 14: SFPU-input alternative still loses target readbacks

Replaced direct RISC-to-Dst input stores with SFPLOADI/SFPSTORE fills: all
24 input rows are written on the vector unit, with an explicit completion
drain before output ZEROACC or MOVD2A/B. Explicitly clears the SFPU stack
address base as well. The 640-byte image has 160 code words, 420 preserved
words, park PC `70258` and final-wait return PC `70240`. Matrix/config-only
metadata versions are now 5/6, so old metadata checks refuse the changed
image. The independent decoder verifies all assembled bytes, both nonzero
matrix reference fixtures, 768 SFPU input writes, 256 actual output reads,
13 drains and NO direct RISC input store. This functional test takes 2,401
scalar steps; it still does not model hardware timing. Fault/rollback and
seven-bank GDDR policy regressions pass.

At 22:39 local October 14 (05:39 UTC October 15), the new config-only control
PASSED on silicon, all restorations 1/1/1 and recovery 0. After matching
matrix metadata, one SFPU-input matrix trial failed with the same early
unexpected zero reset/PC/L1 readbacks: state 7, polls 2,
started/completed/stopped 1/0/0, no fixture/marker, L1/TLB/PCI restored
0/1/0, commands 0000/0000 and cold recovery 1. No subsequent card operation
or module reload was attempted. Haiku was still responsive with all 64 CPUs
and full RAM; ordinary shutdown was requested and a relay cold recovery
started. Bypassing direct input stores did NOT resolve the failure.

Native artifacts/backups: `/boot/home/pioneer-p100a-20261014/sfpu-matmul-1/`.
Driver SHA `d9b1586a8609b7388ac84b41e2bf3c3bb987695df17f3cdc75c9c9f726d869d9`;
CLI SHA `248bb9c1d04b723411e30e2aa30f4de9b742f8181d90b1a45345a6f8acd7664c`.
Next audit: the normal ARC power/clock initialization used before compute,
against the installed 18.5.0 firmware, not speculative firmware flashing.

### October 14: read-only operating-state snapshot

Audited official firmware tag `v18.5.0`, commit
`80b056af360a58730756467d5cc8df8db789640d`, matching the reported bundle
family. Its normal `AICLK_GO_BUSY` handler requests the maximum through
the firmware DVFS arbiter; it is not a forced-voltage/frequency command.
The default ASIC state is already A0. No state/clock/flash command was sent
on the basis of this source audit.

Added a separate bounded read-only operating telemetry selector at `5467`,
result version 2. The legacy health selector and result layout are unchanged.
Its fixed whitelist reads clocks, voltage, power limits/input, thermal trip
count and firmware/heartbeat; no serial or ASIC identifiers. Bounds, table
validation and TLB rollback use the existing tested telemetry policy.
CLI flag: `--read-operating-telemetry-and-restore`.

After the latest cold recovery, the clean-filesystem full-RAM Haiku profile
booted again with 64 CPUs and max memory 137,034,936,320 bytes. At 22:47
local October 14 (05:47 UTC October 15), the operating snapshot PASSED:
AI clock 800 MHz, AXI 960 MHz, ARC 800 MHz, Vcore 742 mV, board power limit
300 W, input power 89 W, thermal trip count 0, bundle `12050001`,
CM `000e0000`, heartbeat `12b0`. AICLK maximum is not published by this
firmware. ARC/TLB/PCI restoration all passed and commands returned 0000/0000.
These readings make a normal busy-clock test worth investigating; they do
not establish missing clock activation as the cause of the compute failure.

Artifacts/backups: `/boot/home/pioneer-p100a-20261014/operating-1/`.
Driver SHA `d66b82ed9657cef9a0b1cd8eee50496f057916281d02b46d90921e15cdccf1f7`;
CLI SHA `0090f6c27a4781e44dcaa76d098ff92c78f891b23caeeb95e9f604e5c4e22dad`.
Next: a fixed ARC no-op round trip with queue ownership/range/timeout guards,
then assess normal runtime clock activation. No firmware image is modified.

### October 14: first verified ARC request/reply under Haiku

Implemented a fixed `GET_AICLK` (`34`) request with zero arguments, rather
than assuming a declared NOP has a handler. The matching 18.5.0 firmware
defines the clock query, its nonzero clock/mode reply, four queues of four
32-byte entries, double-wrapped pointers and an eight-word header. This
implementation uses application queue 3 only. It verifies an idle, stable
queue twice, non-overlapping descriptor/telemetry/queue regions, the inbound
MSI catcher's readiness before publication, all staged request words, and
the exact pointer/counter advancement before acknowledging the response.
It never rewinds queue counters or writes firmware-owned response bytes.
Saved host-owned request bytes stay kernel-local and are restored only after
proven idle ownership. Unknown publication/completion/acknowledgement latches
cold recovery; notification is an inbound ARC interrupt, not host MSI or DMA.

The initial 128 KiB guard correctly refused the live queue descriptor
`10029ea8/00000404/0/0` before any request write. PCI/TLB restoration passed,
recovery 0. The pinned firmware DTS explicitly documents 512 KiB of CSM at
`10000000`; the queue-only limit was therefore increased to the first
192 KiB, just enough for this queue, while legacy telemetry retains its
128 KiB bound. Tests now use the observed live placement and cover exact
upper bounds, all 64 request/response pointer combinations, counter wrap,
each dropped request/pointer/TLB write, malformed/overlapping descriptors,
bad response, timeout, changed ownership and failed restoration.

New explicit operations: `5468` clock query, `5469` metadata v2.
CLI: `--check-fixed-arc-query-support`, then
`--query-fixed-arc-clock-and-restore`. There is no arbitrary message API,
clock/voltage/reset command, host DMA, firmware flash or RISC start here.

At 23:00 local October 14 (06:00 UTC October 15), the clock query PASSED on
the P100A under Haiku: submitted/replied/acknowledged 1/1/1, three polls,
status 0, 800 MHz, mode 3 (normal/unforced DVFS), MSI-catcher status 1.
Application queue is `1002a208`; request/response pointers advanced 0/0 to
1/1 and the sequence counter 0 to 1. Request/TLB/PCI restored 1/1/1,
commands 0000/0000, cold recovery 0. This proves the firmware transport,
not matrix execution or model inference. No clock-changing message was sent.

Artifacts/backups: `/boot/home/pioneer-p100a-20261014/arc-query-2/`.
Driver SHA `dc84a37a2d7c0e3512b48ec38af22f843e7a4053d07590440f46929e87e7cc13`;
CLI SHA `2580224de840261ac7027c788d56303f83b4af8fcaabb2615b50a95798dd9313`.

### October 14: normal DVFS activation passed; clocked matrix still failed

The matching system firmware v18.5.0, commit
`80b056af360a58730756467d5cc8df8db789640d`, implements normal busy (`52`)
and idle (`54`) requests through its voltage-before-frequency DVFS arbiter.
Added a fixed clock guard, not arbitrary messages or forced clock/voltage.
It requires measured idle 800 MHz, normal mode 3 and ASIC temperature at
most 70 C, verifies busy above 800 and at most 1400 MHz, and verifies return
to the original idle clock. Unknown queue state prevents further commands.
Host tests cover all individual dropped writes, failed clock settling,
idle-restoration failure and no commands after uncertain transport/compute.

At 06:08 UTC October 15 (23:08 local October 14), the real P100A passed:
800 -> 1350 -> 800 MHz, mode 3 throughout, five messages, busy/idle replies
acknowledged, clock/request/TLB/PCI restored, cold recovery 0. Operations
`546a`/`546b` expose this one fixed cycle and its metadata. Artifacts:
`/boot/home/pioneer-p100a-20261014/arc-clock-1/`; driver SHA
`ff116ac09be48073f4e7df6d3aeca8257fecd3a5525dbd88cd6d2198fe7388cd`,
CLI SHA `c3aab27eacfec8f173812fdc8b4bc363cf8c26010aff0c4168afbb3595dd81b2`.

Added guarded matrix operation `546c`, metadata `546d` v7, wrapping the
unchanged 160-word SFPU-input fixture. A first invocation safely refused
before routes, clock messages or core start; every restoration passed.
The diagnostic invocation then passed the full baseline (seven routes,
120 held cores, expected firmware/harvesting/training/error masks), entered
1350 MHz normal mode, started TRISC0, but again lost trustworthy target
reset/PC/status readings on poll 2. Matrix state 7, complete/stop 0/0,
no output or completion marker. This **does not establish a clock fix**.
No idle request was issued after compute uncertainty; cold recovery was
latched. Three ARC messages preceded compute, queue bytes/TLB restored,
math L1/context not restored, aggregate PCI state 5, command words 0000/0000.
Requested normal Haiku shutdown, then relay power-off and cold recovery.

Artifacts: `/boot/home/pioneer-p100a-20261014/clocked-matmul-1/`;
driver SHA `006f5fa75f81a38ca815dcf1a8605681dae487ac978324a890e8748b736cc577`,
diagnostic CLI v2 SHA
`db1cc8c5cb53ac6b03b5031c279db62031d0725cba563bb2b1c6bd2f578a14e9`.
Neither matrix execution nor any model inference has passed. Next isolate
the first SFPU/data-path operations, retaining all cold-recovery gates.

### October 14: vector execution passes; operand preparation isolates failure

Added separately assembled fixed-prefix controls with unchanged 160-word
code/420-word L1 footprint and common drain/park/restore protocol. New
operations `546e`/`546f` (metadata v8) run two SFPLOADI instructions only;
`5470`/`5471` (v9) additionally execute the twelve SFPSTORE instructions
covering 24 rows of Dst, but no ZEROACC/MOVD2/matrix/RISC Dst reads.
The assembler-byte comparison, constrained functional decoder and kernel
policy/fault tests pass. These controls require their zero output footprint
to remain unchanged; count 2 is a protocol marker, not two matrix results.
An offline completion-marker-before-drain mistake was caught and fixed
before deployment. Neither test numerically reads back Dst on hardware.

At 06:24 UTC October 15, SFPU-load-only PASSED on P100A: started/completed/
stopped 1/1/1, two polls, expected final phase/marker and two parked PCs,
L1/TLB/PCI restored 1/1/1, recovery 0. Artifacts
`/boot/home/pioneer-p100a-20261014/sfpu-load-1/`; driver SHA
`658be222d062267a8106725d40cd2f3049de5403f160e76a631af31dd4d878aa`,
CLI SHA `26f0434df9355d73adfe293aa2297e57e7a70b4fc13641e1e507a6d5005a5461`.

At 06:27 UTC, SFPU-store-only PASSED with the same complete/stop/restore
checks, one poll, recovery 0. Artifacts
`/boot/home/pioneer-p100a-20261014/sfpu-store-1/`; driver SHA
`32a2fbda464205a83c3b690e0d8c77df2de925bb0b80dab9b8209a8ce5323f37`,
CLI SHA `109002f28743e7d2c045637f879d7e97759d5de54924eba6a38092f7a1d3829f`.

Added operand-only `5472`/`5473` (v10): same SFPU prefix, ZEROACC,
MOVD2A/B and UNPACR_NOP handoff, then drain/park without matrix or RISC
Dst reads. Also added matrix-exec-only `5474`/`5475` (v11), which issues
one MVMUL/drain but skips numerical readback. At 06:31 UTC, operand-only
FAILED: start/complete/stop 1/0/0, poll 2 target reset/PC/status all zero,
state 7, L1 not restored, TLB restored, aggregate PCI state 5, recovery 1.
Matrix-exec-only was therefore NOT run. Card/root commands remained 0000.
Requested normal shutdown and relay power-off; no further card commands.

Artifacts `/boot/home/pioneer-p100a-20261014/operand-1/`; driver SHA
`bd630ce8d779938af3b185d8e98e7fc9a5153b5bdfb4f57409cd1b71d026d237`,
CLI SHA `9073be2f688cd2663fd5a0a2f1ff9031a4afbf9854b0e52a8a83d810127e1747`.
The failure is now narrowed to ZEROACC, MOVD2A/B or operand handoff, not
necessarily MVMUL or RISC Dst reads. No numerical matrix or model success.

### October 14: ZEROACC/MOVD pass; handoff remains non-repeatable

After another default Linux cold recovery and clean full-memory/64-CPU
Haiku boot, repeated SFPU-store control passed. New fixed ZEROACC-only
(`5476`/`5477`, metadata v12) and MOVD-only (`5478`/`5479`, v13) controls
both PASSED at 06:38 UTC October 15: completion/stop 1/1, three/two polls,
expected phase/marker/park, L1/TLB/PCI restored, recovery 0. Both bypass
UNPACR_NOP handoff, MVMUL and RISC Dst reads. This narrows the failing
prefix to the handoff rather than SFPU, ZEROACC or MOVD2A/B individually.
Artifacts `/boot/home/pioneer-p100a-20261014/zeroacc-1/`; driver SHA
`ac0d9b7bcde0133cc985f7d93a6fe272578c505574bc6caa13a4746e4ab6967c`,
CLI SHA `e1f13d2dc3c74313416f20836897c729e0fd4d285448659d2d997dbc99ebba89`.

The ISA's pure-handoff `43000007`/`43800007` differs from the pinned
LLK's actually used dummy-valid helper: `SET_DVALID=1`, `UNP_ZEROSRC=1`
through its macro yields `43000101`/`43800101`. Added a separately selected
kernel-style variant using that clear-and-valid sequence BEFORE MOVD2
(otherwise the clear would erase inputs). It executes once, retaining
Matrix ownership through the second fixture. It is not silently substituted
into the existing diagnostic variants. All images retain their fixed
footprint and are independently assembled/decoded, including a numerical
model that fails if matrix execution is removed. New metadata v14 operand
(`547a/b`), v15 matrix-execution-only (`547c/d`), v16 full numeric (`547e/f`).

At 06:43 UTC, kernel-operand-only PASSED once: one poll, complete/stop and
all restoration checks passed, recovery 0. The immediately following
kernel-matrix-exec-only test TIMED OUT, 2000 polls, phase `70190`, two PCs
`70274`, still-running reset `46800`. That phase is the handoff drain,
BEFORE reaching MVMUL. Stop/reset succeeded but L1 was deliberately not
restored after uncertain completion; cold recovery 1. No numeric full
matrix test was run. Thus this is **not a proven repeatable handoff fix**.
Requested normal shutdown and relay cold recovery. Next test the numerical
variant first from a fresh cold state, without a preceding dummy-valid test.

Artifacts `/boot/home/pioneer-p100a-20261014/kernel-handoff-1/`; driver SHA
`716ca2cc741892ab2a0584d69d67508b57859868b0278aa6549e2e684aea752c`,
CLI SHA `6d1ecbff645d8a0c8293acd2263fae80e53dc285dce86859496d6e81d9b8c23b`.
No firmware, clock-gating, voltage-force or default-boot changes were made.

### October 14: matrix completes with wrong values; Dst numerical control passes

Following a timed (>60 s) relay off interval, default Linux cold boot and
clean 64-CPU/full-memory Haiku handoff, the v16 kernel-style numerical
matrix fixture ran FIRST at 06:50:45 UTC October 15. Both fixture markers,
drains, parked PCs and stop completed. L1/TLB/PCI restoration passed,
recovery 0, but all 256 numerical outputs mismatched: representative bits
`72741ef6/72741ef6`, not `41800000/42c00000`. Matrix state 6, aggregate
PCI state 2 (bad result, not failed restoration). This is execution progress,
NOT numerical matrix success. Same kernel-handoff-1 artifacts as above.

Added fixed SFPU-fill/RISC-Dst-read control `5480`/`5481`, metadata v17,
result v12. It bypasses ZEROACC, MOVD2, handoff and MVMUL, reading known
input rows 0..7 after SFPU fills (1.0 then 2.0). Independent assembly and
decoder agree; removing SFPU writes breaks the model's numerical check.
At 06:56 UTC this control PASSED on P100A: 256/256 output values correct,
representative bits `3f800000/40000000`, start/complete/stop 1/1/1, four
polls, L1/TLB/PCI restored, recovery 0. This numerically establishes the
SFPU-fill and RISC-Dst-read path, not matrix operands or arithmetic.

Artifacts `/boot/home/pioneer-p100a-20261014/dst-read-1/`; driver SHA
`9d1022bbe7f559537eb9fbd3150857034eb080953b68fc4d35ae3ebd37ccbb94`,
CLI SHA `b024b9ecb57c5344ff6b08420f903236a8716f81146e9063e4ce065ccf0467ae`.
Next numerically round-trip SrcA/SrcB separately to isolate MOVD2 formatting
from MVMUL. The pinned LLK has a Blackhole MOVD2B/B2D FP32 workaround
(issue 449); the generic ISA functional descriptions are not timing/silicon
proof. No speculative forced clock/voltage or firmware change is justified.

### October 15: source-register numerical controls and pure-valid alternative

Added SrcA/SrcB round-trip controls (`5482/3`, metadata v18; `5484/5`, v19),
using MOVA2D of eight rows / MOVB2D broadcast of one row, never MVMUL.
Both use the kernel-style clear-and-valid handoff first. At 07:03 UTC the
SrcA test timed out at phase `70190`, PCs `70274`, before the source check.
Stop succeeded, but completion was uncertain, so L1 was not restored and
cold recovery was latched. SrcB was NOT run. No numerical inference about
SrcA can be drawn from a timeout before reaching it. Normal shutdown and
relay power-off requested/completed; all card commands remained 0000.
Artifacts `/boot/home/pioneer-p100a-20261014/src-roundtrip-1/`; driver SHA
`b3c54d468a475046d4c4a46952b6ef193295f651dea5be9fe0f7eb5d04f18e73`,
CLI SHA `d5b8e3c197a1abfbf91057a0695510eb295e384b7f648626268d0082bc846362`.

The pinned LLK Blackhole `instructions/assembly.yaml` documents a separate
pure-valid form: Clr_to1_fmt_Ctrl bits 7:6 = 3, with UNP_NOP bits 1:0 = 2.
This yields `430000c2/438000c2`, distinct from both the ISA's low-mode-7
form and the clearing `43000101/43800101` helper. Added explicitly separate
fixed variants: SrcA `5486/7` metadata v20, SrcB `5488/9` v21, full matrix
`548a/b` v22. They change only these two handoff payloads in the kernel-style
prefix, with no register/overlay-write modes, clearing, DMA or ARC command.
All 17 assembled code variants and the reset-jump section match the
independently decoded constants and policy tests. This is a prepared
experiment, NOT yet a hardware result.

At 07:12 UTC October 15, the fresh-cold-boot pure-SrcA test TIMED OUT:
phase `701e8` (after MOVA2D), PCs `70274`, 2000 polls, running reset
`46800`, stop succeeded. Thus the NOP/format-3 form got past its handoff
drain but did not provide a readable Matrix-owned SrcA bank on this setup.
No SrcB or MVMUL variant was run. Cold recovery was latched, L1 deliberately
left untouched, normal shutdown and relay power-off completed. Artifacts
`/boot/home/pioneer-p100a-20261014/pure-handoff-1/`; driver SHA
`32de6fbb87ca6e751d1d360b7587d668ccb10b2ef6320ed665d32c56a232affe`,
CLI SHA `29f8a6b8dd3010c6826878e5eba28cac5df7f30a2b40eff4fb8eb332642c52ec`.
Next investigate the standard bounded L1-to-SrcA/B UNPACR path rather than
treating another guessed standalone handoff encoding as a fix.

### October 15: first repeated numerical matrix success via normal L1 unpack

Added a separate fixed TRISC0 program using normal uncompressed BF16
L1-to-SrcA/B UNPACR loads, FP32 arithmetic and RISC Dst readback. It issues
no device NoC requests, host DMA, ARC commands, packer or firmware updates.
The sole idle tile's compute context is deliberately reinitialized, not
preserved. Original L1 words remain kernel-private and are restored only
after proven completion/stop. Legacy images retain their original layout.

New image: 648 words covering code/private inputs, 908 total preserved
words (3,632 bytes), park `703bc`, final wait return `703a4`. Code ends at
`70404`; A header/payload occupies `70700..7090f`, B `70910..70a1f`, all
inside the existing 4 KiB mapped code page. No instructions execute from
input buffers. The snapshot now uses a bounded kernel heap allocation;
null allocation refuses before writes, avoiding kernel-stack growth.
Completion blocks ThCon until CFGEXU, both unpackers, FPU and SFPU are
idle, then polls a GPR63 SETDMAREG marker. No PCBuf completion read is used.

Configuration-only v26 passed first. Initial source tests completed and
restored but matched only one SrcA element / one broadcast SrcB column.
Our SETADCXY mask `f` cleared X1, the input end counter, after establishing
255/127 bounds. Y-only mask `a` preserves those bounds, as the runtime does.
The decoder now tracks both X channels and rejects clobbered bounds.
At 07:38 UTC all 256 SrcA and all 256 SrcB values then PASSED.

Matrix returned 19 and 115, consistent with accumulating onto the prior
SrcB test's 3, then onto 19. The one-row ZEROACC sequence did not invalidate
this FP32 block on the live card. Switching this L1-only path to the
runtime's all-Dst clear fixes it. A regression preloads Dst with 3 and
reproduces 19/115 if the clear is removed. The precise silicon cause of
the former one-row clear behavior is not established.

At **07:42:36 UTC October 15**, actual MVMUL PASSED all 256 outputs:
`41800000/42c00000` = 16.0/96.0, complete/stop 1/1, one poll, zero
mismatches, L1/TLB/PCI restored, recovery 0. An immediate WARM repeat also
PASSED, three polls, no driver reload or cold recovery. MVMUL releases/flips
both operand banks; source round-trip controls release them after readback.
Haiku still reports 64 CPUs and 137,034,936,320 bytes maximum RAM (127.62
GiB usable). Subsequent operating telemetry passed: idle 800 MHz, 742 mV,
input 89 W, thermal trips 0, advancing heartbeat, command words 0000/0000.

Passing operations: `548c/d` matrix/metadata v31, `548e/f` SrcA v32,
`5490/1` SrcB v33, `5492/3` configuration-only v34. CLI flags start
`--check-l1-unpack-...-support` / `--run-l1-unpack-...-and-reset`.
Artifacts `/boot/home/pioneer-p100a-20261014/l1-unpack-3/`; driver SHA
`e4a4f2d212f2de7b06043b9ed5da85c7c708933e85d080f93f0407076fb38a59`,
CLI SHA `875b4c43db93d4cc5ff2c53e3859d3ec2f4f3cc68b3f49fef8d9cd6ec223abd1`.
Earlier underfilled v23..26 artifacts: `l1-unpack-1/`, driver
`70974fb9f229fd181c2cc1a200f674dc9076b7c66212af65cd43de4b080846e2`, CLI
`e9627ecc3f1dfcdd47ec0180cf9229935724ec5d10c43b1644b96cd39cf3b547`.
Corrected-bounds/stale-accumulator v27..30: `l1-unpack-2/`, driver
`e1a4bf366587ddfe0e68229cb9b1340771e06917f99ddcaa4690da2e8e97b4ec`, CLI
`e5dd619aa86cba47cc8e1880b90561b397936bd4cf7c75ed5dfea5ad7a4ec4ef`.

All 21 code variants are independently assembled/decoded. Tests cover
rollback, recovery gates, null allocation, ADC bounds and stale Dst; removing
actual unpacking or MVMUL breaks the numerical reference. This does NOT yet
prove nonuniform layouts, a scalable runtime or model inference. Next test
nonuniform data before model-backend integration. Default Linux boot,
firmware and both SDs remain unchanged; no BAR4 enlargement/bus mastering.

### October 15: nonuniform, mixed-sign layouts verified

At **07:59 UTC**, the new SrcA dense readback, SrcB patterned broadcast,
identity multiplication and nonsymmetric dense multiplication all PASSED
on the live P100A. Each operation checked all 256 FP32 outputs against an
integer CPU oracle. The dense multiplication also passed an immediate
warm repeat. All runs completed/stopped, restored L1/TLB/PCI, and reported
recovery 0 and commands 0000/0000. Haiku retained all 64 CPUs and
137,034,936,320 maximum RAM bytes.

Input B rows contain distinct values in [-8,7], with a second independent
fixture `-B-1`; dense A is `(row+2*column)%5-2`. A separate identity A
establishes orientation. The independently assembled RV32 program only
copies bounded BF16 input tables; actual MVMUL generates the outputs.
The decoder independently calculates the reference and rejects a transposed
A layout or removed MVMUL. It also verifies all 25 program variants,
read/write whitelists and existing rollback/recovery gates.

New fixed operations 5494/5 identity v35, 5496/7 dense v36, 5498/9 SrcA v37,
549a/b SrcB v38 preserve the same 908-word footprint and completion PCs.
Native artifacts: `/boot/home/pioneer-p100a-20261014/nonuniform-1/`.
Driver SHA `8bfc79537c1f07be7ceaf388183589ec20547e838e8a4d570e391e77a25bf32e`;
CLI SHA `641cf396986aa3d573e32ff4f58acf278d16c540b9397fabbc5799d076bcaa1b`.
This proves these bounded numerical layouts, NOT model inference or a
scalable backend. Next add a bounded data-only input/output interface;
no user-controlled device addresses, code or DMA descriptors.

### October 15: bounded data-only BF16 interface and model bootstrap

Added root-only ioctl `549c` with metadata `549d` v39. Request is exactly
two 16x16 BF16 arrays (`A[k][column]`, `B[row][k]`); output is 16x16 FP32
`B @ A`. No caller-controlled addresses, code, dimensions or descriptors.
NaN/Inf and nonzero subnormals are rejected before hardware access.
The driver snapshots inputs on a bounded heap allocation, zeros results,
and never reads userspace during execution. Result state **10 EXECUTED**
is deliberately distinct from a numerical fixture PASS. Output is erased
if completion, stop or L1/TLB/PCI restoration fails. Cold-recovery gates
remain in force. Context on the single idle tile is reinitialized.

The separately assembled program adds four fidelity phases at `70480..704c3`,
retaining both source banks until the last MVMUL. Thread register 11 selects
the phase; each change and multiplication is explicitly drained. Code/data
and snapshot footprint remain 908 words. The decoder compares the complete
assembled image, emulates all four phase contributions, and verifies that
omitting low phases fails. Parameterized policy tests cover data-write loss,
invalid inputs and timeout/no-restore rules; the source lock audit now has
22 guarded handlers.

Live driver SHA `295c8c82f6a39a07cd7d74c8cb237d7602226c7de445951a06140c407d2ae0bb`,
artifact folder `/boot/home/pioneer-p100a-20261014/data-tile-1/`.
Nine diagonal/equal-exponent reduction controls passed **exactly**. The
full-mantissa mixed-exponent sweep is NOT bit-exact: an initial absolute
1e-5 target failed, and a normalized 1/4096 target missed one output in
trial 6. At **08:19 UTC**, all eight trials / 2048 outputs passed the explicit
BF16-level acceptance target `abs(error) <= 1e-6 + sum(abs(products))/1024`.
Measured maximum absolute error 0.00425720215, maximum normalized error
0.000244877592, elapsed 1.342 seconds. This is an empirical acceptance
test, not a universal hardware error bound or a diagnosis of the precise
mixed-exponent reduction rounding. Official MVMUL documentation explicitly
does not promise IEEE-exact reduction. The earlier strict failures remain
important evidence; do not describe this path as bit-exact FP32 GEMM.
Sweep CLI SHA `d47c088ca83b11ed84f7c5f60d43e21c8c11844acd28510bd84004fb8cd5e170`.

`tenstorrent_haiku_tinyllama.cpp` is a strict fixed-shape 260K TinyStories
bootstrap, not a Qwen loader. Every learned linear projection and classifier
uses actual P100A BF16 tile operations, without CPU fallback. Host software
does normalization, attention/KV, activations, embedding lookup and partial
tile sums. A separate full-FP32 model context compares every generated
token's 512 logits (cosine >=0.999, relative L2 <=0.02, exact argmax agreement).
The offline implementation matches the independently compiled pinned
karpathy/llama2.c reference; all eight BF16-emulated greedy tokens agree:
`Once upon a time, there was a`. Those are **offline** results, not yet a
claim about live P100A inference. A two-token live test is underway.

At **08:23 UTC**, the first LIVE token completed: id 403, ` Once`, identical
to the independent CPU argmax. Logit cosine 0.999997107, relative L2
0.002905042, maximum error 0.0623066. All 1028 real device tiles completed
and restored; elapsed 173.071 seconds. This is hybrid inference with every
learned linear projection on the P100A, not all-device execution. The next
token is still being tested. Native harness SHA
`a1de72855ce62f2bdf94985da281c7d44ea1a58de6f7f3fa593c10fc6585843d`,
artifacts `/boot/home/pioneer-p100a-20261014/tinyllama-1/`.

At **08:26 UTC**, the two-token LIVE run PASSED: ` Once upon` (ids 403,407),
both matching the independent full-FP32 CPU argmax. Second token cosine
0.999997918, relative L2 0.002064653, maximum logit error 0.0702281. All
2056 actual P100A tile transactions completed/stopped/restored; total
346.172 seconds. The P100A, not a CPU fallback, computed all learned linear
projections/classification; CPU still handles the other listed operations.
This establishes a very small working hybrid model, not the requested 27B
model, useful throughput, resident weights or all-device inference.

Pinned model author/revision: karpathy/tinyllamas
`0bd21da7698eaf29a0d7de3992de8a46ef624add`; raw model SHA
`b0a507e7ad0f626624f17112325e66691f9076d622e1d3274d103d00299f2696`,
tokenizer SHA `037cb335abb25d1fa9e8ecae30ed2a3a8ace9302862ebcdc05d51a6bbb10c312`.
`tenstorrent_fetch_tiny_bootstrap.sh` fetches only pinned plain C/license,
raw float checkpoint and tokenizer, verifies hashes, and executes nothing.
Do not download/execute the `.pt` pickle. The reference source remains
unmodified in ignored artifacts, with its original MIT license.

Reproduce offline checks:

```sh
sh tools/pioneer/tenstorrent_fetch_tiny_bootstrap.sh /absolute/fresh/artifacts
sh tools/pioneer/tests/run_pioneer_tinyllama_tests.sh /absolute/fresh/artifacts
sh tools/pioneer/tenstorrent_cross_build_tools.sh CONFIGURED_BUILD /absolute/fresh/tools
```

The diagnostic per-tile save/check/restore path is far too slow for 27B.
Qwen3.8-27B 4-bit, resident GDDR weights and a scalable batched backend remain
separate, unfinished milestones. No firmware/boot/SD changes or DMA enable.

### October 15: bounded batching and eight-token live inference

Added data-only batch ioctl `549e`, metadata `549f` v40. It accepts 1..16
fixed-shape inputs, privately snapshots/validates all before hardware access,
and holds the same lock/PCI reservation throughout. Original L1 is saved
once/restored once. Every job is independently started, drained, parked and
stopped; only the 256 input-table words and three progress words change
between jobs. Executable code and entry are untouched. A failed batch
erases all public outputs. A timeout after job one still forbids L1 restore
and cold-latches the driver; dropped second-job data before start safely
restores the original snapshot. Offline tests cover all 16 ordered results,
invalid final input, count bounds, mid-batch timeout and dropped data.
The lock audit now has 23 guarded handlers. Single-job/legacy tests pass.

At **08:30 UTC**, eight real matrices passed in one transaction, 2048 checked
outputs, same numerical error as the single-job test, elapsed 0.189 s. An
immediate warm repeat passed in 0.191 s, recovery 0, full restoration.

At **08:33 UTC**, the live batched pretrained-model run PASSED all eight
tokens: ` Once upon a time, there was a` (ids 403,407,261,378,432,383,286,261).
All CPU argmaxes matched. Across tokens cosine >=0.999989242, relative L2
<=0.005256786, max logit absolute error <=0.110325. All **8224 real device
tiles** completed/stopped/restored, total **123.050 seconds**, about 15.38
seconds/token vs 173.09 before batching (11.25x faster). The first two tokens'
logits were identical to the unbatched live results. Host attention/norm/KV
and partial sums remain; no CPU fallback for learned projections.
Post-run health: 64 CPUs, 137,034,936,320 maximum RAM bytes, card idle 800 MHz,
742 mV, 90 W input, zero thermal trips, advancing heartbeat, PCI commands
0000/0000. No cold recovery, boot changes, firmware flash, SD changes or DMA.

Artifacts `/boot/home/pioneer-p100a-20261014/batch-1/`: driver SHA
`c0cdb75a7f2d4a90638be45660e99121b02deb0152656fc96643071ff3c32a77`,
matrix CLI `9dee6fe46afbc29b851030dd617cb27fa2dfb7cf95fd0dcd9ec9cd4e9c32b925`,
model harness `3f7571fdfd2098b4d5ac5d001a3453f8d45fecc38a40e03099ba9545713fb968`.
This is a working tiny hybrid language model, NOT Qwen3.8-27B. Resident
quantized weights, device-side transfers and a scalable model backend remain
necessary for that target. Default boot remains Linux and both SDs stay out.

### October 15: requested Qwen GGUF preflight and mixed-format projections

Pinned primary model candidate: [ggml-org/Qwen3.8-27B-GGUF](https://huggingface.co/ggml-org/Qwen3.8-27B-GGUF),
revision `71bc7b627595dc8a91039addd9c791ae548d6747`, file
`Qwen3.8-27B-Q4_K_M.gguf`, **18,973,870,528 bytes** (17.67 GiB).
Published full LFS SHA-256:
`c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747`.
Only a 16 MiB header range and three small tensor ranges were downloaded,
NOT the complete model. Range hashes do not verify that full-file checksum.
HTTP responses were bounded 206 ranges from that immutable revision.

The new read-only `tenstorrent_gguf_preflight.py` parses GGUF v3 with bounded
lengths/counts, validates tensor formats/alignment/non-overlap/full-file
bounds and never evaluates chat templates or executes a model. Tests reject
every header truncation, duplicate keys, bad alignment/types/dimensions and
out-of-file tensors. Actual header ends at byte 10,994,112 and describes
851 tensors: **193 Q4_K, 288 Q8_0, 17 Q6_K and 353 F32**. It is not uniform
4-bit. Architecture is `qwen35`, 64 layers, width 5120, FFN 17408, recurrent
inner width 6144, full-attention interval 4. The TinyStories harness cannot
load this graph; convolution/Gated DeltaNet and gated-attention execution,
tokenization and memory planning remain substantial unfinished work.

Independent original host block decoders now cover Q4_K (144 bytes/256
weights), Q8_0 (34/32), Q6_K (210/256) and IEEE FP16 scales. Offline fixtures
test all finite FP16 encodings, 16384 Q4_K, 8192 Q8_0 and 65536 Q6_K values,
including signed scales/quants and rejection of nonfinite scales.
No existing llama.cpp files were changed. Native probe uses bounded host
block decode/BF16 conversion, P100A matrix multiplication and host partial
sums. It compares both decoded-FP32 and BF16 CPU references for 16 real
weight rows with a synthetic activation vector; this is NOT complete model
inference or device-side quantized storage/dequantization.

At **08:41..08:44 UTC**, actual Qwen weights PASSED:

| Tensor sample | Shape tested | CPU FP32 cosine | Relative L2 | Device tiles / transactions | Time |
|---|---|---|---|---|---|
| Q4_K `blk.0.ffn_gate.weight` rows 0..15 | 5120 → 16 | 0.999997570 | 0.002212184 | 320 / 20 | 4.337 s |
| Q8_0 `blk.0.attn_gate.weight` rows 0..15 | 5120 → 16 | 0.999998024 | 0.002027083 | 320 / 20 | 4.330 s |
| Q6_K `blk.3.attn_output.weight` rows 0..15 | 6144 → 16 | 0.999998654 | 0.001672537 | 384 / 24 | 5.231 s |

All outputs satisfied the separate BF16 numerical budget; every transaction
completed/stopped/restored without recovery. Driver remains batching SHA
`c0cdb75a7f2d4a90638be45660e99121b02deb0152656fc96643071ff3c32a77`.
Latest native projection CLI SHA
`ed5b2b1e2006a05bccfff851f21e552a34ba25f38b1f5f3ddb86e8d2f9149e3e`,
folder `/boot/home/pioneer-p100a-20261014/qwen-mixed-1/`.
The earlier Q4-only CLI was
`5719b881425a9f9ecb7f38b33587e49f0772197952a9ce60fcf4fee9bab1ad3e`
in `qwen-q4-1/`.

`tenstorrent_fetch_qwen_preflight.sh` reproduces bounded header and tensor
ranges, verifies exact lengths, Content-Range and local range hashes. It
does not execute code, connect to the card, fetch pickle files, or download
the complete 19 GB model. Run the inspector with declared full size
18973870528. Do not treat the header range as a runnable GGUF file.

Remaining scale gate: resident quantized weights and device-side transfers
must replace host PIO weight streaming, then implement/validate the complete
64-layer hybrid graph. A model-sized BF16 expansion would exceed 28 GB and
must not be used as the residency plan. No SD, default boot, firmware or DMA
changes were needed for any of these results.

Download the other pinned official sources with:

```sh
sh tools/pioneer/tenstorrent_fetch.sh
```

Default location is ignored `generated.pioneer/tenstorrent/`. Existing trees
are never overwritten. No upstream build/install scripts are executed.

| Component | Official source | Investigated revision | License notice |
|---|---|---|---|
| Kernel driver | https://github.com/tenstorrent/tt-kmd | 22cca4cb33455d962cf8340a8f5b896dae696fd7 | GPL-2.0-only; ioctl header has Linux-syscall-note |
| User-mode driver | https://github.com/tenstorrent/tt-umd | 5c9ea40f8e29f84c32dde2646f93e19e1d13dd9a | Apache-2.0 |

Keep original notices and provenance. Do not relabel copied GPL driver code
as MIT. Packaging/upstream acceptance needs an explicit licensing review.
These pins are investigation snapshots, not a validated mutually compatible
release pair or a firmware recommendation.

### October 15: device-residency prerequisite, not Qwen acceleration

The existing tile interface cannot become a practical Qwen accelerator just
by removing reference comparisons or enabling its UI entry. It streams tiny
BF16 weight tiles over host PIO, executes on one tile, and repeatedly starts,
drains, stops and restores it. Qwen needs persistent device weights and much
larger, parallel device-dispatched operations. Qwen remains CPU-only in the
Assistant; TinyStories remains explicitly experimental, not a Qwen fallback.

A fresh CPU comparison baseline used the existing native llama.cpp
`c060ca974c77`, installed Qwen3-0.6B-Q8_0 (SHA-256 `9465e63a...`), 32 CPU
threads, batch/microbatch 128, GPU layers 0, normal warmup and three repeats:
prompt processing 128 tokens `32.219 +/- 0.303` tokens/s; generation 64 tokens
`11.695 +/- 0.091` tokens/s. See the unmodified native
[JSONL results](benchmarks/haiku-qwen3-0.6b-q8-cpu-residency-baseline-20261015.jsonl).
Prompt and generation are separate llama-bench cases, not an Assistant
end-to-end latency result. A preceding one-repeat prompt32/generation16
check gave `33.888/12.015` tokens/s. Benchmarks ran in a separate process,
without switching the open Assistant away from TinyStories. Do not compare
these numbers to the much smaller TinyStories model as an acceleration ratio.

Two new root-only, fixed-address operations were implemented and deployed:

- `tenstorrent_haiku_compute_noc --read-compute-noc-and-restore` first
  verifies all seven GDDR routes and all 120 enabled tiles held reset. It
  samples physical `(1,2)` NoC0's identity, translation configuration, four
  command controls and sixteen outstanding-response counters three times.
  Only temporary BAR/TLB controls are written. The live samples agreed:
  node `10611081`, endpoint `00010001`, NIU configuration `00006005`, all
  commands/counters zero, request FIFO disabled. TLB/PCI rollback passed.
- `tenstorrent_haiku_resident_read --read-fixed-gddr-to-l1-and-restore`
  repeats the ownership/route gates, requires unused initiator 0 and no NIU
  interrupt enables, then reads bank 0 at 64 MiB through the host window for
  comparison. Those bytes stay kernel-local and GDDR is never written.
  It saves the final 64 bytes of `(1,2)`'s L1 and poisons them with the
  inverse of the expected bytes. It temporarily enables the NIU request
  FIFO and issues exactly one 64-byte read from GDDR to that L1 address.
  Both endpoints are on-card; bus mastering/host DMA stays disabled.

Completion requires command-control readback before the first counter read,
exactly one additional read response, data flit, command acceptance and read
request sent, all initiators idle, all outstanding/outgoing ID counters zero,
stable source routes and unchanged stopped-core state. A command accepted
without its completed response is not success. The destination must then
match the independently read GDDR bytes. Only proven completion/ownership
permits restoring L1, initiator fields and the original FIFO setting. A
timeout or uncertain route/completion leaves payload/control state untouched
and latches cold recovery; never retry, clear counters or reload the driver
to bypass it. Event counters naturally advance; this does not roll them back.

The first live transfer and the final stricter-flit-check build both PASSED:
issued/completed `1/1`, one poll, zero mismatches, counter deltas `1/1/1/1`,
L1/NIU/TLB/PCI restored `1/1/1/1`, card/root commands `0000/0000`, cold flag
zero. No core was released or reset-written. Metadata queries also confirm
the old data-tile/batch ABI remains available. Neither test transfers model
weights, proves all-bank bandwidth, nor executes Qwen on the device.

The source policy tests use ASan/UBSan and inject every individual TLB,
initiator, FIFO, doorbell and L1 write loss, timeouts, accepted-but-incomplete
commands, incorrect flit counts, data corruption, source changes, active
cores, enabled interrupts, counter wrapping and permanent L1 rollback
failure. They verify no request retry, counter clear or NIU/L1 rewrite after
uncertainty. All thirteen PCI/ARC/core/L1/DRAM/compute policy suites pass;
the existing independent TinyStories/reference and Assistant-adapter tests
pass too. The optional driver builds through Jam and all twenty-one tools
cross-build with warnings as errors.

Final installed driver SHA-256:
`4e128ef4857f6e311ce0c784a6020632e711a860bb08c27892ba48bb43d68790`.
The source files were synced to `/Volumes/HaikuBuildLocal/haiku` before the
build. Verified native backups/evidence are in
`/boot/home/pioneer-compute-noc-20261015/` and
`/boot/home/pioneer-resident-read-20261015/`; copied logs are ignored under
`generated.pioneer/tenstorrent/resident-read-20261015/`. No Assistant binary,
conversation, settings, model, boot medium, kernel, firmware or CPU/RAM
configuration changed during these tests. The original dirty llama.cpp
submodule was left untouched.

Register semantics were audited against official
[NoC memory-map documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/NoC/MemoryMap.md)
and [completion-counter documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/NoC/Counters.md).
The implementation is original fixed-policy code, not an upstream Linux
runtime transplanted onto Haiku.

Remaining acceptance gates for a real Qwen P100A mode:

1. Own bounded GDDR allocations, upload resident weights once, verify their
   contents and lifetimes, and safely release them on cancel/exit. Preserve
   quantized storage for the requested 27B model; full BF16 expansion does
   not fit its 28 GB card.
2. Build persistent, larger matrix-vector kernels using device-side
   GDDR-to-L1 transfers; validate first on one tile, then distribute work
   across the enabled tiles. Do not use a host ioctl/reset per tiny tile.
3. Integrate a real Qwen inference backend: begin with installed ordinary
   Qwen3-0.6B and independent layer/logit references; the requested 27B
   `qwen35` hybrid graph and mixed quantization require additional support.
4. Validate generation, KV/context growth, deterministic comparisons,
   cancellation, repeated requests, model switching and recovery, with no
   silent CPU fallback.
5. Measure the same model/quantization, prompt and output lengths against
   the CPU using repeated warm runs, time to first token, prompt throughput,
   sustained generation and total latency. Include cold load separately.
   Only expose a practical faster-Qwen mode after this gate passes.

### October 15: compute-core-driven resident-input matrix proof

The fixed `TT_UNPACK_RESIDENT` variant of `blackhole_tensix_unpack.S`
retains the independently verified full-fidelity BF16 matrix program and
its original completion/park addresses. L1 activation/weight tables are
initialized to zero, not usable fixtures. A bounded NoC reader is appended
outside the input buffers, within one fixed 4 KiB code/data page. Before
each of the two matrix fixtures, TRISC0 issues a read of the same 1,088-byte
resident GDDR block into its private L1 input area. The host cannot ring a
NoC doorbell through this operation, and performs no intervening data writes.

The reader checks an idle initiator, records the expected response count,
reads back its doorbell before polling counters, waits for that response
and zero outstanding requests, then fences before consuming externally
written L1. Its 65,536-iteration deadline parks with `NOCF`, never `MAT1`,
on uncertainty. It never retries a request or clears counters. The linker
and independent RV32I/FPU/NoC simulator verify exact assembled bytes,
executable bounds, both dense numerical outputs, delayed transfers,
corruption detection and missing/incomplete-response refusal.

The optional root-only `tenstorrent_haiku_resident_math` ioctl first runs
the existing firmware, seven-bank route, 120-held-tile and compute-source
checks. It saves/validates the fixed GDDR block and L1 footprint, uploads
one known dense BF16 fixture block, then installs/verifies the poisoned
program. The NIU's fixed command fields and request FIFO are configured and
verified before releasing only TRISC0. The host requires normal math drain,
two fixtures, `MAT1`, two park-PC samples, exact NoC counter deltas
`2/34/2/2` (responses/flits/accepted/read requests), all initiators idle and
all outstanding/outgoing counters zero before declaring completion.

Unlike the earlier no-NoC tile test, a timeout does **not** assert reset:
an outstanding request could still overwrite L1 after a RISC parks. Unknown
execution, route, completion or stop therefore forbids further reset,
NIU/control or payload writes and latches cold recovery. Only verified
completion and stop permit restoring original NIU fields/FIFO, L1, GDDR,
TLBs and PCI state. Src/Dst, local RISC state and coprocessor configuration
are explicitly initialized, not claimed preserved.

Two real Pioneer runs PASSED with math ABI 27, state 1, started/completed/
stopped `1/1/1`, three host polls, zero numerical mismatches, phase `703a4`,
two fixtures, marker `4d415431`, both PCs `703bc`, reset
`47800 -> 87ffff -> 46800 -> 47800`, NoC deltas `2/34/2/2`, all five
GDDR/NIU/L1/TLB/PCI restoration flags 1, commands `0000/0000`, cold flag zero.
No firmware, host DMA, model, Assistant setting or boot change was made.
The input block is resident across the two fixtures only: this is not yet
a model allocator, persistent command queue, multicore backend or benchmark.

ASan/UBSan policy tests inject every individual setup/restore write loss,
NoC timeout, wrong flit count, wrong park PC, numerical corruption, source
change, active cores/interrupts, allocation failure and permanent GDDR/L1
restore failures. They assert no host doorbell, repeated start, reset or
payload rewrite after uncertain execution. All previous image and thirteen
PCI/ARC/core/memory policy suites remain passing. Jam builds the optional
driver and all twenty-two utilities cross-build with warnings as errors.

Installed driver SHA-256:
`e3cbe60248d8ec940ed7f00dfa6357e619a7028e9efc849fc4b256537668c8a5`;
CLI SHA-256:
`a375d177bd5b0a6c0769f951f243fff0b037466258f249c936a5dd0ae3323613`.
Verified original driver and native evidence are in
`/boot/home/pioneer-resident-matrix-20261015/`; copied logs and assembled
artifacts are ignored under
`generated.pioneer/tenstorrent/resident-matrix-20261015/`. Larger resident
matrix-vector kernels, model residency, Qwen graph integration and the
same-model faster-than-CPU acceptance gate remain unfinished.

### October 15: larger resident GEMV with on-card partial sums

The fixed resident program now has a separate `TT_UNPACK_GEMV` variant:
exactly 16 outputs and K chosen from `{1024,5120,6144}`. It uses one launch,
initializes Dst once, then iterates over K/16 blocks. Each iteration's source
is the fixed GDDR base plus `block*1088`; the device issues its read, waits
for completion, unpacks BF16 and adds four fidelity phases into the retained
FP32 accumulator. Only after the full dot product completes are results
copied to L1. No host block dispatch, intervening input writes or CPU partial
sums are used. No arbitrary code, pointers, coordinates or shapes are accepted.

The original matrix proof and GEMV share a templated policy selected solely
by trusted kernel program types. The original operation keeps math ABI 27;
the new root-only GEMV operation has math ABI 28 and an explicit version/shape
request. It maps/saves at most 417,792 fixed GDDR bytes, one code/data page,
the fixed output/status words and NIU controls. The same cold-recovery,
stopped-core ownership, NoC completion, numerical comparison and exact
GDDR/NIU/L1/TLB/PCI rollback gates apply. A failed/uncertain completion never
triggers a reset, retry or memory rewrite.

The simulator independently decodes the assembled RV32I instructions,
checks executable/data address bounds and models NoC payload delivery and
the BF16 FPU. Every output matches an independent full-length integer dot
product for all three shapes, including unused zero output rows. Fault
tests detect stale Dst, clearing between blocks, corrupted weights, delayed
or missing responses at the first, middle and final block. The byte test
independently assembles every supported shape and compares every code word.
Shared policy tests retain exhaustive small-operation write-loss coverage;
larger GEMV tests sample upload/restore interiors and boundaries and cover
NoC/PC/source/rollback failures and invalid shapes.

All three live Pioneer tests PASSED:

| Inputs -> outputs | Device reads / response flits | Host polls | Entire reversible test |
|---|---|---|---|
| 1024 -> 16 | 64 / 1088 | 11 | 0.341552 s |
| 5120 -> 16 | 320 / 5440 | 9 | 0.983847 s |
| 6144 -> 16 | 384 / 6528 | 9 | 1.146131 s |

Each had state 1, ABI 28, started/completed/stopped `1/1/1`, zero mismatches,
phase `703a0`, matching tile count, marker `4d415431`, both park PCs `703b8`,
all five restoration flags 1, commands `0000/0000` and cold flag zero. The
wall times include all initial ownership checks, CPU PIO uploads/readbacks,
numerical checks, original-memory restoration and mapping cleanup. They are
**not kernel throughput, real Qwen projections, whole-model inference,
nor a CPU speed comparison**. Inputs are known synthetic integer/BF16
fixtures. Persistent model allocations and efficient host transfers remain
necessary before practical inference timing can be measured.

The optional driver builds through Jam, all twenty-three utilities
cross-build with warnings as errors, and original image/memory/compute
tests remain passing. Final installed driver SHA-256:
`0efb637754cb925facd830678d125908c2e4b2815cbb4a08df17e7695e5ae176`;
GEMV utility SHA-256:
`189fb50a269e7113b55d7e2613b9ad19fea7bc7275031fcd00845e2ccfc3b407`.
Native backup/logs are under `/boot/home/pioneer-resident-gemv-20261015/`;
ignored local evidence is under
`generated.pioneer/tenstorrent/resident-gemv-20261015/`. Driver source and
program definitions were synced to the build mirror. Assistant, models,
boot, firmware and host DMA state were not changed.

### October 15: read-only host-transfer address-path audit

Before enabling any host DMA, a new root-only read-only preflight checks the
compute source route `(19,24)` to the actually connected PCIe tile `(2,0)`,
all previous idle/firmware/route gates, the Cadence inbound no-BAR-match
mapping, and the card's NoC-to-host SII window configuration. A temporary
read-only BAR0 TLB uses the full 43-bit local-address field to reach fixed
device-local `fffffffff0000000 + 134..150`, not a host pointer. The native
driver maps both SII and root-AT pages read-only/uncached. There are no
SII/inbound/initiator/payload writes, bus mastering, core starts or DMA.

The live audit PASSED: connected PCIe `18611002`; all three samples had
root inbound `0000002f/00000000` (the existing 48-bit identity mapping), and
SII windows `00400000 00400200 00400100 00400300 00000000 00000200
00000100 00000300`, matching pinned firmware 18.5 defaults. In particular,
window 0 uses memory requests with iATU bypass, no address prefix, relaxed
ordering, no-snoop or DBI flag. TLB/PCI restoration flags were `1/1`,
commands `0000/0000`, cold flag zero. This proves the sampled configuration,
**not an actual DMA transfer or cache-coherent host-buffer lifetime**.

ASan/UBSan tests verify independent full-width address decoding, route
refusal, every individual TLB-write loss/rollback, wrong/changing inbound
or SII values, active cores, source changes and failed restoration. Jam and
all twenty-four cross-built utilities pass. Installed driver SHA-256:
`0d0ebffb6accc34621d5054f83b6cb40b51ca3cb5d835215ecda7a4dc4385148`;
host-path utility SHA-256:
`806def48eea75766a3eece25bcb88b869be5628c143cd3c402b2a6c28971d6dc`.
Native backup/log: `/boot/home/pioneer-host-path-20261015/`; ignored local
evidence: `generated.pioneer/tenstorrent/host-path-20261015/`. No firmware,
model, Assistant, boot or DMA state was changed. Next: dedicated owned,
pinned non-cached host buffers and a bounded device-read-only transfer.

Register provenance: pinned official system-firmware 18.5 `pcie.c` (SII
base, window data fields and default values), and
[PCI Express tile documentation](https://github.com/tenstorrent/tt-isa-documentation/blob/8a34e25e04d1ad7dd129ac9231e126b85923fac3/BlackholeA0/PCIExpressTile/README.md).
This policy is original code; no GPL KMD implementation was copied.

## Findings from source

- P100A belongs to Blackhole. KMD `enumerate.h` identifies Blackhole as
  1e52:b140; this ID alone does not identify the precise board SKU.
- KMD `blackhole.c` maps portions of BAR0 and BAR2. BAR4 length determines
  the exposed count of 4-GiB TLB windows. Do not assume a particular BAR size
  or that a 32-bit `pci_info` field can describe the complete aperture.
- KMD `memory.c` uses coherent DMA allocation, pinned user pages, scatter/
  gather mappings and IOMMU handling. Blackhole's current DMA limit is 58 bits;
  that is not proof the Pioneer host can DMA to every physical RAM address.
- KMD `interrupt.c` requests one PCI interrupt with Linux IRQ allocation.
  Haiku has MSI/MSI-X interfaces, but the P100A path must be verified.
- UMD `tt-kmd-lib/src/tt_kmd_lib.c` uses Linux ioctls plus file-backed mmap
  offsets for device/TLB/DMA memory. Implement a Haiku transport; do not assume
  a Linux ioctl number or mmap offset works with Haiku's driver ABI.
- UMD `device/api/umd/device/driver_atomics.hpp` already includes RISC-V
  fences. This is a head start, not evidence that the whole runtime supports
  RISC-V Haiku. Audit discovery, mapping, dependencies and cache behavior.
- UMD's README describes IOMMU/hugepage requirements for host-device memory.
  An initial bounded DMA allocation is more realistic than promising general
  Linux hugepage/pinning compatibility on Haiku.

## Existing Haiku foundations and risks

`src/add-ons/kernel/busses/pci/ecam/ECAMPCIControllerFDT.cpp` already has
SG2042 PCIe translation and MSI support. Preserve this working infrastructure;
change it only when a captured P100A failure demonstrates a missing capability.

The current NVMe driver explicitly uses non-cacheable DMA bounce buffers on
SG2042 because ordinary cached payload memory is not coherent in this port.
This is a central accelerator risk: CPU fences alone do not flush/invalidate
non-coherent caches. Initially use small, explicitly managed DMA buffers with
validated bus addresses, ownership transitions and lifetime rules.

BAR addresses are PCI bus addresses, not automatically CPU physical addresses.
Validate translated ranges, 64-bit BAR pairing, full widths, mapping attributes
and bridge apertures before touching registers. Avoid destructive BAR sizing
probes against a live device. PCI enumeration is not accelerator readiness.

## Incremental implementation plan

1. **Inventory:** install with the machine fully powered down; capture Linux
   PCI identity/BDF, assigned BAR resources, link state and firmware/driver
   versions. Confirm Haiku enumerates the same endpoint. Do not flash firmware.
2. **Native diagnostic driver:** bind only the Blackhole device, expose a
   versioned read-only identity/resource ioctl, and fail closed on unsupported
   operations. No DMA, resets, arbitrary physical mapping or user register
   writes. Build this as an optional add-on, not a mandatory boot dependency.
3. **Controlled device access:** validate BAR mappings, port the minimum ARC
   firmware/message and TLB access required for a bounded identity/telemetry
   query. Register reads are not all side-effect-free; audit each access.
4. **DMA/interrupt smoke tests:** a small buffer round-trip with known patterns,
   guards, timeout, cleanup and repeated tests; then high RAM, multiple CPUs,
   cancellation and process-exit cases. Enable bus mastering only after DMA
   mappings and teardown are correct. No arbitrary user-page DMA initially.
5. **UMD transport:** substitute Haiku discovery/ioctls/areas and implement only
   verified operations. Unsupported Linux-only features return errors, not
   dummy success. Validate runtime/firmware compatibility before compute.
6. **Compute:** minimal supported runtime operation, then matrix multiply,
   then an LLM integration and performance tests. A PCI/kernel driver by itself
   does not make the existing CPU llama.cpp build use the card; it needs a
   compatible accelerator backend/runtime path too.

Hardware-dependent steps cannot be marked complete from compilation alone.
Preserve the working SD/Samsung CPU-only configuration as a recovery path.

## Fedora hardware inventory, 2026-09-21

With the P100A in the bottom PCIe slot and a native 12+4-pin PSU cable, the
first boot did not enumerate the card. After a clean shutdown and physical
reseat, Fedora 38 (Linux 6.1.31, riscv64) enumerated `0002:81:00.0` as
`1e52:b140`, class `1200` (processing accelerator). The root port is
`0002:80:00.0`. The link trained at PCIe 2.5 GT/s x8; this is below the
endpoint's advertised 32 GT/s x16 capability and is not a performance result.

No Tenstorrent module, device node, or `tt-smi` installation was present.
Linux failed to assign all three memory BARs: BAR0 512 MiB, BAR2 1 MiB,
and BAR4 32 GiB. It also failed to allocate the root-port's requested
48 GiB prefetchable bridge window. The root bus advertises a high
prefetchable window at `0x4900000000..0x4affffffff` (8 GiB), plus a
768 MiB lower prefetchable window. Its non-prefetchable windows cannot
accommodate the 32 GiB prefetchable BAR. `lspci` shows the bridge memory
windows disabled and the endpoint resources unassigned. The other Pioneer
root buses likewise advertise only 8 GiB high prefetchable windows in this
Fedora boot; moving slots alone is not an established fix.

The card's usable memory capacity is distinct from the size of
its BAR4 PCI address aperture. Do not treat the user's 28 GB memory figure
as a contradiction of the measured 32 GiB BAR. The captured allocation
failure is sufficient to stop driver/LLM installation attempts for now.
The SG2042 manual and Tenstorrent BAR4 discussion below narrow the possible
workaround; a larger device-tree window is not the solution.
Do not write live PCI configuration, flash firmware, or replace the working
boot setup as a diagnostic shortcut. A future aperture change needs a
recovery plan and separate validation. No Linux/Haiku PCI configuration,
driver, firmware, or storage was changed during this inventory.

## Linux boot check, 2026-10-04

With the Haiku SD removed, the Pioneer booted its SPI loader, then kexec'd
Fedora Linux 6.1.31 from NVMe. The serial log again shows the P100A at
`0002:81:00.0` (`1e52:b140`) requesting BAR0 512 MiB, BAR2 1 MiB and BAR4
32 GiB. The root port's high prefetchable aperture remains 8 GiB; Linux reports
no space for the 48 GiB bridge window and fails to assign all three endpoint
BARs. This confirms the same aperture problem on a fresh boot with the card
installed. Serial capture is saved in ignored
`generated.pioneer/tenstorrent/pioneer-linux-boot-2026-10-04.log`. The boot
used no SD, and no PCI settings, card firmware, or disks were modified.

### Read-only aperture feasibility check

The active device tree's `pcie@7062000000` ranges match the kernel source in
`arch/riscv/boot/dts/sophgo/mango-pcie-4rc.dtsi`: 768 MiB low prefetchable,
256 MiB non-prefetchable, 8 GiB high prefetchable at `0x4900000000`, and
4 GiB high non-prefetchable at `0x4b00000000`. `/proc/iomem` confirms the
adjacent `0x4c00000000..0x4fffffffff` address space belongs to the next
PCIe controller, while `0x4800000000..0x48ffffffff` contains this root's
configuration and smaller windows. Thus the 32-GiB-aligned interval
`0x4800000000..0x4fffffffff` cannot simply be assigned to BAR4.

The Sophgo Cadence host driver programs an outbound translation region for
each device-tree range. Its CPU-address fixup masks addresses with
`0xCFFFFFFFFF`; for example, the next 32-GiB-aligned candidate at
`0x5000000000` would become `0x4000000000` in the programmed translation
register. The controller's wider address decode and a non-overlapping host
aperture have **not** been established, so changing only the device-tree
window would be an unsafe experiment. This read-only check made no changes
to the Fedora boot or PCI configuration.

The local Tenstorrent KMD source derives its number of BAR4 TLB windows from
the exposed BAR length, but the UMD and compute stack still need validation
without BAR4. Even after BAR0/BAR2 allocation works, host-side TT-Metal
support on riscv64 remains a separate porting question.

### SG2042 physical limit and BAR4-free path

The [SG2042 Technical Reference Manual, Chapter 2, Table 1](https://github.com/milkv-pioneer/pioneer-files/blob/main/hardware/SG2042-TRM.pdf)
assigns only **16 GiB** of CPU physical address space to each PCIe link:
`0x4000000000..0x43ffffffff`, `0x4400000000..0x47ffffffff`,
`0x4800000000..0x4bffffffff`, and `0x4c00000000..0x4fffffffff`.
The P100A is on PCIe1 link 0 (`0x4800000000..0x4bffffffff`). Its 32 GiB
BAR4 cannot fit inside that link's documented physical aperture, regardless
of how the current 8 GiB device-tree subwindow is enlarged. Do **not** make
a 32 GiB device-tree `ranges` change or borrow space from the neighboring
link. The live `lspci -vv` capability list also shows no PCI Resizable BAR
capability; a standard BAR resize is not available in this observation.

There is, however, a credible BAR4-free path. A [Tenstorrent engineer's
August 2026 Linux PCI patch](https://lists.openwall.net/linux-kernel/2026/08/24/1853)
targets Blackhole device `1e52:b140`, exactly this P100A's PCI ID. It
proposes omitting BAR4 from Linux resource assignment when the host aperture
is too small, allowing BAR0 and BAR2 to fit. The author reports that this
made `tt-smi` and `tt-bh-linux` work on a 4 GiB-aperture RISC-V host.
That is evidence of basic card access without BAR4, **not** evidence that
TT-Metal or LLM inference works without it.

The patch is not safe to copy uncritically: [Linux PCI review](https://lkml.iu.edu/2608.3/06163.html)
points out that zeroing Linux's BAR4 resource metadata does not disable
the BAR in the device. Enabling memory decoding could leave a live,
unassigned BAR that decodes an unintended PCI address. Do not use this
unmodified PCI quirk on the Pioneer. The firmware-side sizing mechanism
below is preferable once the card can safely be reached for flashing.

On the live Fedora boot, read-only `setpci` shows PCI COMMAND `0x0000`
(memory decoding and bus mastering both off), while BAR4's low/high config
dwords are `0x0000000c`/`0x00000000` (an unassigned 64-bit BAR). This is
safe in the current state, but it reinforces the reviewer's concern:
turning on memory decoding after merely hiding BAR4 from Linux would leave
the hardware BAR pointed at bus address zero. No PCI config register was
written during this check.

### Firmware-side BAR4 resize: offline dry run

Tenstorrent's public [system-firmware source](https://github.com/tenstorrent/tt-system-firmware)
at `04f0df6ebfbd016d98189e8131b16af17b48638b` exposes
`pci0_property_table.pcie_bar4_size` in MiB. The P100A table defaults to
`32768`; `lib/tenstorrent/bh_arc/pcie.c` accepts power-of-two values and
explicitly handles `0` as “BAR4 Disabled.” The project's own
`scripts/update_bar4_size.py` edits that field in a firmware bundle and
states that a **cold reboot** is required for the change to take effect.
The `P100A-1` board in the bundle matches this card's `0x0043` subsystem
device family, but its exact installed firmware version remains unknown.

An offline dry run used the official `v19.15.0` firmware bundle, whose
downloaded SHA-256 matched the published
`c1a317f9658435a7f2e8ab1e18a9fe942cd36334d8b88f01b777c8de975b2aef`.
The official script changed only `P100A-1` PCI bus 0 to BAR4 size `4096`
MiB, and its post-write verification passed. `tt_fwbundle.py diff` reported
only the P100A-1 `cmfwcfg` entry changed, from CRC `8cc1b6a1` to
`34a5fa6f`. The resulting **unflashed, experimental** bundle is at
`generated.pioneer/tenstorrent/fw_pack-19.15.0-p100a-bar4-4096-test.fwbundle`
(ignored local artifact, not pushed to GitHub) with SHA-256
`6dda5a74404672a194bf472a2c816e75b043dda015307d6b6774c6e4df61d602`.
Do not copy this firmware bundle to the public project repository or flash
it without determining the card's installed firmware version, taking a
recoverable backup, verifying board compatibility, and establishing a way
to access the card with BAR0/BAR2 despite today's failed allocation.

Why 4 GiB: one BAR4 4-GiB TLB window plus BAR0 (512 MiB) and BAR2 (1 MiB)
should, in principle, fit the root's 8 GiB prefetchable window. Linux bridge
allocation and Tenstorrent userspace behavior with only one large window
have **not** been tested. Setting BAR4 to zero is another documented firmware
option for basic access, but its effect on TT-Metal/LLM use is even less
certain. No card firmware, kernel, boot configuration, or PCI register was
changed during this work.

Next gate: find a safe one-time access path (for example a compatible host)
and a firmware backup/recovery procedure, then select a matching firmware
release and test a 4 GiB BAR4 configuration under supervision. Do not
reboot or flash this Pioneer unattended: a failed card flash or boot test
could need physical recovery. First verify PCI resource assignment and
`tt-smi`; model inference is a later milestone.

## Official references

- P100A hardware: https://docs.tenstorrent.com/aibs/blackhole/installation.html
- Kernel driver: https://github.com/tenstorrent/tt-kmd
- User driver and host-memory requirements: https://github.com/tenstorrent/tt-umd
- Compute runtime: https://github.com/tenstorrent/tt-metal
