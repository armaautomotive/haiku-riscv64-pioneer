#!/usr/bin/env python3
"""Verify offline synthetic recurrent matrix bytes and role boundaries."""
import os
from pathlib import Path
import subprocess
import tempfile

here=Path(__file__).resolve().parent
compiler=os.environ.get('CXX','c++')
assert not any(k.startswith(('PIONEER_','TT_PIONEER_')) for k in os.environ)
flags=['-DPIONEER_Q38_SYNTHETIC_EMBEDDING_TEST=1','-DPIONEER_Q38_SYNTHETIC_QGW8_LAYER_TEST=1',
       '-DPIONEER_Q38_SYNTHETIC_RECURRENT_MATRIX_TEST=1']
with tempfile.TemporaryDirectory(prefix='q38-synthetic-recurrent-',dir='/private/tmp') as tmp:
    binary=str(Path(tmp)/'test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',binary],check=True)
    subprocess.run([binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    gate_binary=str(Path(tmp)/'gate-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1',
                    str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',gate_binary],check=True)
    subprocess.run([gate_binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    qkv_binary=str(Path(tmp)/'qkv-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST=1',str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',qkv_binary],check=True)
    subprocess.run([qkv_binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    conv_binary=str(Path(tmp)/'conv-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST=1','-DPIONEER_Q38_SYNTHETIC_CONV_STATE_TEST=1',
                    str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',conv_binary],check=True)
    subprocess.run([conv_binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    gdn_binary=str(Path(tmp)/'gdn-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST=1','-DPIONEER_Q38_SYNTHETIC_CONV_STATE_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST=1',
                    str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',gdn_binary],check=True)
    subprocess.run([gdn_binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    output_binary=str(Path(tmp)/'output-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST=1','-DPIONEER_Q38_SYNTHETIC_CONV_STATE_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST=1','-DPIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST=1',
                    str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',output_binary],check=True)
    subprocess.run([output_binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    ffn_binary=str(Path(tmp)/'ffn-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST=1','-DPIONEER_Q38_SYNTHETIC_CONV_STATE_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST=1','-DPIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST=1',
                    str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',ffn_binary],check=True)
    subprocess.run([ffn_binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    ffn_up_binary=str(Path(tmp)/'ffn-up-test')
    subprocess.run([compiler,'-std=c++17','-O1','-g','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',
                    '-fsanitize=address,undefined',*flags,'-DPIONEER_Q38_SYNTHETIC_GATE_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_QKV_MATRIX_TEST=1','-DPIONEER_Q38_SYNTHETIC_CONV_STATE_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_GDN_WEIGHT_TEST=1','-DPIONEER_Q38_SYNTHETIC_OUTPUT_MATRIX_TEST=1',
                    '-DPIONEER_Q38_SYNTHETIC_FFN_MATRIX_TEST=1','-DPIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST=1',
                    str(here/'pioneer_q38_synthetic_recurrent_matrix_test.cpp'),
                    str(here/'pioneer_q38_synthetic_embedding_bridge.cpp'),'-o',ffn_up_binary],check=True)
    subprocess.run([ffn_up_binary,str(here/'synthetic_q4_qgw8_layer.fixture')],check=True)
    for value in (0,1,2):
        result=subprocess.run([compiler,'-std=c++17',*flags,'-DPIONEER_Q38_SYNTHETIC_FFN_UP_MATRIX_TEST='+str(value),
                               '-fsyntax-only',str(here/'pioneer_q38_synthetic_embedding_bridge.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Synthetic FFN up matrix requires' in result.stderr
    for forbidden in ('_KERNEL_MODE','PIONEER_Q38_TOKEN_NATIVE_CONTEXT','PIONEER_Q38_WORK_INFERENCE',
                      'TT_PIONEER_Q38_WORK_MODE','PIONEER_Q38_FULL_OFFLOAD_NATIVE','PIONEER_Q38_FULL_OFFLOAD_CLIENT'):
        result=subprocess.run([compiler,'-std=c++17',*flags,'-D'+forbidden,'-fsyntax-only',
                               str(here/'pioneer_q38_synthetic_embedding_bridge.cpp')],capture_output=True,text=True)
        assert result.returncode and 'Synthetic embedding bytes require their isolated host-test implementation' in result.stderr
print('PASS synthetic recurrent matrix source role isolation; NO DEVICE')
