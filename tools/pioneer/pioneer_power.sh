#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ "$#" -lt 1 ]; then
	echo "Usage: pioneer_power.sh {test|on|off|restart} [relay options]" >&2
	echo "Run 'pioneer_power.sh --help' for full relay options." >&2
	exit 2
fi

if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
	exec python3 "$SCRIPT_DIR/sh_ur01a.py" --help
fi

exec python3 "$SCRIPT_DIR/sh_ur01a.py" "$@"
