#!/usr/bin/env python3
"""Bind the guarded v4 diagnostic to an observed stable generation. No I/O to the board."""
from hashlib import sha256
from pathlib import Path
import re
import sys

from build_qwen38_head_csr_trial import bind_raw_pair

PACKAGE={
    'candidate': ('2026111700010213',
        '4b6f208da0ed2960d7d3e9a6617f3e354737b9ec4b7b7c4cf0c62d25a3fbd9ec',
        '5c650f81142f5fc38c8f6ac24cd62ef9c4136b65144a750c47a8c8a01af148f6',
        'ec90ba4575b78457c64d72a43846346b64923dc16e7aee1e1b6a808361a37889'),
    'live': ('2026111700010214',
        'dacf779941dfdb94abe2ef5391519896b641a5812b43b7d78eb88cd4a274f2fc',
        '6ccb9f48445671ba9845a4c4cf5f4c3d58e773c72bf946c9f5be0232e8f78dc1',
        'c334f4e5ea0f3a2f3f02e5acee5a7126e96b401212d022b746a55922a7c5741b'),
    'recipe': ('2026111700010216',
        '791a898258f757677e9928f591e9095f24f49c61d19a840e693d1a68ff1af913',
        '59711d94ad957ffd792330597d38f45385addf9e6673c86afdb35edfab3ca9e7',
        'ad5d23bc5baf35773f9745290e60659376d56fc9671f1d1a14c15a0742506f94'),
}
RECIPE_MANIFEST='0293066d8d872861bb3150a736e45e89d51f1ffece9bccb30cfbef40e663952f'
LIVE_MANIFEST='d2666936f624f65b6eeaeab9a42776943aef88b62fdfb1f5c8f4414be2ac4d7a'
POSTFLIGHT=Path(__file__).with_name('pioneer_q38_layer_split_postflight.sh')
POSTFLIGHT_SHA='22242231188928d8277e8aa37833d78cbb07673a7c207b8c0d9cc050af06f375'


def bind(start,live=False,*,recipe=False):
    assert type(live) is bool and type(recipe) is bool and not (live and recipe)
    variant='recipe' if recipe else 'live' if live else 'candidate'
    return bind_raw_pair(variant,start,PACKAGE,'layer-split','layer split profile','LAYER_SPLIT')


def postflight(start,parent,variant):
    """Read only; use the actual parent identity after its observed exit."""
    if variant not in ('recipe','live'):
        raise ValueError('unknown postflight role')
    runner=bind(start,live=variant=='live',recipe=variant=='recipe')
    if not re.fullmatch(r'[1-9][0-9]{0,8}',parent):
        raise ValueError('supply the observed positive detached parent team')
    template=POSTFLIGHT.read_bytes()
    if sha256(template).hexdigest()!=POSTFLIGHT_SHA:
        raise ValueError('the original postflight changed; review it before rebinding')
    text=template.decode('ascii')
    generation=int(start)+2
    control,candidate=PACKAGE['candidate'],PACKAGE[variant]
    tag,manifest=(216,RECIPE_MANIFEST) if variant=='recipe' else (214,LIVE_MANIFEST)
    changes=[
        ('tag213 generation3',f'tag{tag}'),
        ('layer-split-candidate-213',f'layer-split-{variant}-{tag}'),
        ('STABLE041_GEN4_RESTORED',f'STABLE041_GEN{generation}_RESTORED'),
        ("printf '621\\n' | cmp -s launch.pid -",f"printf '{parent}\\n' | cmp -s launch.pid -"),
        ('$1==621 ||',f'$1=={parent} ||'),
        (control[1],candidate[1]),(control[2],candidate[2]),(control[3],candidate[3]),
        ('3e3f76f2b54a22d1ab3fbfda0799680cd554b09f4fa44c2b6023d29383e804b2',manifest),
        ('1e7cdd1238593c7b317ea725e722574ed600dea10dc0b4dc9ee19d931f82e01c',
         sha256(runner.encode('ascii')).hexdigest()),
        ("test \"$status\" = 'P100A_RELOAD driver55 tag2026102100010012 generation4 ",
         f"printf '%s\\n' \"$status\" | /bin/grep -Eq '^P100A_RELOAD driver[1-9][0-9]* tag2026102100010012 generation{generation} "),
        ("guard1;CPU_METADATA_ONLY'","guard1;CPU_METADATA_ONLY$'"),
    ]
    for old,new in changes:
        assert text.count(old)==1,old
        text=text.replace(old,new,1)
    return text


def recipe_postflight(start,parent):
    return postflight(start,parent,'recipe')


def main():
    live=len(sys.argv)==4 and sys.argv[3]=='--live-image-check'
    recipe=len(sys.argv)==4 and sys.argv[3]=='--recipe-batch'
    closure=len(sys.argv)==5 and sys.argv[3] in ('--recipe-postflight','--live-postflight')
    if not __debug__ or (len(sys.argv)!=3 and not (live or recipe or closure)):
        raise SystemExit('use OBSERVED_STABLE_GENERATION NEW_SCRIPT '
                         '[--live-image-check|--recipe-batch|--recipe-postflight PARENT|--live-postflight PARENT]')
    generation,path=sys.argv[1:3]
    text=postflight(generation,sys.argv[4],sys.argv[3].split('-')[2]) if closure else bind(generation,live,recipe=recipe)
    output=text.encode('ascii')
    with Path(path).open('xb') as dest:
        dest.write(output)
    print(sha256(output).hexdigest(),path)


if __name__=='__main__':
    main()
