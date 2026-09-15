#!/bin/sh
# Download reference sources only. Never install modules or flash firmware.
set -eu
tt_root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
tt_dest=${1:-"$tt_root/generated.pioneer/tenstorrent"}
mkdir -p "$tt_dest"

fetch_source()
{
	tt_name=$1
	tt_revision=$2
	tt_path="$tt_dest/$tt_name"
	if [ -e "$tt_path" ]; then
		if [ -d "$tt_path/.git" ] && [ "$(git -C "$tt_path" rev-parse HEAD)" = "$tt_revision" ]; then
			printf '%s already present at %s (local changes preserved)\n' "$tt_name" "$tt_revision"
			return
		fi
		printf 'Refusing to overwrite existing %s\n' "$tt_path" >&2
		exit 1
	fi
	git init -q "$tt_path"
	git -C "$tt_path" remote add origin "https://github.com/tenstorrent/$tt_name.git"
	git -C "$tt_path" fetch --depth 1 origin "$tt_revision"
	git -C "$tt_path" checkout --detach FETCH_HEAD
	test "$(git -C "$tt_path" rev-parse HEAD)" = "$tt_revision"
}

fetch_source tt-kmd 22cca4cb33455d962cf8340a8f5b896dae696fd7
fetch_source tt-umd 5c9ea40f8e29f84c32dde2646f93e19e1d13dd9a
printf 'Reference sources downloaded to %s; nothing built, installed, or flashed.\n' "$tt_dest"
