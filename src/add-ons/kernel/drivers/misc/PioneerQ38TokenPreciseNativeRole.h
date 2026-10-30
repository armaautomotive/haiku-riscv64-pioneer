/* SPDX-License-Identifier: MIT */
// Exact QTP2 profile discriminator only. The enclosing TokenNativeRole also
// requires ALL21 native parents and six renamed ABI names on every include.
#if defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT)
#error QTP2 native profile cannot alias QTP3 selectors,including defined0
#endif
#if !defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || PIONEER_Q38_TOKEN_PRECISE_PROGRAMS != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x5154324e4f445256ULL
#error Exact QTP2/QGW2 QT2NODRV native compile profile/program family/tag required
#endif
