# Haiku AGENTS.md

## Policies

- Have fun.

## Pioneer development box

- The user treats the Pioneer as a development box with work saved. Restarts
  and power-offs needed for this project are authorized without asking again
  whether work is saved.
- The relay controls the front-panel button, not an idempotent power switch:
  send OFF once while the board is known to be on. Allow a few minutes for
  shutdown, check available power-state evidence, then assume it is off and
  send ON once without routinely asking for a fans/LEDs confirmation. If
  evidence shows the board is still on or the OFF action failed, resolve
  that contradiction before sending ON. Never repeat OFF to confirm shutdown;
  it can start the board again. Relay contact state is not board power state.
- This standing authorization does not authorize erasing disks, changing SD
  cards, changing hardware connections, or switching mains power.
