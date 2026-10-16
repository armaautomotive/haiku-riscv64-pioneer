# Haiku Assistant

Native local Qwen chat, backed by a separate resident llama.cpp service.
Hardware now also offers **P100A — Qwen3 0.6B (experimental, slow)**, using
the exact, two-core resident Q8_0 runtime. It is currently slower than CPU,
not a practical acceleration claim. The older TinyStories experiment remains
available separately.
No kernel changes or network listener. A native tools bridge is under testing;
desktop mutations require one-time native approval (see [TOOLS.md](TOOLS.md)).

## Components

- `HaikuAssistant`: conversation/input window. Send or Ctrl+Enter submits.
  Stop cancels the current request. New chat clears conversation context.
- **Models…** opens the native model manager: choose an installed model or
  install another from the curated Hugging Face catalog, with progress and
  cancellation. Model files stay in `~/models` and are shared across chats.
- **Hardware…** opens the native hardware chooser, including availability,
  explanations and CPU-thread controls. `HaikuAssistantHardware` opens the
  same dialog independently without closing an existing conversation window.
- `HaikuAssistantService`: background BApplication. Loads Qwen once and reuses
  its model and context across requests. All llama calls run on a dedicated
  worker. Haiku messages carry bounded requests and streaming responses.
- `HaikuAssistantShortcut`: input-server filter for Ctrl+Space. An independent
  looper handles launch/toggle; the input thread only queues a message.
  Repeated key-downs and the matching key-up are consumed for the shortcut.
- `assistantctl`: service status, CPU-thread settings, stop, toggle, and
  request/cancellation tests.

Ctrl+Space opens the GUI if absent, brings it forward if inactive, or hides it
if active. Hiding preserves the conversation. Closing quits the GUI and cancels
its current request, but leaves the model service loaded. Closing loses the
memory-only conversation. Ordinary Space is unchanged.

The GUI starts the service on first launch, so loading occurs before the first
request where possible. This is **not yet an automatic OS-boot service**. A
reboot or `assistantctl stop` releases the model. The next GUI launch/Send can
restart it. A failed load can be retried from Models without restarting.

## Hardware selection

Open **Hardware…** in Assistant, launch `HaikuAssistant --hardware`, or run
`/boot/home/config/non-packaged/apps/HaikuAssistantHardware` separately.
The dialog shows the selected and actually running backend, logical CPU
count, and an availability list supplied by the resident service.

- **CPU (llama.cpp)** is available for the currently supported Qwen3 models.
  Choose it, set a CPU-thread count, and click **Use selected hardware**.
  The choice and thread count are saved atomically alongside the selected
  model. CPU-to-CPU settings changes do not unload/reload the resident model.
- **Tenstorrent P100A** is available for experimental TinyStories completion
  when its two-file model bundle is installed and driver ABI 39/40 is present.
  Choosing it also selects TinyStories. Choosing CPU restores the previous
  Qwen model. Both model/backend fields are saved only after successful load.
  This is execution capability, not PCI card detection or Qwen compatibility.
- **P100A — Qwen3 0.6B (experimental, slow)** selects the already installed,
  hash-verified Qwen3-0.6B Q8_0 model. It does not support 1.7B, 4B or 27B.
  First use uploads roughly 639 MiB of packed weights (about 15 minutes).
  Later questions keep all 197 matrices resident. Expect about 0.15 output
  tokens/second after upload, plus prompt evaluation and CPU validation.
  Questions are single-turn: 512 UTF-8 bytes / 192 prompt tokens, eight output
  tokens in the GUI (service limit 1–16), 256-token context. No history or
  system tools are used in this experimental mode. CPU mode restores the
  previous CPU model/thread setting.

Changing hardware is rejected during model loading or an active request.
Unknown IDs, unsupported backends, malformed settings and invalid thread
counts are rejected by the service too, not just the UI. Requests from the
updated GUI/tool-session bridge pin both model path and backend ID. Asking
for P100A will never silently run the request on CPU. An unavailable or corrupt
saved backend stops startup model loading; explicitly choosing CPU in the
dialog can recover it. Missing backend settings from older installs default
to CPU. There is no Auto mode or automatic CPU fallback.

Hardware selection does not execute card work, load firmware, change power or
download models. Capability detection reads driver ABI metadata only. Actual
TinyStories generation uses the existing guarded data-tile transactions, which
temporarily map BAR0/BAR2, execute/check/stop the tile, and restore L1/TLB/PCI
state. BAR4 stays unforwarded and bus mastering/DMA remain disabled. A failed
card transaction disables the backend for that service session; a driver cold
recovery latch is never bypassed, even after restarting the service.

P100A Qwen instead uses the separately verified owned resident session and
two fixed workers. Every learned matrix/classifier product must run on the
card: each evaluated position requires 197 retained tensors, 163,692 verified
upload pages and 234 real matvec calls. CPU handles attention/KV/norms,
activation preprocessing and an independent 32-thread full-logit reference.
Cosine >= .999, relative L2 <= .02 and CPU-identical greedy output tokens are
required; failed output is never substituted. Card failures disable both
card choices for the service session, with no automatic CPU fallback.
The existing driver owns mappings/high host memory, checks both completion/
stop proofs, and retains quarantine/cold-recovery gates. This update does not
change the kernel, firmware, boot media or runtime libraries.

Uploads and matvecs report progress and honor Stop only between completed
transactions; no thread is killed during an ioctl. Cancelling clears the
partial card context safely and can require a fresh upload next time. Closing
the service or explicitly switching hardware releases its resident session.
CPU contexts remain CPU-only even after accelerator registration: they use
an inert backend with no supported operations and no card access. It accepts
only its default host-storage buffer to satisfy ggml's scheduler contract.
An additional service guard refuses CPU inference if an executable card
context unexpectedly exists.

Diagnostics: `assistantctl hardware` lists capabilities; `assistantctl hardware
cpu [THREADS]` selects CPU, while `assistantctl hardware p100a` queues the
TinyStories/P100A pair if available. `assistantctl status` reports
both selected and active backend IDs. The native `HaikuAssistantHardwareTest`
checks in-memory settings/policy without writes; `--loading-test` checks the
busy gate during an actual model load, and `--service-tests` validates the
resident service, writes only the current CPU/thread selection, and tests
that a P100A-tagged request is rejected without inference.
Service-dependent test modes return skip code 77 if their ready/loading
preconditions are no longer true; they do not stop the service to satisfy them.
Use `assistantctl hardware p100a-qwen` for the new Qwen/card pair;
`HaikuAssistantHardwareTest --p100a-qwen-tests` checks rejection of unpinned
hardware/model, wrong request format, output bounds and question limits
without executing inference.

### Qwen/P100A integration checks on Pioneer (2026-10-16)

The full standalone cross-build passes with warnings as errors against the
pinned native llama runtime. Native hardware-policy/settings, loading-lock,
service capability/selection and model-store tests pass. Ten deliberately
invalid Qwen/P100A requests are rejected without inference. After registering
the accelerator and selecting Qwen/P100A, explicitly switching back to CPU
restores Qwen3-1.7B and 32 threads; the public prompt `Reply with the single
word READY.` returns `READY`, success true. A one-second public cancellation
test succeeds before card upload begins and retains the selected model.

An initial inactive-backend implementation rejected its own default host
buffer, triggering ggml's scheduler assertion during CPU switch-back, before
any card execution. The corrected backend accepts that storage but rejects
**every operation**. Native CPU switch-back/inference now passes; a new
ASan/UBSan scheduler-reservation/assignment regression verifies that the
inactive backend is never chosen and opens no device. The backend suite also
injects cancellation before/after completed uploads and matvecs, sticky device
failure and exclusive-owner cleanup. Separate resident session, stream,
tensor-cache and benchmark-controller software tests pass. These offline
fixtures are not native performance evidence.

The final service's first full public request, `What is the capital of
France?`, exits zero and returns `The capital of France is **Paris**`:
eight independently checked output tokens in 1,075.5 seconds including cold
upload. Each evaluated position passes the full-logit, greedy-output and
exact device-coverage gates above. All 197 matrices remain resident afterward;
the service stays ready with no model error. This is a correctness/integration
result, not a CPU speedup or support for a larger model.

The approved ordinary GUI is reopened with the new Hardware dialog. Its
actual Send action returns the same eight-token public answer using the
warm weights, with no further upload or model reload. Selected/running both
remain `p100a-qwen`, all three hardware choices are listed, and the correct
experimental description is shown. Applying the already selected pair keeps
the warm model; the rejection tests still pass afterward. The GUI is left
open with this public reply and the card model ready. This warm test is not
a generation-throughput or CPU-speed comparison.

Original application/service/dialog/control/settings backups and this
integration's public test logs are retained privately on the Pioneer under
`/boot/home/pioneer-assistant-qwen-20261016`. No existing conversation text was
read. Runtime libraries, model data, kernel, firmware and boot configuration
are unchanged; the pre-inference card audit passes with cold-recovery zero.

## Model installation and selection

The deliberately small catalog contains Qwen3 0.6B Q8_0, 1.7B Q8_0,
and 4B Q4_K_M from the official Qwen repositories, plus MIT-licensed
TinyStories-260K and its tokenizer from `karpathy/tinyllamas`. The 0.6B model is the
established on-device chat baseline; 1.7B installation/selection/chat was also
tested on Pioneer. The 4B entry is an architecture-compatible CPU candidate,
not yet inference-tested here. Compatibility is not a speed or output-quality
guarantee. Each catalog entry
pins an immutable repository revision, exact byte length, and publisher LFS
SHA-256. Sources and licenses are shown in the window; no Hugging Face account
or token is needed. Arbitrary repositories, gated models, automatic remote
catalog updates, and local-file imports are not part of this first version.

Click **Install from Hugging Face**, then **Use for new chats**. Nothing
downloads automatically. Downloads require twice the model size plus 64 MiB
(private staging plus an exclusively created destination on BFS),
run off the UI thread, and use temporary files. The installed `/boot/system/bin/wget`
must support HTTPS and `--no-config`. Every redirect is checked for HTTPS,
TLS verification remains enabled, and the completed file must match both
size and SHA-256. No shell commands, downloaded scripts or remote chat
templates are executed. Existing files are never overwritten. Cancellation
removes only this operation's temporary/incomplete files. BFS has no hard links
or atomic no-replace rename: publication copies verified staging data into an
`O_EXCL`-reserved file, synced before reporting success. A partial destination
does not count as installed, and activation always verifies the full file.
A crash/power loss can leave an unselected `.part-*` or incomplete destination;
automatic resume/cleanup is not implemented yet.
An existing same-name file with the wrong size/hash must be inspected/moved
manually rather than silently replaced. A model install does not activate it.

The Pioneer currently resets its clock to 1970 on boot. Set Date & Time /
network time before downloading; the installer refuses a clearly invalid
clock rather than disabling certificate checks. DNS and internet access must
also be configured on Haiku. Downloads can work independently of chat, but
selection requires the updated resident service to be running.

Selection is asynchronous and rejected while a request or load is in progress.
The service verifies the complete file again before loading. It retains the
previous model until the replacement model/context and settings save succeed;
a failed switch leaves the old model usable. The selected path and CPU-thread
setting are saved together without discarding either. Successful model changes
start a fresh GUI conversation, and multi-step tool sessions pin their model
path to prevent silently switching mid-request.

The window explicitly lists **Qwen3.8 27B as unsupported** and disables install
and activation. Models' normal **Use on CPU** action remains CPU-only;
choose **Hardware… → P100A — Qwen3 0.6B** for the experimental card runtime.
The pinned llama runtime still lacks Qwen3.8 architecture support. Correct
standalone 0.6B generation is verified, but practical CPU-beating performance
is not. The detailed qualification history is in
[the port log](../../../tools/pioneer/TENSTORRENT_PORT.md).

### P100A experimental story mode

Install catalog entry 4 in Models, then click **Use on P100A (story mode)**,
or choose **P100A + TinyStories** in Hardware. The installer downloads the
1,056,540-byte model and 6,227-byte tokenizer from one immutable Hub revision;
both hashes are required before activation. An interrupted bundle install
can reuse its completed, hash-verified half, but never overwrites a bad file.

Enter a short story beginning such as `Once`. The native conversation window
sends this raw text directly to the P100A executor, without ChatML, previous
turns, system instructions, tool execution, or CPU fallback. Initial limits
are 128 printable ASCII characters, 16 prompt tokens, and 8 output tokens
(the service supports explicitly requested limits of 1–16 output tokens).
This tiny base model is for integration testing, not useful assistant chat.

Every learned linear projection and the classifier runs on the P100A via
BF16 tiles. CPU handles embedding lookup, attention/KV, norms, activations
and partial sums, plus an independent FP32 numerical check. Generated tokens
come from the card's logits, never the reference's logits. Weights are host
streamed, not resident in GDDR. Expect roughly 15 seconds per evaluated
prompt/output token, including verification. Progress streams while a token
is being evaluated. Stop waits for the current checked transaction to finish;
the worker is never killed during an ioctl. Each request gets fresh KV state.

Offline adapter tests, including independent tokenizer comparison, simulated
faults and batch-boundary cancellation, run with:

```sh
sh tools/pioneer/tests/run_pioneer_tinyllama_tests.sh PINNED_BOOTSTRAP_DIRECTORY
```

Those explicitly use a software BF16 adapter and do not count as card tests.

For diagnostics: `assistantctl models`, `assistantctl install INDEX`,
`assistantctl verify INDEX`, and `assistantctl model INDEX` use the same catalog
and installation/selection code. Indices are zero-based. Selection acceptance
does not mean loading succeeded: check `assistantctl status` for `loading=0`,
`ready=1`, `model`, and `model_error`.

The service serializes requests and rejects a second concurrent request. Each
request starts with cleared KV memory and a fresh sampler, so clients do not
inherit another conversation. Weights and allocated context remain resident.
Prompt processing is still required each turn; this is not prefix-cache reuse.

## Current runtime assumptions

Pinned llama revision: `c060ca974c773c7c3d17fd1b66dc9d312bc292c0`.
Model: Qwen3-0.6B-Q8_0, with its explicit ChatML/non-thinking template.
CPU threads default to the lesser of 32 or the available CPU count. Change the
saved value in the GUI or with `assistantctl threads N`; it applies to the next
request without reloading the model. CPU-mode context is 4,096 tokens,
batches are 64, and the maximum response is 256 tokens. The experimental
card mode has the separate, smaller limits documented above.
The Apply button saves `~/config/settings/HaikuAssistant_settings`. Both prompt
processing and generation use this count; an in-flight request keeps its old
count. This controls worker threads, not CPU affinity or every service helper
thread. Valid range is 1 through the available CPU count. SD and Samsung
installations keep separate settings on their respective boot volumes.
The GUI trims older completed turns above 3,000 bytes and limits input to 2,000
bytes. The service validates actual token count and rejects oversized context.
Cancelled/failed replies are not added to subsequent GUI context.

Model path uses an explicit `LLAMA_MODEL` startup override when set, otherwise
the saved selection, otherwise `/boot/home/models/Qwen3-0.6B-Q8_0.gguf`, then
`/Haiku1/home/models/Qwen3-0.6B-Q8_0.gguf` as an SD-session fallback.
Only allowlisted catalog paths with matching hashes can be activated. The
launcher no longer exports a default `LLAMA_MODEL` that would mask a saved
selection. Changing selection in Models takes effect in the current process;
an explicitly exported override still takes precedence at the next startup.
The GUI launches the service with libraries from the existing native runtime
under `/boot/home/develop/llama-c060ca974c77/build-pioneer/bin`, or the
equivalent `/Haiku1/home/develop/...` location. This layout is temporary until
the runtime is packaged. An SD boot needs Samsung mounted at `/Haiku1` first.

## Build and installation

```sh
ASSISTANT_LLAMA_LIB_DIR=/private/tmp/pioneer-assistant-libs \
  sh tools/pioneer/build_assistant.sh /private/tmp/HaikuAssistant
```

The library directory must contain the matching **Haiku RISC-V** libraries:
`libllama.so.0.2.0`, `libggml.so.0`, `libggml-base.so.0`, `libggml-cpu.so.0`.
The script emits the GUI, `HaikuAssistantService`, `HaikuAssistantShortcut`,
`HaikuAssistantControl`, `HaikuAssistantHardware`, `HaikuAssistantHardwareTest`,
and `HaikuAssistantModelStoreTest`, plus tools-bridge binaries/tests. Without
the library variable it skips the service.
It also emits a separately signed `HaikuAssistantP100A` preview build for
isolated native UI tests; the ordinary Assistant includes the same story mode.
It attaches application resources and does not deploy or write a disk image.

Install, preserving an old version before replacing it:

| Artifact | Path below `/boot/home/config/non-packaged/` |
|---|---|
| HaikuAssistant | `apps/HaikuAssistant` |
| HaikuAssistantService | `servers/HaikuAssistantService` |
| HaikuAssistantShortcut | `add-ons/input_server/filters/HaikuAssistantShortcut` |
| HaikuAssistantControl | `bin/assistantctl` |
| HaikuAssistantHardware | `apps/HaikuAssistantHardware` |

All executables must have executable permissions. Input-server add-on
directories are monitored; no input-server restart is needed. To disable the
shortcut, move only this add-on outside the filters directory. Existing
Shortcuts preferences are not modified; Ctrl+Space may conflict with another
user-assigned shortcut or input method.

GUI/control/filter Jam targets exist. Service compilation currently uses the
standalone script because the llama runtime has not yet been integrated into
the system package. Neither runtime nor assistant is automatically included
in newly rebuilt OS images yet.

Run the native `HaikuAssistantModelStoreTest` on Haiku for size/hash, path,
symlink, cancellation, existing-file, and catalog-boundary regression checks.
It creates private temporary fixtures, and verifies the existing 0.6B model
when present. It does not download files or alter installed models/settings.
The optional `--cancel-download` test starts an HTTPS install of catalog entry
2 on a regular Haiku worker and cancels it after one second; it requires that
entry not already be installed. Assertions stay enabled in release test builds.

## P100A integration verification on Pioneer, 2026-10-15

- Cross-builds passed with warnings as errors, as did the standard GUI,
  control, hardware and model-store Jam targets. Relevant sources were synced
  to the build mirror. No llama submodule changes were included.
- The two-file TinyStories bundle was installed over verified HTTPS on Haiku
  and both SHA-256 pins passed. A tokenizer URL initially returned 404; its
  corrected `stories260K/tok512.bin` path then reused the verified model half
  without overwriting it. The saved P100A/model pair survived service restart.
- ASan/UBSan offline tests passed against the independent pinned C reference,
  including BPE prompts, input limits, missing-file rejection, safe
  cancellation, simulated batch failure and no retries after a fault. The
  software-only adapter also passed natively on Haiku.
- Native service tests rejected wrong backend/model, missing story format,
  invalid token limits/types and unsupported text without inference/fallback.
  Real-card cancellation returned safely and left the same resident service
  ready. A hardware switch during generation was rejected as busy.
- An initial UI run stopped during a DRAM-route preflight before the next
  compute batch started. Its C++ exception error path entered the debugger.
  This was **not** a successful generation. Error/cancellation paths were
  changed to explicit returns, not exception unwinding. The driver cold gate
  remained clear and the read-only PCI snapshot matched the original disabled
  state before another controlled test. No guard was relaxed or bypassed.
- The repaired native GUI completed eight output tokens for `Once`:
  ` upon a time, there was a little`. Nine evaluated positions used 9,252
  card tiles, with numerical/reference and completion/stop/restore checks
  on every batch. CPU handles attention/KV/norms and verification as described
  above; this is not Qwen inference or a performance-ready accelerator runtime.
- Independent post-run telemetry passed: ASIC about 49 C, reported GDDR
  uncorrected errors zero, firmware 18.5 unchanged, PCI/TLB restored, endpoint
  command `0000`, original BAR words. No reboot or SD change was required.
- Native UI inspection confirmed clear P100A/hybrid labeling, disabled CPU
  thread controls in story mode, and a completed story in the actual main
  Assistant window. No pre-existing chat contents were read; the user saved
  their chat and explicitly approved reopening the older GUI.
- Switching back to CPU restored Qwen3-1.7B and 32 threads; a `READY` request
  and CPU/no-fallback service regressions passed. Selecting TinyStories in
  the actual Models window switched back to P100A. The native Send/Stop path
  also cancelled safely. The 27B entry remained disabled; TinyStories showed
  its MIT license, two-file requirement and explicit story-mode Use button.
- A second full service request, starting from BOS, returned `Once upon a
  time, there was a`: eight output tokens, 8,224 real card tiles, 124.17 seconds
  total, `success=1`. That is about 0.064 output tokens/second **including CPU
  verification**, not a general P100A performance result. The same service
  remained ready/idle with P100A selected; no automatic fallback was used.
- The original four installed binaries have `.before-p100a-20261015`
  backups; settings and test artifacts are under
  `/boot/home/pioneer-assistant-p100a-20261015/`. Kernel, firmware, boot media,
  runtime libraries and existing Qwen files were unchanged. An intermittent
  preflight refusal was observed; this remains an experimental integration.

## Earlier hardware-dialog verification on Pioneer, 2026-10-15

- Standalone cross-builds passed with warnings as errors; standard Jam builds
  passed for Assistant, control utility, hardware app and hardware tests.
- Native policy tests passed for backend IDs, fail-closed unsupported/corrupt
  saved IDs, CPU-thread bounds, and preservation of unrelated/model settings.
- The service rejected hardware changes during loading. Ready/idle tests
  passed for unsupported/unknown backend selection, invalid or malformed
  thread counts, and P100A-tagged or wrongly typed backend requests. No
  fallback inference was performed. CPU selection preserved the resident
  model (`model_loads=1`), its path and 32 threads; saving threads preserved
  the explicit saved CPU backend field.
- Native UI tests confirmed P100A disables Apply and CPU-thread controls,
  while CPU enables them. Applying CPU from the actual dialog kept the model
  resident. A window-only screenshot confirmed the full scrollable P100A
  explanation. The service generated `READY` on Qwen3-1.7B after the changes.
- The hardware dialog was installed and opened independently; the existing
  conversation window was left open. The main app was replaced atomically,
  so its Hardware button appears on its next launch. No chat contents were
  inspected. The updated GUI ignores transient empty model paths during
  service loading/restart rather than treating them as model changes.
- Previous GUI/service/control binaries have `.before-hardware-20261015`
  backups; settings were backed up under
  `/boot/home/pioneer-assistant-hardware-20261015/`. The same Qwen3-1.7B model
  and 32 CPU threads remain selected. No PCI, firmware, power, bootloader,
  kernel, model files or runtime libraries were modified.

## Model-manager verification on Pioneer, 2026-10-15

- Standalone RISC-V cross-build passed with `-Wall -Wextra -Werror`; the GUI,
  control utility and regression tests also passed their standard Jam builds.
  Assistant sources and its app-subdirectory entry were synced to the mirror.
- Native tests passed: catalog/path bounds, size and SHA-256 rejection,
  symlink rejection, worker-stack verification, exclusive BFS publication,
  existing-file protection, and incomplete-publication cleanup on cancellation.
  Live HTTPS worker cancellation passed in standalone and Jam-built tests.
- Existing tool parser tests remained at zero failures. Installed service
  answered a short chat request and cancelled generation without unloading.
- Native UI scripting confirmed four catalog entries; the unsupported 27B
  entry disabled both actions, while the installed 0.6B entry enabled Use.
  A window-only screenshot was inspected for layout.
- Idle selection was accepted asynchronously, a second selection while
  loading was rejected, and unsupported selection was rejected. The saved
  model path and original 32-thread setting survived service restart.
- A full 1,834,426,016-byte Qwen3-1.7B Q8_0 HTTPS download passed its pinned
  SHA-256 and BFS publication. The native Models window then selected it;
  the same service reported `ready=1 loading=0 model_loads=2` and generated
  `READY` successfully. It remains the selected model. A read-only settings
  test confirmed its saved path plus 32 threads both before and after saving
  the thread setting again. The window-only final layout was inspected.
- GUI, service, `assistantctl`, and launcher were installed under the existing
  non-packaged paths, with `.before-model-manager-20261015` backups of all four.
  No kernel, bootloader, PCI driver, runtime library, or existing model changed.
- The clock was corrected from 1970 for TLS. Unresponsive DHCP-provided DNS
  was fixed by putting the existing gateway (`10.0.0.1`) first and retaining
  both ISP servers. The original resolver file is backed up in
  `/boot/home/pioneer-assistant-models-20261015/resolv.conf.before`.

## Verification on Pioneer, 2026-09-15

- RISC-V cross-build passed with warnings treated as errors; shell syntax and
  diff checks passed.
- Original subprocess GUI was confirmed working by the user before this change.
- Resident service team 598: greeting and arithmetic requests both succeeded;
  `ready=1 busy=0 model_loads=1` after each.
- Cancellation returned `Stopped. Model remains loaded.` A subsequent request
  succeeded, still team 598 and `model_loads=1`.
- Installed copies matched Mac hashes. `listimage` confirmed the shortcut
  loaded in input_server without restarting it.
- User confirmed the updated app and shortcut work.

These changes are installed on the currently booted **SD** system and were
subsequently copied to Samsung with explicit user approval. All five deployed
files (GUI, service, shortcut, control utility, launcher) matched their source
checksums. No existing Samsung files were overwritten. Its kernel, bootloader,
runtime, model, and user settings were not changed. Samsung was restored to a
read-only mount after syncing. Testing after booting Samsung remains pending.
The old SD GUI is backed up as `apps/HaikuAssistant.before-resident-20260915`.

Next: verify the installation after booting Samsung, create proper packages,
and add optional boot preload/settings.
The first native system-tools service is documented in [TOOLS.md](TOOLS.md).
Chat tool-calling completed a read-only end-to-end test and is installed on SD;
close/reopen the GUI to activate it. Samsung still has the earlier chat build.
An actual MCP transport remains separate work. Small-model tool selection and
summary accuracy are imperfect; prefer the displayed native result for facts.
