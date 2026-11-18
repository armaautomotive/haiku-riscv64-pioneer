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
    'wire': ('2026111700010217',
        '3303735cb89298c7b7b4be70b0133965923375cfce135f456fbc8ff4757ae749',
        '344be63e6b0214d8cb02a98d77884b0f671cbd81dd335627b9f6958eb102f407',
        '383d1a3057f458635f899f9a48fe1eb3e255d6b4d530175c69f3080553ea3f9b'),
    'head-raw': ('2026111700010218',
        '3cc769d364ccb337c088d47129567b6b568aaf017ae57e4d077870a3962f1060',
        'b9429117551145489d1877d5f92ac90ca1e265a974fd5434615ab8d3db63d9b2',
        '8bf18b80f86e1d2ed3581c2ac93d1990fc15d903c4352a6e7c40ea5484b85f16'),
    'head-wire': ('2026111700010219',
        'b81b275f6a9b564b30ebf2246d87439f8d46427243bf587d962d8f8493e83a17',
        '6970a0cef948abbcb2f6fe5d6def306dfb2073cfd7b7edc60c5186279f50c642',
        'ca317de33c616f79b6a013b69624869be66ad4f4ce495ffc3f693f4e647fb274'),
    'observation': ('2026111700010224',
        '76f7448d3d4fe29247c032e84f40ee5393ee3e0203757edd2daeb49d2172d3b2',
        'fc50b50fa8c34d938e26e93cdb91cfd79fe540e71112844c57601bb517c65815',
        'bce683e79be35e5b89d5e172df2e2551d197b5eee4738c5e9d4c73108af38ee5'),
}
RECIPE_MANIFEST='0293066d8d872861bb3150a736e45e89d51f1ffece9bccb30cfbef40e663952f'
LIVE_MANIFEST='d2666936f624f65b6eeaeab9a42776943aef88b62fdfb1f5c8f4414be2ac4d7a'
WIRE_MANIFEST='75603557a948fca8ac7f5efea288930ae0c0e6811a175b9fd9b521616881834a'
OBSERVATION_MANIFEST='343247c17d5442c21bb7ff088d7a8bf76f2824bac819d56b02c97659f90c845f'
HEAD_MANIFEST={'raw':'ef960d3ba6126f190efe6d7ffa03c2fe7e15702638d5356ee8c9265d7514f79a',
               'wire':'bbb8c18cff9259b464c961efae83c42b42c78a8619f651341ac62fb29adcb38f'}
POSTFLIGHT=Path(__file__).with_name('pioneer_q38_layer_split_postflight.sh')
POSTFLIGHT_SHA='22242231188928d8277e8aa37833d78cbb07673a7c207b8c0d9cc050af06f375'


def bind(start,live=False,*,recipe=False,wire=False,head=None,observation=False):
    assert all(type(v) is bool for v in (live,recipe,wire,observation)) and head in (None,'raw','wire')
    assert sum((live,recipe,wire,observation,head is not None))<=1
    if wire or head or observation:
        if head:
            text=bind_raw_pair(head,start,{v:PACKAGE['head-'+v] for v in HEAD_MANIFEST},
                               'profile-head','v4 head batch','PROFILE_HEAD')
        elif observation:
            text=bind_raw_pair('observation',start,PACKAGE,'native-q8','native Q8','NATIVE_Q8_OBSERVATION')
        else:
            text=bind_raw_pair('wire',start,PACKAGE,'native-q8','native Q8','NATIVE_Q8')
        manifest=HEAD_MANIFEST[head] if head else OBSERVATION_MANIFEST if observation else WIRE_MANIFEST
        old='\texact "$candidate" "$candidateHash" || fail candidate_driver'
        assert text.count(old)==1
        return text.replace(old,old+f'\n\texact "$stage/manifest.json" {manifest} || fail manifest',1)
    variant='recipe' if recipe else 'live' if live else 'candidate'
    return bind_raw_pair(variant,start,PACKAGE,'layer-split','layer split profile','LAYER_SPLIT')


def postflight(start,parent,variant):
    """Read only; use the actual parent identity after its observed exit."""
    manifests={'recipe':RECIPE_MANIFEST,'live':LIVE_MANIFEST,'wire':WIRE_MANIFEST,'observation':OBSERVATION_MANIFEST,
               **{'head-'+v:manifest for v,manifest in HEAD_MANIFEST.items()}}
    if variant not in manifests:
        raise ValueError('unknown postflight role')
    head=variant.removeprefix('head-') if variant.startswith('head-') else None
    runner=bind(start,live=variant=='live',recipe=variant=='recipe',wire=variant=='wire',head=head,
                observation=variant=='observation')
    if not re.fullmatch(r'[1-9][0-9]{0,8}',parent):
        raise ValueError('supply the observed positive detached parent team')
    template=POSTFLIGHT.read_bytes()
    if sha256(template).hexdigest()!=POSTFLIGHT_SHA:
        raise ValueError('the original postflight changed; review it before rebinding')
    text=template.decode('ascii')
    generation=int(start)+2
    control,candidate=PACKAGE['candidate'],PACKAGE[variant]
    tag,manifest=candidate[0][-3:],manifests[variant]
    stage=(f'profile-head-{head}-{tag}' if head else f'native-q8-{variant}-{tag}' if variant in ('wire','observation')
           else f'layer-split-{variant}-{tag}')
    changes=[
        ('tag213 generation3',f'tag{tag}'),
        ('layer-split-candidate-213',stage),
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
    wire=len(sys.argv)==4 and sys.argv[3]=='--native-q8-wire'
    observation=len(sys.argv)==4 and sys.argv[3]=='--native-q8-observation'
    head={'--head-batch-v4':'raw','--native-q8-head-batch-v4':'wire'}.get(sys.argv[3]) if len(sys.argv)==4 else None
    variants={'--'+v+'-postflight':v for v in ('recipe','live','wire','head-raw','head-wire','observation')}
    closure=len(sys.argv)==5 and sys.argv[3] in variants
    if not __debug__ or (len(sys.argv)!=3 and not (live or recipe or wire or observation or closure or head)):
        raise SystemExit('use OBSERVED_STABLE_GENERATION NEW_SCRIPT '
                         '[--live-image-check|--recipe-batch|--native-q8-wire|--native-q8-observation|--head-batch-v4|--native-q8-head-batch-v4|'
                         +'|'.join(k+' PARENT' for k in variants)+']')
    generation,path=sys.argv[1:3]
    text=postflight(generation,sys.argv[4],variants[sys.argv[3]]) if closure else bind(
        generation,live,recipe=recipe,wire=wire,head=head,observation=observation)
    output=text.encode('ascii')
    with Path(path).open('xb') as dest:
        dest.write(output)
    print(sha256(output).hexdigest(),path)


if __name__=='__main__':
    main()
