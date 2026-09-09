# Match Haiku's CPU_GENERIC backend selection for the scalar comparison.
# The compilers remain native RISC-V Linux GCC; this only controls CMake's
# architecture dispatch in ggml. Do not use this for optimized Linux builds.
set(CMAKE_SYSTEM_PROCESSOR "other")
