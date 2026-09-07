#!/bin/sh

set -eu

ssh_directory=/boot/home/config/settings/ssh
host_key=$ssh_directory/ssh_host_ed25519_key

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

mkdir -p "$ssh_directory"
chmod 700 "$ssh_directory"
chmod 600 "$ssh_directory/authorized_keys" "$host_key"

exec /bin/sshd -D -e \
	-h "$host_key" \
	-o PidFile="$ssh_directory/sshd.pid" \
	-o AuthorizedKeysFile="$ssh_directory/authorized_keys" \
	-o PermitRootLogin=prohibit-password \
	-o PasswordAuthentication=no \
	-o KexAlgorithms=curve25519-sha256 \
	-o HostKeyAlgorithms=ssh-ed25519 \
	-o Ciphers=aes128-ctr
