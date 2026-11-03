#!/bin/sh
# CPU/userspace tests only. Does not open a P100A device or change a driver.
set -eu
[ "$(uname -s)" = Haiku ] && [ "$(id -u)" = 0 ]
stage=/boot/home/pioneer-native-weight-cache-20261103-a
[ -d "$stage" ] && [ ! -L "$stage" ]
exact() {
  test -f "$stage/$1" && test ! -L "$stage/$1"
  row=$(sha256sum "$stage/$1"); printf '%s\n' "$row"
  [ "${row%% *}" = "$2" ]
}
exact pioneer-q38-native-weight-cache 264bc2842e74d38c3ca31cb549a9c8761a6e8fe9c7558e6073f188cb58604c7f
exact pioneer-model-weight-cache-test 92e501215288390799149547ecb432fc397aad45aa022d77e4a0d2b2ffce34f1
exact pioneer-blackhole-block-float-test ee249d1063b5641a59a68c3c37dbc6b2f2eed0302668f7b772c8485ea4068138
exact pioneer-q38-native-weight-tiles-test 9c6c2edc93213bf2b643dfef91d4ed83978c98f2f7f6eb0c1adb2a461acd8247
test ! -e "$stage/cache-fixtures" && test ! -e "$stage/tiles-fixtures"
mkdir -m 700 "$stage/cache-fixtures" "$stage/tiles-fixtures"
"$stage/pioneer-blackhole-block-float-test"
"$stage/pioneer-model-weight-cache-test" "$stage/cache-fixtures"
"$stage/pioneer-q38-native-weight-tiles-test" "$stage/tiles-fixtures"
echo HAIKU_CPU_ONLY_SELFTESTS_COMPLETED_NO_ACCELERATOR_ACCESS
