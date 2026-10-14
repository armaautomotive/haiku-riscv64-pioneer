#!/bin/sh

set -eu

diagnostics=0
case "${1-}" in
	'') test "$#" -eq 0 ;;
	--serial-diagnostics) test "$#" -eq 1; diagnostics=1 ;;
	*) echo "Usage: $0 [--serial-diagnostics]" >&2; exit 64 ;;
esac

# Release launch_daemon otherwise discards stdout/stderr. Opt in explicitly;
# never trace shell commands, configuration files, or private-key contents.
if [ "$diagnostics" = 1 ]; then
	exec > /dev/dprintf 2>&1
	stage=begin
	trap 'result=$?; if [ "$result" -ne 0 ]; then
		printf "PIONEER-SSH: failed stage=%s status=%s\n" "$stage" "$result"
	fi' EXIT
	printf 'PIONEER-SSH: startup diagnostics v1\n'
fi

ssh_directory=/boot/home/config/settings/ssh
host_key=$ssh_directory/ssh_host_ed25519_key

stage=account
# Image construction does not run OpenSSH's account-creation install hook.
# Never disable privilege separation or fall back to passwordless access.
if ! id sshd >/dev/null 2>&1; then
	# An existing group is harmless; useradd validates it before proceeding.
	groupadd sshd || :
	useradd -g sshd -d /var/empty -s /bin/false sshd
fi
if [ "$(id -u sshd)" = 0 ]; then
	echo 'Refusing privileged sshd service account.' >&2
	exit 1
fi

stage=permissions
mkdir -p "$ssh_directory"
chmod 700 "$ssh_directory"
chmod 600 "$ssh_directory/authorized_keys" "$host_key"

# Offline package deployment may not initialize OpenSSH's .settings link.
# Use only the explicit configuration below, not the package default file.
set -- -f /dev/null \
	-h "$host_key" \
	-o PidFile="$ssh_directory/sshd.pid" \
	-o AuthorizedKeysFile="$ssh_directory/authorized_keys" \
	-o PermitRootLogin=prohibit-password \
	-o PasswordAuthentication=no \
	-o KexAlgorithms=curve25519-sha256 \
	-o HostKeyAlgorithms=ssh-ed25519 \
	-o Ciphers=aes128-ctr

stage=configuration-test
/bin/sshd "$@" -t

if [ "$diagnostics" = 1 ]; then
	printf 'PIONEER-SSH: account/permissions/configuration passed; starting daemon\n'
	# Only local wired-interface status; no packet capture or settings dump.
	# Bounded and independent of sshd's lifetime, including a failed exec.
	(
		for delay in 0 10 20 30; do
			sleep "$delay"
			printf 'PIONEER-SSH: wired interface snapshot\n'
			for interface in /dev/net/rtl8125/*; do
				[ -e "$interface" ] || continue
				/bin/ifconfig "$interface" || :
			done
		done
	) < /dev/null &
fi

stage=daemon-exec
exec /bin/sshd "$@" -D -e
