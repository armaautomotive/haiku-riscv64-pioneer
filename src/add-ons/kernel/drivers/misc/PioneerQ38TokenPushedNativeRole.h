/* SPDX-License-Identifier: MIT */
// Profile discriminator ONLY. TokenNativeRole still requires all21 parents
// and renamed conventional ABI on EVERY include. Not a loadable driver.
#if !defined(PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT) || PIONEER_Q38_TOKEN_PUSHED_NATIVE_CONTEXT != 1 \
 || !defined(PIONEER_Q38_TOKEN_PUSHED_PROGRAMS) || PIONEER_Q38_TOKEN_PUSHED_PROGRAMS != 1 \
 || !defined(PIONEER_DRIVER_BUILD_TAG) || PIONEER_DRIVER_BUILD_TAG != 0x5154334e4f445256ULL
#error Exact QTP3/QGW3 QT3NODRV native compile profile/program family/tag required
#endif
#if defined(PIONEER_Q38_TOKEN_PRECISE_PROGRAMS) || defined(PIONEER_Q38_TOKEN_PRECISE_NATIVE_CONTEXT)
#error QTP3 native profile cannot alias QTP2 selectors,including defined0
#endif
#ifdef PIONEER_Q38_TOKEN_PROGRAMS_H
#error QTP3 native profile cannot retain an already selected QTP1/QTP2 package
#endif
#ifdef PIONEER_Q38_TOKEN_FOOTPRINT_H
static_assert(PioneerQ38TokenL1::Family==0x33505451 && PioneerQ38TokenL1::SavedBytes==53913832,
 "QTP3 native profile requires its actual journal type,not late selector flags");
#endif
