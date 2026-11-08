# Frozen BFP8 cache writer support

`PioneerModelWeightCache.h` and `PioneerModelWeightCache.cpp` are byte-exact
copies from commit `3c8fd42d51`. The previously deployed all-BFP8 recipe
includes both files in its source fingerprint. Keep these copies unchanged;
changing the current cache API must not silently relabel a different writer
as that existing recipe.

Their SHA-256 digests are respectively
`5caa3faa6bea60670d0fb7aba2c978829b809ed370c0a80d40496e70e5174ae0`
and `2c56f7ee522de23a8f775d8c1e6e041dd92d55086690b20e6b4db63ead18887d`.

The wrapper compiles the existing filtered BFP8 writer against these frozen
files. The current reader remains separate and has an existing-directory
only API. `pioneer_q38_frozen_bfp8_recipe_test.py` checks the complete old
source fingerprint and recipe key; the companion cache compatibility test
uses separate writer and reader processes with synthetic bytes. Neither is a
substitute for real model quality, device qualification, or a speed test.
