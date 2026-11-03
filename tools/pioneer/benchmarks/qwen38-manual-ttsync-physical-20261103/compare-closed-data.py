import pathlib,sys
from fractions import Fraction
sys.path.insert(0,"tools/pioneer/tests")
import compare_pioneer_q38_token_constant_q4_log as old
import compare_pioneer_q38_token_manual_ttsync_log as new
from compare_pioneer_q38_frontier_benchmark_pair import bounds
root=pathlib.Path("/Users/arma/Documents/ChatGPT/HaikuOS/tools/pioneer/benchmarks/qwen38-token-constant-q4-physical-20261102")
current=pathlib.Path("/private/tmp/pioneer-q38-manual-watchdog-20261103.ipSSfU")
a,ar,ap=old.read_log(root/"physical.log",root/"physical.stderr")
b,br,bp=new.read_log(current/"physical.log",current/"physical.stderr")
assert len(a)==len(b)==34 and ap["valid"] and bp["valid"]
changed=[p for p in range(34) if a[p][:12]!=b[p][:12]]
assert not changed,changed
assert all(ar[k]==br[k] for k in ("sampled_tokens","first_committed","last_committed"))
al,ah=bounds(ar);bl,bh=bounds(br)
print("CLOSED_LOG_DATA_ONLY all34 first12 words EXACT;NOT external full-model numerical acceptance")
print("MATCHED_COUNT_WINDOW sampled_tokens32 first_committed2 last_committed34")
print("PREVIOUS_RATE",ar)
print("CURRENT_RATE",br)
print("DESCRIPTIVE_RATE_CHANGE_PERCENT_NOT_CONTROLLED_PAIRED_SPEEDUP",float(100*(bl/ah-1)),float(100*(bh/al-1)))
print("NO contemporaneous controlled A/B,repeatability,causal attribution,hardware health or Work claim;unchanged strict CPU readers both exit1")
