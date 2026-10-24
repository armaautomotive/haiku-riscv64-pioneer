#!/bin/sh
# Offline DATA + emitted root/119 arithmetic peers; never native/card IO.
set -eu
[ "$#" = 4 ] || [ "$#" = 5 ] || exit 2
python3 -c 'import os,sys;sys.exit(0 if all(not k.startswith("PIONEER_") or v=="0" for k,v in os.environ.items()) else 2)' || exit 2
here=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
python3 "$here/pioneer_q38_next_gdn_output_image_gates_test.py"
python3 "$here/pioneer_q38_next_gdn_output_generator_test.py" "$1" "$2"
sh "$here/run_pioneer_q38_next_gdn_output_catalog_tests.sh" "$1" "$2"
sh "$here/run_pioneer_q38_next_gdn_output_shape_tests.sh"
sh "$here/run_pioneer_q38_next_gdn_output_input_image_tests.sh" "$2" "$3" "$4"
sh "$here/run_pioneer_q38_next_gdn_output_graph_image_tests.sh" "$1" "$2"
sh "$here/run_pioneer_q38_next_gdn_output_graph_image_tests.sh" "$1" "$2" --faults
if [ "$#" = 5 ];then sh "$here/run_pioneer_q38_next_gdn_output_kernel_tests.sh" "$5";fi
printf 'PASS composed QOP2/QOI2/QOG2 offline component;NEXT actual native61 consumer/learned-model proof still REQUIRED;NO_HARDWARE_TPS_CLAIM\n'
