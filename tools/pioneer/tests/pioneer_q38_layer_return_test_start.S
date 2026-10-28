/* SPDX-License-Identifier: MIT */
/* TEST CALLERS ONLY. No boot/reset entry or native ABI. Parent bytes are
 * canaries, not a fabricated model/producer/ownership completion record. */
.section .text.layer_return_test_entry,"ax",@progbits
.balign 4
.global layer_return_test_begin_recurrent
layer_return_test_begin_recurrent:
 li sp,0x6fb00
 mv s11,sp
 call q38_return_recurrent_layer
 j layer_return_test_returned
.global layer_return_test_begin_dense
layer_return_test_begin_dense:
 li sp,0x6fb00
 mv s11,sp
 call q38_return_dense_layer
 j layer_return_test_returned
.section .test_fault,"ax",@progbits
.balign 4
.global scalar_fault_park
scalar_fault_park:
 j scalar_fault_park
.global layer_return_test_returned
layer_return_test_returned:
 j layer_return_test_returned
