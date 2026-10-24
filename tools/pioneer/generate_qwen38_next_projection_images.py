#!/usr/bin/env python3
"""Frozen distinct QPS2 ROOT; require complete unchanged QFP3 peer identities.

All eight binary sections authenticate before ANY patch output. No SDK,
native role,device open,mapping or execution. Existing QPS1 stays unchanged.
"""
import contextlib
import io
import pathlib
import runpy
import sys

module = runpy.run_path(str(pathlib.Path(__file__).with_name('generate_qwen38_projection_graph_images.py')))
PINS = dict(module['PINS'])
PINS.update({
    'root-main': ('text', 7692, 'd6b9ae3355f11fca212835ad2084c84a791091ee439354aaa2804e5894b163c6', 'RootMain'),
    'root-prefix': ('prefix', 6564, 'a38f05255608edc828287776654c8e1e94072c2170c1c3d0eaabf989cc8d86b7', 'RootPrefix'),
    'root-graph': ('graph', 16052, '15f6cc742173ec93247cb6002f01d40957b0bea929a61dcccd3f1de0e6c039f2', 'RootGraph'),
})


def main(args):
    if len(args) != 3 or args[2] not in ('root-main', 'root-prefix', 'root-graph'):
        raise ValueError('expected QPS2 ROOT, unchanged QFP3 PEER and one root section')
    emit = module['main']
    emit.__globals__['PINS'] = PINS
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        emit(args)
    # Only the new independently pinned ROOT sections are emitted;peers are
    # authenticated above and reused from their existing immutable catalogue.
    patch = output.getvalue().replace('QPR1/QFP3', 'QPS2/QFP3')
    patch = patch.replace('PioneerQ38ProjectionGraphRoot', 'PioneerQ38NextProjectionGraphRoot')
    patch = patch.replace('PIONEER_Q38_PROJECTION_GRAPH_', 'PIONEER_Q38_NEXT_PROJECTION_GRAPH_')
    print(patch, end='')


if __name__ == '__main__':
    main(sys.argv[1:])
