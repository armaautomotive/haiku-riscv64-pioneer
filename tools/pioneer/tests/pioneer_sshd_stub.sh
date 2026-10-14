#!/bin/sh
set -eu
command=${0##*/}
line=$command
for argument do line="$line <$argument>"; done
printf '%s\n' "$line" >> "$SSHD_TEST_LOG"
case "$command" in
	id)
		case "$*" in
			sshd)
				if [ "$SSHD_TEST_SCENARIO" = missing ] && [ ! -e "$SSHD_TEST_CREATED" ]; then
					exit 1
				fi ;;
			'-u sshd')
				if [ "$SSHD_TEST_SCENARIO" = root ]; then echo 0; else echo 1000; fi ;;
			*) exit 92 ;;
		esac ;;
	useradd) touch "$SSHD_TEST_CREATED" ;;
	groupadd|sleep|ifconfig) : ;;
	chmod) [ "$SSHD_TEST_SCENARIO" != permissions ] || exit 22 ;;
	sshd)
		case " $* " in
			*' -t '*) [ "$SSHD_TEST_SCENARIO" != configuration ] || exit 21 ;;
			*' -D -e '*) : ;;
			*) exit 93 ;;
		esac ;;
	*) exit 94 ;;
esac
