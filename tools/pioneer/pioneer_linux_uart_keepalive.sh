#!/bin/sh
# Temporary native runtime-PM policy only. Never loads or executes an image.
set -eu
fail() { printf 'BLOCKED: %s\n' "$1" >&2; exit 1; }
[ "$#" -eq 1 ] || fail 'Usage: pioneer_linux_uart_keepalive.sh status|enable|restore'
case "$1" in status|enable|restore) ;; *) fail 'Unknown action' ;; esac
[ "$(uname -s)" = Linux ] && [ "$(uname -m)" = riscv64 ] \
	&& [ "$(uname -r)" = 6.1.31 ] || fail 'Expected recorded native Linux 6.1.31'
uart=/sys/devices/platform/soc/7040000000.serial
state_dir=/run/pioneer-uart-keepalive
[ "$(readlink -f /sys/class/tty/ttyS0/device)" = "$uart" ] \
	|| fail 'Unexpected native ttyS0 binding'
[ "$(findmnt -n -o SOURCE /)" = /dev/nvme0n1p3 ] || fail 'Unexpected root'
show_status()
{
	printf 'UART power/control: '; cat "$uart/power/control"
	printf 'UART runtime_status: '; cat "$uart/power/runtime_status"
}
[ "$1" != status ] || { show_status; exit 0; }
[ "$(id -u)" = 0 ] || fail 'Use root for the runtime policy change'
[ "$(cat /sys/kernel/kexec_loaded)" = 0 ] || fail 'Require an empty kexec slot'
case "$1" in
	enable)
		[ "$(cat "$uart/power/control")" = auto ] || fail 'Do not take ownership of another on policy'
		[ ! -e "$state_dir" ] && [ ! -L "$state_dir" ] || fail 'Existing state; inspect before reuse'
		umask 077
		mkdir -m 0700 "$state_dir"
		printf 'auto\n' > "$state_dir/control.before"
		chown 0:0 "$state_dir" "$state_dir/control.before"
		printf 'on\n' > "$uart/power/control"
		[ "$(cat "$uart/power/control")" = on ] \
			&& [ "$(cat "$uart/power/runtime_status")" = active ] \
			|| fail 'UART did not become active; restore using the saved state'
		printf 'ENABLED: UART runtime suspension forbidden until restore/reboot\n'
		;;
	restore)
		if [ ! -e "$state_dir" ] && [ ! -L "$state_dir" ]; then
			printf 'No owned volatile state; no policy changed\n'
			show_status
			exit 0
		fi
		[ -d "$state_dir" ] && [ ! -L "$state_dir" ] \
			&& [ "$(stat -c '%u:%g:%a' "$state_dir")" = 0:0:700 ] \
			|| fail 'Unknown state directory'
		[ -f "$state_dir/control.before" ] && [ ! -L "$state_dir/control.before" ] \
			&& [ "$(stat -c '%u:%g:%a:%h:%s' "$state_dir/control.before")" = 0:0:600:1:5 ] \
			&& [ "$(cat "$state_dir/control.before")" = auto ] \
			|| fail 'Unknown saved policy; do not remove it'
		printf 'auto\n' > "$uart/power/control"
		[ "$(cat "$uart/power/control")" = auto ] || fail 'Could not restore runtime policy'
		unlink "$state_dir/control.before"
		rmdir "$state_dir"
		printf 'RESTORED: original auto policy; removed owned volatile state\n'
		;;
esac
show_status
