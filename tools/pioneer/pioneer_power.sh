#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ "$#" -lt 1 ]; then
	echo "Usage: pioneer_power.sh ACTION [relay options]" >&2
	echo "Actions: test, status, on, off, restart, sd-attach, sd-detach, sd-status" >&2
	echo "Run 'pioneer_power.sh --help' for full relay options." >&2
	exit 2
fi

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
	exec python3 "$SCRIPT_DIR/sh_ur04a.py" --help
fi

exec python3 "$SCRIPT_DIR/sh_ur04a.py" "$@"
