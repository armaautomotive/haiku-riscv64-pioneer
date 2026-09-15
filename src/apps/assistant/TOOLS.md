# Native Haiku system-tools service

`HaikuAssistantTools` is a background BApplication, independent of the resident
model. It uses local Haiku BMessage/BMessenger IPC. **It is not an MCP server:**
there is no JSON-RPC, stdio adapter, HTTP endpoint, or MCP lifecycle yet.
The chat/model does not automatically call these tools yet; this is the service
and native client layer for that later integration.

## Installed components and commands

Service: `/boot/home/config/non-packaged/servers/HaikuAssistantTools`

Client: `/boot/home/config/non-packaged/bin/haiku-tools`

The client starts the service if necessary. Examples:

```sh
haiku-tools list_tools
haiku-tools system_info
haiku-tools list_apps
haiku-tools list_windows
haiku-tools launch_app DeskCalc
haiku-tools move_window TEAM TOKEN 100 100
haiku-tools resize_window TEAM TOKEN 500 300
haiku-tools focus_window TEAM TOKEN
haiku-tools --stop-service
```

Use actual team/token pairs from a fresh `list_windows` result. Width and height
are Haiku BRect extents. Move/resize keeps the whole requested frame on the
current screen, with a title-bar margin. Immovable/unresizable windows are
rejected for their respective operations. Window title, team, client port/token,
visibility and original frame are revalidated after approval; a changed target
requires a new request.

Launch allowlist: Terminal, StyledEdit, DeskCalc, WebPositive. Only installed
applications are launched, by their resolved application entry; no arguments,
document paths, executable paths, or shell command strings are accepted.

## Consent and security boundaries

- System info, running-app and visible-normal-window lists are read-only.
- Every mutation shows a native dialog with the exact action and target.
  Deny is the default; Escape denies. Consent expires after 60 seconds.
- Only one approval may be pending. A second mutation is rejected as busy.
- An unpredictable nonce binds the dialog result to the stored request.
  A client-supplied `approved` field has no meaning. Failure to obtain random
  bytes or create the timeout fails closed.
- The original message is retained using `DetachCurrentMessage`, rather than
  copied, to preserve Haiku's synchronous reply metadata through the dialog.
- Lists are capped at 64 entries; displayed window titles at 256 bytes. Tool
  requests above 8 KiB are rejected. Coordinates must be finite.
- No model-accessible shell, deletion, file read/write, process termination,
  password entry, network listener, privilege escalation, or approval bypass.
- This is a consent boundary for cooperative local clients, **not an OS
  sandbox**. Other local programs retain their existing Haiku privileges.
  Window titles and app identities may contain private information; a future
  external MCP adapter needs its own access and disclosure controls.

## Native interface

Signature: `application/x-vnd.Haiku-AssistantTools`.
Constants are in `ToolsProtocol.h`.

Send `kToolCall` (`'tool'`) with string `tool`. Arguments are top-level typed
BMessage fields:

| Tool | Arguments |
|---|---|
| list_tools, system_info, list_apps, list_windows | none |
| launch_app | string `application` from the allowlist |
| focus_window | int32 `team`, int32 `token` |
| move_window | team/token, float `a`=x, float `b`=y |
| resize_window | team/token, float `a`=width, float `b`=height |

Response `kToolResult` (`'tres'`): int32 `status` (B_OK on success), string
`text`, and optional BMessage `data`. Read-only results also provide typed
CPU/memory/uptime/OS fields or repeated `items` messages. Mutation calls may
wait for approval; the client uses a 65-second reply timeout.

Read-only operations never interpret titles or application data as commands.
This first interface is internal and versioned with the distribution, not a
promise of long-term compatibility for third-party clients.

## Build and verification

`sh tools/pioneer/build_assistant.sh` builds the service/client in addition to
the GUI components. No llama library is required for these two binaries.
Jam targets are also registered.

On the current Pioneer SD Haiku session:

- Cross-build, shell syntax and diff checks passed.
- Tool catalog, system info (64 CPUs), running apps and window queries passed.
- Unknown shell tool, `/bin/sh` launch, and nonexistent window were rejected.
- Corrected an asynchronous-reply bug found during initial consent testing.
- An approved DeskCalc launch completed and returned success; the new window
  was independently observed in `list_windows`.
- Further move/resize/focus and deny/expiry results are recorded in
  `tools/pioneer/NATIVE_BUILD_STATUS.md` as tests complete.

Installed on SD only for testing. The Samsung chat deployment is unchanged by
this service work. No kernel, firmware, model or llama-source modifications.

## Chat bridge

`ToolBridge` validates bounded, single `<tool_call>` JSON responses against the
allowlist and exact argument schemas. `ToolSession` sends native requests,
feeds escaped tool results back to the resident model, and displays the tool
name and result in the conversation. A turn is limited to three tool calls.
Denial, expiration or failure ends the turn without automatic retries. Stop
cancels a pending request; it cannot undo an already-approved action.

Optional int64 `id` is echoed in tool replies. `kToolCancel` cancels a matching
pending request only when its return messenger matches the original caller.
Desktop changes still require the native one-time consent dialog.

`HaikuAssistantToolFlowTest --parser-tests` passed all 17 cases on Pioneer.
Run the same binary with a question to exercise the shared chat orchestration
without replacing an open GUI. A CPU/RAM query completed the full model/tool/
model round-trip. Qwen then misconverted the correct byte count in its summary;
the native result now includes formatted GiB (verified 126.93 GiB). Treat the
native result as authoritative, not model arithmetic. The updated GUI is
installed on SD and needs close/reopen; manual GUI tool testing is pending.
This is a native Haiku service, not an MCP-compatible transport.
