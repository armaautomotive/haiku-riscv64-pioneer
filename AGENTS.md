# Haiku AGENTS.md

## Policies

- Have fun.

## Pioneer development box

- The user treats the Pioneer as a development box with work saved. Restarts
  and power-offs needed for this project are authorized without asking again
  whether work is saved.
- Still verify actual power state. The relay controls the front-panel button,
  not an idempotent power switch: send OFF once while the board is on, verify
  fans/LEDs are off, then send ON once. Never repeat OFF to confirm shutdown;
  it can start the board again. Relay contact state is not board power state.
- This standing authorization does not authorize erasing disks, changing SD
  cards, changing hardware connections, or switching mains power.
