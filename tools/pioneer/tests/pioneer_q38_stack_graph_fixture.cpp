/* SPDX-License-Identifier: MIT */
#if defined(_KERNEL_MODE) || defined(PIONEER_Q38_TOKEN_NATIVE_CONTEXT) || defined(PIONEER_Q38_WORK_INFERENCE)
#error Stack-audit compiler fixture is isolated TEST code,never a native driver
#endif
extern "C" {
__attribute__((noinline)) int stack_probe_leaf(int x){volatile int data[8]={};data[x&7]=x;return data[x&7];}
__attribute__((noinline)) int stack_probe_good(int x){volatile int saved=x;return stack_probe_leaf(x)+saved;}
int stack_probe_unknown(int);
__attribute__((noinline)) int stack_probe_external(int x){volatile int saved=x;return stack_probe_unknown(x)+saved;}
__attribute__((noinline)) int stack_probe_indirect(int x,int (*callback)(int)){volatile int saved=x;return callback(x)+saved;}
int stack_probe_recursive_b(int);
__attribute__((noinline)) int stack_probe_recursive_a(int x){volatile int saved=x;return x>0?stack_probe_recursive_b(x-1)+saved:stack_probe_leaf(x);}
__attribute__((noinline)) int stack_probe_recursive_b(int x){volatile int saved=x;return x>0?stack_probe_recursive_a(x-1)+saved:stack_probe_leaf(x);}
__attribute__((noinline)) int stack_probe_dynamic(int x){volatile int data[(x&31)+1];data[0]=x;return stack_probe_leaf(data[0]);}
}
