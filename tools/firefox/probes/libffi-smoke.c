/* Check both RISC-V argument passing and executable closure trampolines. */
#include <ffi.h>
#include <stdint.h>
#include <stdio.h>

static int64_t mixed(int64_t integer, double fraction, void* pointer)
{
    return integer + (int64_t)(fraction * 100) + *(int64_t*)pointer;
}

static void callback(ffi_cif* cif, void* result, void** arguments, void* data)
{
    (void)cif;
    *(int64_t*)result = mixed(*(int64_t*)arguments[0],
        *(double*)arguments[1], *(void**)arguments[2]) + *(int64_t*)data;
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    ffi_cif cif;
    ffi_type* types[] = { &ffi_type_sint64, &ffi_type_double, &ffi_type_pointer };
    int64_t integer = 40, pointed = 7, bias = 3, result = 0;
    double fraction = 0.25;
    void* pointer = &pointed;
    void* arguments[] = { &integer, &fraction, &pointer };
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 3, &ffi_type_sint64, types) != FFI_OK)
        return 1;
    ffi_call(&cif, FFI_FN(mixed), &result, arguments);
    if (result != 72)
        return 2;
    puts("ffi_call integer/double/pointer: PASS");
    void* executable = NULL;
    ffi_closure* closure = ffi_closure_alloc(sizeof(ffi_closure), &executable);
    if (!closure)
        return 3;
    if (ffi_prep_closure_loc(closure, &cif, callback, &bias, executable) != FFI_OK)
        return 4;
    int64_t (*call)(int64_t, double, void*) = (int64_t (*)(int64_t, double, void*))executable;
    result = call(integer, fraction, pointer);
    ffi_closure_free(closure);
    if (result != 75)
        return 5;
    puts("ffi executable closure: PASS");
    puts("LIBFFI_SMOKE_PASS (not a Firefox browser test)");
    return 0;
}
