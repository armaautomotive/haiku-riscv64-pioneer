import ast
import hashlib
import pathlib
import subprocess

base = pathlib.Path('/private/tmp/pioneer-q38-manual-native-20261102.20fpwL')
main = pathlib.Path('/Users/arma/Documents/ChatGPT/HaikuOS')
tool = '/Volumes/HaikuBuildLocal/generated.riscv64/cross-tools-riscv64/bin/riscv64-unknown-haiku-g++'
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
for source, name, public in ((main, 'main-private-r1', False), (base/'clean-source', 'clean-private-r2', False),
                             (main, 'main-public-r1', True), (base/'clean-source', 'clean-public-r1', True)):
    directory = base/name
    script = source/'tools/pioneer'/('build_qwen38_token_manual_ttsync_benchmark_native.py' if public else 'build_qwen38_token_manual_ttsync_native.py')
    constants = {t.id: ast.literal_eval(n.value) for n in ast.parse(script.read_text()).body if isinstance(n, ast.Assign)
                 for t in n.targets if isinstance(t, ast.Name) and t.id in ('PRIVATE_SHA', 'OBJECT_SHA', 'MODULE_SHA')}
    expected = constants['OBJECT_SHA' if public else 'PRIVATE_SHA']
    command = (directory/'compile-argv.txt').read_text().splitlines()
    own = source/'tools/pioneer/tests'/('pioneer_q38_token_manual_ttsync_benchmark_native_compile.cpp' if public else 'pioneer_q38_token_manual_ttsync_native_context_compile.cpp')
    assert command[0] == tool and command[command.index('-c')+1] == str(own)
    old = pathlib.Path(command[command.index('-o')+1])
    fresh = base/(name+'-FINAL-SOURCE.o')
    assert not fresh.exists() and sha(old) == expected
    command[command.index('-o')+1] = str(fresh)
    subprocess.run(command, check=True)
    assert sha(fresh) == expected and fresh.read_bytes() == old.read_bytes()
    if public:
        link = (directory/'link-argv.txt').read_text().splitlines()
        previous = pathlib.Path(link[-1])
        target = base/(name+'-FINAL-SOURCE-module')
        assert link.count(str(old)) == 1 and not target.exists() and sha(previous) == constants['MODULE_SHA']
        link = [str(fresh) if word == str(old) else str(target) if word == str(previous) else word for word in link]
        subprocess.run(link, check=True)
        assert sha(target) == constants['MODULE_SHA'] and target.read_bytes() == previous.read_bytes()
    print('PASS_FINAL_CURRENT_SOURCE_FROZEN_IDENTITY '+name+' '+expected+';NO fresh ancestor,lifecycle,hardware or speed grant', flush=True)
