#!/bin/sh
set -eu
directory=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
temporary=$(mktemp -d "${TMPDIR:-/tmp}/pioneer-sshd-test.XXXXXXXX")
trap 'rm -rf "$temporary"' EXIT HUP INT TERM
mkdir "$temporary/bin" "$temporary/ssh" "$temporary/rtl8125"
touch "$temporary/ssh/ssh_host_ed25519_key" "$temporary/ssh/authorized_keys" "$temporary/rtl8125/0"
cp "$directory/pioneer_sshd_stub.sh" "$temporary/bin/stub"
chmod 700 "$temporary/bin/stub"
for command in id useradd groupadd chmod sshd sleep ifconfig; do
	ln -s stub "$temporary/bin/$command"
done
# Substitute only platform paths; execute the complete production script.
sed -e "s|/boot/home/config/settings/ssh|$temporary/ssh|g" \
	-e "s|/dev/dprintf|$temporary/serial|g" \
	-e "s|/dev/net/rtl8125/\*|$temporary/rtl8125/*|g" \
	-e "s|/bin/sshd|$temporary/bin/sshd|g" \
	-e "s|/bin/ifconfig|$temporary/bin/ifconfig|g" \
	"$directory/../pioneer_start_sshd.sh" > "$temporary/start.sh"
export PATH="$temporary/bin:$PATH"
export SSHD_TEST_LOG="$temporary/commands" SSHD_TEST_CREATED="$temporary/created"
for SSHD_TEST_SCENARIO in existing missing root configuration permissions; do
	export SSHD_TEST_SCENARIO
	: > "$SSHD_TEST_LOG"
	: > "$temporary/serial"
	rm -f "$SSHD_TEST_CREATED"
	result=0
	sh "$temporary/start.sh" --serial-diagnostics || result=$?
	# The bounded reporter is asynchronous; wait for its final stub invocation.
	if [ "$result" = 0 ]; then
		attempt=0
		while [ "$(grep -c '^ifconfig ' "$SSHD_TEST_LOG" || :)" -lt 4 ]; do
			attempt=$((attempt + 1)); [ "$attempt" -lt 100 ]
			/bin/sleep 0.01
		done
	fi
	case "$SSHD_TEST_SCENARIO" in
		existing|missing)
			[ "$result" = 0 ]
			grep -q 'account/permissions/configuration passed' "$temporary/serial"
			[ "$(grep -c '^sshd ' "$SSHD_TEST_LOG")" = 2 ]
			grep -q '<-D> <-e>' "$SSHD_TEST_LOG"
			grep -q '<PasswordAuthentication=no>' "$SSHD_TEST_LOG"
			grep -q '<PermitRootLogin=prohibit-password>' "$SSHD_TEST_LOG"
			if [ "$SSHD_TEST_SCENARIO" = missing ]; then test -f "$SSHD_TEST_CREATED"; fi ;;
		root) [ "$result" = 1 ]; grep -q 'failed stage=account status=1' "$temporary/serial" ;;
		configuration) [ "$result" = 21 ]; grep -q 'failed stage=configuration-test status=21' "$temporary/serial" ;;
		permissions) [ "$result" = 22 ]; grep -q 'failed stage=permissions status=22' "$temporary/serial" ;;
	esac
	if [ "$result" != 0 ]; then ! grep -q '<-D>' "$SSHD_TEST_LOG"; fi
done
SSHD_TEST_SCENARIO=existing; export SSHD_TEST_SCENARIO
: > "$SSHD_TEST_LOG"
sh "$temporary/start.sh"
[ "$(grep -c '^sshd ' "$SSHD_TEST_LOG")" = 2 ]
result=0
sh "$temporary/start.sh" --unknown > /dev/null 2>&1 || result=$?
[ "$result" = 64 ]
grep -q 'requires x-vnd.Haiku-registrar' "$directory/../pioneer-ssh.launch"
printf 'PASS: SSH startup, account guards, configuration failure, opt-in serial diagnostics\n'
