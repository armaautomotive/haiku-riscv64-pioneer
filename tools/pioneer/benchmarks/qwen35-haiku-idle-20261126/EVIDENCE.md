# Haiku host availability and idle check — 2026-11-26

Strict SSH using the saved Haiku host-key pin reached Haiku. The process-list
check found no known 27B or 4B inference diagnostic, and the exclusive lock
was absent. Installed stable driver SHA-256
`041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403`,
admin SHA-256 `b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f`
and guard SHA-256 `adbc86495803654471b83330ea8f2591f0433f8d71e083d6e9f790685bc985a4`
matched the recorded stable files. The trusted admin `--status` returned
`driver55 tag2026102100010012 generation2 quiesced0 cold0 active_owner0 open_count1 guard1;CPU_METADATA_ONLY`.

The 4B and 27B files had expected sizes 2,740,937,888 and 18,973,870,528
bytes, respectively. The 4B full SHA-256 was
`00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4`.
The 27B full SHA-256 also matched
`c600de0300ae8a0eb3a6c0b8b5561b8b96f16bd2c863c2a66c42de29d391a747`. These are
point-in-time observations: repeat process, lock and stable-driver preflight
immediately before any future card access. The check accessed no card and
establishes no model execution, physical recovery, or throughput.
