# Haiku Assistant

Native local Qwen chat, backed by a separate resident llama.cpp service.
No kernel changes or network listener. A native tools bridge is under testing;
desktop mutations require one-time native approval (see [TOOLS.md](TOOLS.md)).

## Components

- `HaikuAssistant`: conversation/input window. Send or Ctrl+Enter submits.
  Stop cancels the current request. New chat clears conversation context.
- **Models…** opens the native model manager: choose an installed model or
  install another from the curated Hugging Face catalog, with progress and
  cancellation. Model files stay in `~/models` and are shared across chats.
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

## Model installation and selection

The initial, deliberately small catalog contains Qwen3 0.6B Q8_0, 1.7B Q8_0,
and 4B Q4_K_M from the official Qwen repositories. The 0.6B model is the
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
and activation. This service uses the **CPU**, not the P100A. The experimental
P100A TinyStories harness is separate; neither a working full-Qwen backend nor
Qwen3.8 architecture support exists in this pinned llama runtime yet.

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
request without reloading the model. Context is 4,096 tokens, batches are 64,
and the maximum response is 256 tokens.
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
`HaikuAssistantControl`, and `HaikuAssistantModelStoreTest`, plus tools-bridge
binaries/tests. Without the library variable it skips the service.
It attaches application resources and does not deploy or write a disk image.

Install, preserving an old version before replacing it:

| Artifact | Path below `/boot/home/config/non-packaged/` |
|---|---|
| HaikuAssistant | `apps/HaikuAssistant` |
| HaikuAssistantService | `servers/HaikuAssistantService` |
| HaikuAssistantShortcut | `add-ons/input_server/filters/HaikuAssistantShortcut` |
| HaikuAssistantControl | `bin/assistantctl` |

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
