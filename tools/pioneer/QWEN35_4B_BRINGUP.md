# Qwen3.5-4B on Pioneer P100A

November 17, 2026: switch development to the smaller dense model. Keep the
27B model and driver work intact. The 27B GitHub checkpoint is `ec4b44ffe9`;
the tag224 diagnostic already running on the Pioneer was allowed to continue
without touching its device, driver, or model files. The 27B goal is paused.
Do not run a 4B card trial while the 27B run owns the card or the driver is
quarantined. A terminal result and independent idle stable-driver check are
required first.

## Pinned 4B development model

- Base: [`Qwen/Qwen3.5-4B`](https://huggingface.co/Qwen/Qwen3.5-4B), Apache 2.0.
- Quantized text model: [`unsloth/Qwen3.5-4B-GGUF`](https://huggingface.co/unsloth/Qwen3.5-4B-GGUF/tree/e87f176479d0855a907a41277aca2f8ee7a09523),
  `Qwen3.5-4B-Q4_K_M.gguf`, revision `e87f176479d0855a907a41277aca2f8ee7a09523`.
- Size: 2,740,937,888 bytes. SHA-256:
  `00fe7986ff5f6b463e62455821146049db6f9313603938a70800d1fb69ef11a4`.

The full download matched the publisher's SHA-256. The independent local
preflight rehashes the entire file and validates the 426 descriptors on the
same open handle. No vision projector is required for this text target.

```sh
python3 -B tools/pioneer/qwen35_4b_p100a_preflight.py /path/to/model.gguf
python3 -B tools/pioneer/tests/qwen35_4b_p100a_preflight_test.py /path/to/model.gguf
```

The upstream config and verified GGUF agree on 32 layers, one full attention
layer every four, and widths of 2560 (hidden), 4096 (recurrent output) and
9216 (FFN). The model's embedding and output head share the same Q6_K tensor;
the 27B has separate tensors. The 4B model is about 6.9 times smaller on disk,
but this is not a speed estimate. Its GGUF has 48 Q5_K recurrent projection
matrices that the existing 27B role does not support. Reading the alternate
Bartowski Q4_K_M header located 16 Q5_K tensors there too; it is a different,
larger artifact and was not selected or loaded. The source should stay fixed
for matched comparisons.

## First card milestones

1. Use real 4B bands to validate the existing Tensix Q4_K/Q6_K/Q8_0 math
   at the three new input widths. Add a lossless Q5_K operand path and prove
   it against the original quantized coefficients and independent CPU
   reference in the separate validation target. No CPU inference in Work.
2. Bind the small ROOT/worker layout, 24 recurrent and eight full-attention
   layers and tied-vocabulary readout. Run a short exact-model comparison
   to verify prefill, decode, state, and clean closure before tuning speed.
3. Measure repeated warm 4B generation separately from authentication and
   load. Promote any change to Assistant only after it passes that test.
   Keep tuning applicable kernels at real 27B widths before resuming that
   goal; a result on the 4B does not establish a 27B rate.

Current checkpoint is download, authentication, and layout only. No 4B
driver or firmware has been installed and no 4B inference has been measured.
