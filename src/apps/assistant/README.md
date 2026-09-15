# Haiku Assistant

Native local Qwen chat, backed by a separate resident llama.cpp service.
No kernel changes or network listener. A native tools bridge is under testing;
desktop mutations require one-time native approval (see [TOOLS.md](TOOLS.md)).

## Components

- `HaikuAssistant`: conversation/input window. Send or Ctrl+Enter submits.
  Stop cancels the current request. New chat clears conversation context.
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
restart it. A failed model load currently requires restarting the service.

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

Model path uses `LLAMA_MODEL` when set, otherwise
`/boot/home/models/Qwen3-0.6B-Q8_0.gguf`, then
`/Haiku1/home/models/Qwen3-0.6B-Q8_0.gguf` as an SD-session fallback.
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
and `HaikuAssistantControl`. Without the library variable it skips the service.
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
