# Haiku Assistant prototype

Native Interface Kit chat window using the existing local Qwen3-0.6B Q8_0
model and llama-completion runtime. No llama.cpp or kernel changes are required.

## Current implementation

- Conversation above a multiline input; Send or Ctrl+Enter submits.
- Background worker starts llama-completion with explicit arguments, not a shell.
- Separate stdout/stderr pipes keep runtime diagnostics out of normal replies.
- Streamed response snapshots, Stop, and New chat.
- Closing cancels and reaps the inference child, then exits the app.
- Single-instance application resources; relaunch requests activate its window.
- Recent completed turns supply conversation context. Old turns are dropped
  above a 6,000-byte context budget; input is limited to 2,000 bytes. The byte
  budget is not a tokenizer guarantee; runtime context errors are displayed.
- Replies are capped at 256 tokens. Failed/cancelled turns are not reused.
- Conversation is memory-only and is not saved to disk.

This first backend reloads the model on every request. It uses the pinned
Qwen3 ChatML template with thinking disabled, four inference threads, and a
4,096-token context. It is not a general-purpose model-template adapter.

## Build and launch

From the repository on the Mac:

```sh
sh tools/pioneer/build_assistant.sh /private/tmp/HaikuAssistant
```

The script cross-compiles the app and attaches its application resources. It
does not modify a disk image or deploy anything. A Jam target `HaikuAssistant`
is also registered, but the app is not yet included in the system package.

On the persistent Samsung Haiku installation, defaults are:

- Runtime: `/boot/home/develop/llama-c060ca974c77/build-pioneer/bin/llama-completion`
- Model: `/boot/home/models/Qwen3-0.6B-Q8_0.gguf`

`LLAMA_RUNTIME` and `LLAMA_MODEL` override those paths. The launcher below also
sets `LIBRARY_PATH` for the existing runtime's shared libraries. Install the app
under `/boot/home/config/non-packaged/apps/HaikuAssistant`, and optionally the
launcher under `/boot/home/config/non-packaged/bin/haiku-assistant`.

```sh
sh /path/to/haiku-assistant.sh
```

## Validation status

RISC-V cross-build passes with warnings treated as errors (SDK headers treated
as system headers). One Linux smoke test using the matching llama revision and
model verified the no-conversation prompt format and successful completion.
This does **not** validate the Haiku GUI or its subprocess handling yet.

Hardware acceptance checklist:

1. Launch on Samsung Haiku; verify layout, input, streamed reply, and follow-up.
2. Move the window and type elsewhere during model loading/generation.
3. Stop during loading and generation; ensure the child exits and Send re-enables.
4. Close during generation; ensure no orphan llama-completion remains.
5. Test missing runtime/model, a failing child, Unicode text, and long input.
6. Relaunch the application; confirm it activates the existing window.

## Next stages

1. Hardware validation, then a resident inference service to avoid model reloads.
2. Global Ctrl+Space show/hide, preferably using Haiku's existing shortcut
   infrastructure. This shortcut is **not implemented or installed yet**.
3. Persisted settings and explicit opt-in conversation storage; package the app,
   runtime, and model selection/download support for the distribution.
4. A separate permission-checked system-tool service with MCP support. Start
   read-only, then require approval for window movement and other state changes.

No MCP client/server, shell execution tool, window automation, networking, or
system-inspection capability is exposed to the model by this prototype.
