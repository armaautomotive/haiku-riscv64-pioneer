#!/bin/sh
# Guarded development A/B exchange,NOT physical PCIe hotplug or cold recovery.
# Candidates pin their complete qualified/offline-tested code. No force/reset/unload
# command and NO automatic rollback after an unproven replacement.
set -eu
if [ "$#" = 1 ] && [ "$1" = --apply-raw-unpack-cache-qualification ]; then
 echo 'STOP_RAW_CACHE_FAILED_NATIVE:tag22 Q4 completion failed;NO install/retry/reset/automatic rollback' >&2;exit 2
fi
if [ "$#" = 1 ] && [ "$1" = --apply-recurrent-block-validation ]; then
 echo 'STOP_RECURRENT71_FAILED_NATIVE:worker114 startup failed;use separately qualified72 ONLY;NO install/retry/reset/automatic rollback' >&2;exit 2
fi
[ "$#" = 1 ] && [ "$(uname -s)" = Haiku ] || exit 2
case "$1" in
 --apply-postscale-delta|--restore-postscale-delta) mode=$1;;
 --apply-gdn-zero-diagnostic|--restore-gdn-zero-diagnostic) mode=$1;;
 --apply-resident-root-diagnostic|--restore-resident-root-diagnostic) mode=$1;;
 --apply-root-cycle-diagnostic|--restore-root-cycle-diagnostic) mode=$1;;
 --apply-block-affine-qualification|--restore-block-affine-qualification) mode=$1;;
 --apply-scale-reuse-qualification|--restore-scale-reuse-qualification) mode=$1;;
 --apply-group32-qualification|--restore-group32-qualification) mode=$1;;
 --apply-native-q8-qualification|--restore-native-q8-qualification) mode=$1;;
 --apply-affine-rawdot-qualification|--restore-affine-rawdot-qualification) mode=$1;;
 --apply-direct-srcb-qualification|--restore-direct-srcb-qualification) mode=$1;;
 --apply-sentinel-poll-qualification|--restore-sentinel-poll-qualification) mode=$1;;
 --apply-graph-csr-diagnostic|--restore-graph-csr-diagnostic) mode=$1;;
 --apply-ffn-code-diagnostic|--restore-ffn-code-diagnostic) mode=$1;;
 --apply-ffn-peer-lifetime-diagnostic|--restore-ffn-peer-lifetime-diagnostic) mode=$1;;
 --apply-ffn-root-lifetime-diagnostic|--restore-ffn-root-lifetime-diagnostic) mode=$1;;
 --apply-ffn-model-init|--restore-ffn-model-init) mode=$1;;
 --apply-ffn-native-graph|--restore-ffn-native-graph) mode=$1;;
 --apply-ffn-host-profile|--restore-ffn-host-profile) mode=$1;;
 --apply-ffn-sealed-proof|--restore-ffn-sealed-proof) mode=$1;;
 --apply-embedding-validation|--restore-embedding-validation) mode=$1;;
 --apply-rms-validation|--restore-rms-validation) mode=$1;;
 --apply-projection-validation|--restore-projection-validation) mode=$1;;
 --apply-decoder-validation|--restore-decoder-validation) mode=$1;;
 --apply-gdn-input-validation|--restore-gdn-input-validation) mode=$1;;
 --apply-gdn-recurrent-validation|--restore-gdn-recurrent-validation) mode=$1;;
 --apply-gdn-rendezvous-validation|--restore-gdn-rendezvous-validation) mode=$1;;
 --apply-conv-silu-precision-validation|--restore-conv-silu-precision-validation) mode=$1;;
 --apply-resident-output-validation|--restore-resident-output-validation) mode=$1;;
 --apply-resident-layer-validation|--restore-resident-layer-validation) mode=$1;;
 --apply-mixed-live-validation|--restore-mixed-live-validation) mode=$1;;
 --apply-recurrent-block-validation|--restore-recurrent-block-validation) mode=$1;;
 --apply-recurrent-deadline-validation|--restore-recurrent-deadline-validation) mode=$1;;
 --apply-recurrent-prefix-validation|--restore-recurrent-prefix-validation) mode=$1;;
 --apply-token-validation|--restore-token-validation) mode=$1;;
 --apply-token-hold-recheck|--restore-token-hold-recheck) mode=$1;;
 --apply-token-root-frontend|--restore-token-root-frontend) mode=$1;;
 --apply-token-hidden-readout|--restore-token-hidden-readout) mode=$1;;
 --apply-host-phase-diagnostic|--restore-host-phase-diagnostic) mode=$1;;
 --apply-batch4-fetch-qualification|--restore-batch4-fetch-qualification) mode=$1;;
 --apply-batch-raw-qualification|--restore-batch-raw-qualification) mode=$1;;
 --apply-frontend-qualification|--restore-frontend-qualification) mode=$1;;
 --restore-raw-unpack-cache-qualification) mode=$1;;
 --apply-tile-prefetch-qualification|--restore-tile-prefetch-qualification) mode=$1;;
 --apply-command-reuse-qualification|--restore-command-reuse-qualification) mode=$1;;
 --apply-peer-phase-qualification|--restore-peer-phase-qualification) mode=$1;;
 --apply-q4-nibble-qualification|--restore-q4-nibble-qualification) mode=$1;;
 --apply-q4-direct-qualification|--restore-q4-direct-qualification) mode=$1;;
 --apply-local-q4-qualification|--restore-local-q4-qualification) mode=$1;;
 --apply-sparse-profile|--restore-sparse-profile|--apply-sparse-export|--restore-sparse-export|--apply-normal-clock|--restore-normal-clock|--apply-input-sum-cache|--restore-input-sum-cache|--apply-zero-minimum|--restore-zero-minimum|--apply-native-operand|--restore-native-operand|--apply-compact-output|--restore-compact-output|--apply-compact-profile|--restore-compact-profile|--apply-deferred-affine|--restore-deferred-affine|--apply-ordered-extents|--restore-ordered-extents|--apply-inline-config|--restore-inline-config) mode=$1;;
 --bootstrap|--apply-a|--apply-b|--apply-profile|--restore-profile|--apply-batch|--restore-batch|--apply-compact|--restore-compact|--apply-fast|--restore-fast|--apply-reuse|--restore-reuse|--apply-cross-reuse|--restore-cross-reuse|--apply-work|--restore-work|--apply-work-profile|--restore-work-profile|--apply-work-posted|--restore-work-posted|--apply-work-cohort|--restore-work-cohort|--apply-work-q4|--restore-work-q4|--apply-work-q4-inline|--restore-work-q4-inline|--apply-work-span|--restore-work-span|--apply-work-fast-span|--restore-work-fast-span|--apply-work-span-code-reuse|--restore-work-span-code-reuse|--apply-work-int-sfpu|--restore-work-int-sfpu|--apply-work-pipelined|--restore-work-pipelined|--apply-work-static-vector|--restore-work-static-vector|--apply-work-cached-affine|--restore-work-cached-affine|--apply-work-replay-operand|--restore-work-replay-operand|--apply-work-dot|--restore-work-dot|--apply-work-output-dma|--restore-work-output-dma|--apply-work-queued-entry|--restore-work-queued-entry|--apply-local-delta|--restore-local-delta|--apply-local-delta-terminal|--restore-local-delta-terminal|--apply-current-profile|--restore-current-profile) mode=$1;;
 *) exit 2;;
esac
stage=/boot/home/pioneer-p100a-hot-reload-20261019-a
installed=/boot/home/config/non-packaged/add-ons/kernel/drivers/bin/tenstorrent_pci_probe
device=/boot/home/config/non-packaged/add-ons/kernel/drivers/dev/misc/tenstorrent_pci_probe
guard=/boot/home/config/non-packaged/add-ons/kernel/generic/pioneer_p100a_reload_guard
client=$stage/pioneer-p100a-reload
candidatea=$stage/tenstorrent_pci_probe-hot-a
candidateb=$stage/tenstorrent_pci_probe-hot-b
candidatep=$stage/tenstorrent_pci_probe-profile-c43d21c8
candidateq=$stage/tenstorrent_pci_probe-batch-3cfa6513
candidatec=$stage/tenstorrent_pci_probe-compact-dc564559
candidatef=$stage/tenstorrent_pci_probe-compact-fast-30423874
candidater=$stage/tenstorrent_pci_probe-reuse-cac152f0
candidatex=$stage/tenstorrent_pci_probe-cross-reuse-f31c8385
candidatew=$stage/tenstorrent_pci_probe-work-a2a7179a
candidatewp=$stage/tenstorrent_pci_probe-work-profile-abacb9a2
candidatews=$stage/tenstorrent_pci_probe-work-posted-2d7f3e9f
candidatewc=$stage/tenstorrent_pci_probe-work-cohort-a60fadcc
candidatew4=$stage/tenstorrent_pci_probe-work-q4-3c1592d3
candidatew4i=$stage/tenstorrent_pci_probe-work-q4-inline-54c3d8db
candidatewsp=$stage/tenstorrent_pci_probe-work-span-e44c3e3c
candidatewspf=$stage/tenstorrent_pci_probe-work-fast-span-6dcf1e71
candidatewspc=$stage/tenstorrent_pci_probe-work-span-code-reuse-aaded0f7
candidatewspi=$stage/tenstorrent_pci_probe-work-int-sfpu-1b677ddf
candidatewspq=$stage/tenstorrent_pci_probe-work-pipelined-4ac8fa1d
candidatewsps=$stage/tenstorrent_pci_probe-work-static-vector-0590eaa2
candidatewspa=$stage/tenstorrent_pci_probe-work-cached-affine-5b7562af
candidatewspu=$stage/tenstorrent_pci_probe-work-replay-operand-388e17ab
candidatewspd=$stage/tenstorrent_pci_probe-work-dot-bdbecc33
candidatewspo=$stage/tenstorrent_pci_probe-work-output-dma-5f5f8cd8
candidatewspj=$stage/tenstorrent_pci_probe-work-queued-entry-a9889c0b
candidatewspv=$stage/tenstorrent_pci_probe-local-delta-16777849
candidatewspg=$stage/tenstorrent_pci_probe-local-delta-terminal-e903b8fc
candidatecurrentprofile=$stage/tenstorrent_pci_probe-current-profile-9f1ed2e0
candidate_sparse_profile=$stage/tenstorrent_pci_probe-sparse-export-profile-8685c971
candidate_sparse_export=$stage/tenstorrent_pci_probe-sparse-export-b852456b
candidate_normal_clock=$stage/tenstorrent_pci_probe-normal-clock-b13e72d3
candidate_sum_cache=$stage/tenstorrent_pci_probe-input-sum-cache-6efd1dd7
candidate_zero_minimum=$stage/tenstorrent_pci_probe-zero-minimum-16a06fee
candidate_native_operand=$stage/tenstorrent_pci_probe-native-operand-93067b8f
candidate_compact_output=$stage/tenstorrent_pci_probe-compact-output-7cbd2bf4
candidate_compact_profile=$stage/tenstorrent_pci_probe-compact-profile-f7d3d395
candidate_deferred_affine=$stage/tenstorrent_pci_probe-deferred-affine-202389d0
candidate_ordered_extents=$stage/tenstorrent_pci_probe-ordered-extents-66936bf6
candidate_inline_config=$stage/tenstorrent_pci_probe-inline-config-03876197
candidate_postscale_delta=$stage/tenstorrent_pci_probe-postscale-delta-dcc77864
candidate_gdn_zero_diagnostic=$stage/tenstorrent_pci_probe-gdn-zero-diagnostic-8ccb2a24
candidate_resident_root_diagnostic=$stage/tenstorrent_pci_probe-resident-root-diagnostic-213ebf0a
candidate_root_cycle_diagnostic=$stage/tenstorrent_pci_probe-root-cycle-diagnostic-52ea41c9
candidate_block_affine_qualification=$stage/tenstorrent_pci_probe-block-affine-qualification-c8cbbc99
candidate_scale_reuse_qualification=$stage/tenstorrent_pci_probe-scale-reuse-qualification-041fef18
candidate_group32_qualification=$stage/tenstorrent_pci_probe-group32-qualification-210946ad
candidate_native_q8_qualification=$stage/tenstorrent_pci_probe-native-q8-qualification-e546b664
candidate_affine_rawdot_qualification=$stage/tenstorrent_pci_probe-affine-rawdot-qualification-8b43f2c7
candidate_direct_srcb_qualification=$stage/tenstorrent_pci_probe-direct-srcb-qualification-7056d20e
candidate_sentinel_poll_qualification=$stage/tenstorrent_pci_probe-sentinel-poll-qualification-84ab80ac
candidate_graph_csr_diagnostic=$stage/tenstorrent_pci_probe-graph-csr-diagnostic-3fce7a8a
candidate_ffn_code_diagnostic=$stage/tenstorrent_pci_probe-ffn-code-diagnostic-6e963d80
candidate_ffn_peer_lifetime_diagnostic=$stage/tenstorrent_pci_probe-ffn-peer-lifetime-diagnostic-b35c00fd
candidate_ffn_root_lifetime_diagnostic=$stage/tenstorrent_pci_probe-ffn-root-lifetime-diagnostic-aa6730ef
candidate_ffn_model_init=$stage/tenstorrent_pci_probe-ffn-model-init-5361c335
candidate_ffn_native_graph=$stage/tenstorrent_pci_probe-ffn-native-graph-453ebdf1
candidate_ffn_host_profile=$stage/tenstorrent_pci_probe-ffn-host-profile-eb6ba274
candidate_ffn_sealed_proof=$stage/tenstorrent_pci_probe-ffn-sealed-proof-23e2e68a
candidate_embedding_validation=$stage/tenstorrent_pci_probe-embedding-validation-779a2c81
candidate_rms_validation=$stage/tenstorrent_pci_probe-rms-validation-a32f462c
candidate_projection_validation=$stage/tenstorrent_pci_probe-projection-validation-a2bcfb34
# New diagnostic build ONLY. Immutable ae05ef96 restores previous4db
# AFTER physical cold recovery;older3648a427 maps d968 ONLY. Both staged
# files stay untouched. This new mapping cannot recover/quiesce the
# currently quarantined card or restore either previous same-tag image.
candidate_decoder_validation=$stage/tenstorrent_pci_probe-decoder-validation-6ce7b4fe
candidate_gdn_input_validation=$stage/tenstorrent_pci_probe-gdn-input-validation-32a661c4
candidate_gdn_recurrent_validation=$stage/tenstorrent_pci_probe-gdn-recurrent-validation-b43a6cc7
candidate_gdn_rendezvous_validation=$stage/tenstorrent_pci_probe-gdn-rendezvous-validation-cdacaa3a
candidate_conv_silu_precision_validation=$stage/tenstorrent_pci_probe-conv-silu-precision-validation-fb995b39
candidate_resident_output_validation=$stage/tenstorrent_pci_probe-resident-output-validation-58064ede
# Frontend-observation revision of role69. A numeric tag is NOT byte
# identity. Immutable helpers0d0f3f9c/7cb6e492 still own beeaf/9609 restoration
# AFTER fresh recovery;this new mapping cannot recover a retained owner.
candidate_resident_layer_validation=$stage/tenstorrent_pci_probe-resident-layer-validation-b554d37d
candidate_mixed_live_validation=$stage/tenstorrent_pci_probe-mixed-live-validation-11d24d92
candidate_recurrent_block_validation=$stage/tenstorrent_pci_probe-recurrent-block-validation-87d0c4e0
candidate_recurrent_deadline_validation=$stage/tenstorrent_pci_probe-recurrent-deadline-validation-3e00974c
candidate_recurrent_prefix_validation=$stage/tenstorrent_pci_probe-recurrent-prefix-validation-4ae7b6ea
candidate_token_validation=$stage/tenstorrent_pci_probe-token-validation-d7846405
# Same diagnostic role/tag, DIFFERENT control-code bytes. Keep the old
# candidate and its immutable restoration helpers separate and untouched.
candidate_token_hold_recheck=$stage/tenstorrent_pci_probe-token-hold-recheck-781ab933
# Current boot/receipt/code seal;the previous two tag74 images and their
# immutable restore helpers retain their OWN exact hash-qualified routes.
candidate_token_root_frontend=$stage/tenstorrent_pci_probe-token-root-frontend-5f2fc63d
candidate_token_hidden_readout=$stage/tenstorrent_pci_probe-token-hidden-readout-b74f4bfa
candidate_host_phase_diagnostic=$stage/tenstorrent_pci_probe-host-phase-diagnostic-acf33ea4
candidate_batch4_fetch_qualification=$stage/tenstorrent_pci_probe-batch4-fetch-qualification-2fcf0b27
candidate_batch_raw_qualification=$stage/tenstorrent_pci_probe-batch-raw-qualification-c38e258c
candidate_frontend_qualification=$stage/tenstorrent_pci_probe-frontend-qualification-f6998083
candidate_raw_unpack_cache_qualification=$stage/tenstorrent_pci_probe-raw-unpack-cache-qualification-03001d15
candidate_tile_prefetch_qualification=$stage/tenstorrent_pci_probe-tile-prefetch-qualification-d3445b26
candidate_command_reuse_qualification=$stage/tenstorrent_pci_probe-command-reuse-qualification-5640dd3d
candidate_peer_phase_qualification=$stage/tenstorrent_pci_probe-peer-phase-qualification-e2a98809
candidate_q4_nibble_qualification=$stage/tenstorrent_pci_probe-q4-nibble-qualification-0f7213d9
candidate_q4_direct_qualification=$stage/tenstorrent_pci_probe-q4-direct-qualification-37b36f90
candidate_local_q4_qualification=$stage/tenstorrent_pci_probe-local-q4-qualification-d714a8b1
guardsource=$stage/pioneer_p100a_reload_guard
incoming=$stage/driver.incoming
guardincoming=$stage/guard.incoming
original=9ecb313bd669df9b6e80100586a0bc3b092a490c20ea0bf426ef41ab27689aea
hasha=77d0f396433b229eb5937b633e1a57f183b85612ae1f7503b2b5bbd48e261679
hashb=67f2b14bd8a0a17c934e51a4952949223e1e0bddbb3e7a29f2664cb0a9f568c9
hashp=c43d21c84b7fd1946343268d4cd413987cd666c07ef40a4b8c83d4094a89a2d6
hashq=3cfa65134ad584fbd716593b7beec592735336f6d8887be29e92133576fe1fe5
hashc=dc564559106418fc9c68f070a9dae41df3a41e0aec801800c8198fa1a9aca08e
hashf=30423874c662e57ea29328b376edb03ca923af1a0e4073ee52a3138b45e71aa2
hashr=cac152f0b4560acb9e48f1f104870226114b419e7a1399b4ca2d0c34799aae25
hashx=f31c83854eef0f922c5e156394615853a30d0a3a73116a3cd763d66e36a321e1
hashw=a2a7179add3c5bf5eb06e0ef8cd063a3113169232a7e0445ba8e2d5b06daf683
hashwp=abacb9a24ad02a59dd2bdca0752234a13ec2010a1539c21e8721ddaa1d989b62
hashws=2d7f3e9f102d0e23a61a85f0651ee6c59337e92e44b9517965fe3f27eeddc3ac
hashwc=a60fadccaec7a3349ee75c08e3c2ecaa75f97550d2dae6a3ab495ae424253b58
hashw4=3c1592d3dade9b5f9276c232f56281308987ddefdf946344d1ab9efaef1539ff
hashw4i=54c3d8db6cdb21868fa08109c6a8be7139f2a6f649df6a133f2a5be934688182
hashwsp=e44c3e3c69a37a04639ae9e816bd80fa043be531752b88738632aef96d092929
hashwspf=6dcf1e7179e8b0b35c530cad389e7e3b4e9e9ef29d087b1e7c5d8bd8d31c156b
hashwspc=aaded0f7924236a4a6995d58174587f92165520d829c2b33a811aa18476f4137
hashwspi=1b677ddfcd4383a2c74edf2652b51dc498c9775801d7a87967eacd9f5b52c53e
hashwspq=4ac8fa1dff682b2c43b3ac53127818445a8cc4d21d27574819e167d0c872a88e
hashwsps=0590eaa2b1c1e9aa9cadd9736a56803d4d8ce21f74f199fe104444c3fdca8af3
hashwspa=5b7562af537609b34e5286bebb4bbd41ab6dc9e376072820ca2796b651928210
hashwspu=388e17ab9ab2701fb900c3a3c39b792bf5a65917a26b4520ddfa0d870f105709
hashwspd=bdbecc3318006a4a3e5ec99d141cb26eb5c2c722b3704ed6b7a01001ce97e352
hashwspo=5f5f8cd82f71ea38842b1fa2c2d5321e53641e69d7090f9c2c25166f3da9de8d
hashwspj=a9889c0b5c7ea0b1281adee641fef0390f3a0ff523080bb05733d335eaf4456b
hashwspv=16777849886e7dfd6ada975f0a5c5db7966dbe580dd4e3d11eaaa90cbbe2914d
hashwspg=e903b8fcee619edf4e40457aed16e0387286a73c7606b4796e5b3961b275e498
hashcurrentprofile=9f1ed2e04c1555d212da47590538832b0c77c8480d26303546016846352e554b
hash_sparse_profile=8685c9711be12b35948a7cf8c5b4533323f16869f6b10bf1cb1da9f75882d4cf
hash_sparse_export=b852456b1bf116575153f99d6358df3b57c4d4c9b956e7882453c44006df855c
hash_normal_clock=b13e72d3fd50b9dd94a13c344b337d9b12bb07d003e0f40a2f14f1ab6015dcca
hash_sum_cache=6efd1dd7ec1b9e4268c29565e767a331c542c8aedcc3f7f61b1fc3315fb84bc8
hash_zero_minimum=16a06fee2117c8ad51086343aa3f05997b6ec9f66961e21704012ae2846ae77a
hash_native_operand=93067b8f01c3c539838b0cea6c390fc904efd802ee1ab2fe7d25838cbb816447
hash_compact_output=7cbd2bf43fa299e1296f01df7e08d41f810cb6d5bae6959d05d4de6f06fc1ec5
hash_compact_profile=f7d3d3954696d4a633483e1d23e1acf565d05240c049f4369e820679e6a6c331
hash_deferred_affine=202389d0a75f07b0ccf40930c94121a3f2094d71d2c17d7bf7017d7ab0433422
hash_ordered_extents=66936bf67f56bee3657bd90df16ed63a89c0be262264ecee2d8ed97a81f849d3
hash_inline_config=038761972b9d53912ec24e38ac01a7f5185e291f0c73e555cdcef60d8bd9ce87
hash_postscale_delta=dcc7786475170d1eb2912439c1989b5b0aa19d903a1bb8e97881b4e500043ef7
hash_gdn_zero_diagnostic=8ccb2a24c7afeca60625c241a81cffa23722c8f5fce341fb5a3a6106fa69e962
hash_resident_root_diagnostic=213ebf0aadd6a78eac32a6cc35d9fe83cb4546dbd3cc54eb224d551557a92831
hash_root_cycle_diagnostic=52ea41c91681353f20d4ce67bc0a0e338a0750aa0ab3f46bcd89e74a558a0424
hash_block_affine_qualification=c8cbbc992fd16862dc60b5f2f0dfdd5766fa588dca8b0499e75bdd6f92221f71
hash_scale_reuse_qualification=041fef189829c58791a7e4a86e2b15b182fa1a6c9cc9f03a91037ba15b353403
hash_group32_qualification=210946ad9f329ea2341e16519421e899ae09d41d1ad56fb1f3789bc77af4554d
hash_native_q8_qualification=e546b6643009018386ff57fceee7c8d425d81a3d8211529e5fc393719ea053d4
hash_affine_rawdot_qualification=8b43f2c7145529fb0e5e6f0e3f4eac431b182db0ea16ddda63d713c7bb83cba0
hash_direct_srcb_qualification=7056d20e09eee61c080c6c900f1cefde92026d000af53f2931d5e1d0ff9ebcb1
hash_sentinel_poll_qualification=84ab80ac87ce3486c4194be6743b44c56492cd7282e934d966e4ee39d48af296
hash_graph_csr_diagnostic=3fce7a8ab03187433ec41a6c8d34f4f00be3d06843a67782ca1a2352a1e332a8
hash_ffn_code_diagnostic=6e963d80d55a79d36c67eb54f8fbe46debf239617c0d06cb9d3be68f1b1c8a76
hash_ffn_peer_lifetime_diagnostic=b35c00fdbdd03f4870b419a1ed4548c12d164671a2209cbd228a4d873dd414ff
hash_ffn_root_lifetime_diagnostic=aa6730ef8157e99e39912acbdef7dea6010eaa5d5d44763b3d88b9fee7e5b577
hash_ffn_model_init=5361c3351131ffff6b2aad399432a6cd53eda843c36c69db761961a6717b75bf
hash_ffn_native_graph=453ebdf12fa13bd39d31c3ec9f01c17e7048946e5cd575f663055b6e49c8f1e1
hash_ffn_host_profile=eb6ba2741c8904dba8b4902ac260934ea2e169ffeb16ab8a3e96bd5e11966ba4
hash_ffn_sealed_proof=23e2e68a33c16f990552bb9bf146252a0ce9d23a4a3c3ec9f7f751c8dba77483
hash_embedding_validation=779a2c81080b5cd7ca891f3614ea5c02ef580d769023b90f40c3296db64ea514
hash_rms_validation=a32f462cbfdbe8558aca780a526e44a59dea15b25489a0131a5c4803771c276a
hash_projection_validation=a2bcfb34baeba0bdca4e029164901c874c27ee7c3494be62912711e7a0c164e9
hash_decoder_validation=6ce7b4fecae927391119163022cb49f8775ca79776555d209e2eb4c3eacc564b
hash_gdn_input_validation=32a661c4d01cf1aebaed1edd5399bd0748bb37f5188d84a07cf8a7476cb3e47c
hash_gdn_recurrent_validation=b43a6cc70dc8b5d1f5d69d1b9207fbf577a9671881963da45854b0cd68a7f89e
hash_gdn_rendezvous_validation=cdacaa3a971823cbc5bbbebde8bd2c3771d48842df814729b5fe67c4dc290759
hash_conv_silu_precision_validation=fb995b39401f731259ce8154bdd77981b152b8038418728daf2cc420b247ecc6
hash_resident_output_validation=58064edee490bb5335fa135122a8d89588987a56c0451d751bccf9ebbe84ff3d
hash_resident_layer_validation=b554d37d0e74204a2ab9e521c6f535ea7c697879295ff2d418def6afabe9f8c7
hash_mixed_live_validation=11d24d92c9ee05f1e3664ee13796f8621f378bb738eb5c62b622314f846a3e29
hash_recurrent_block_validation=87d0c4e005d7df937e22c2806051cd0aa99d736c0fb2c0199fde40d8eb1d7cb4
hash_recurrent_deadline_validation=3e00974c4476f2755341fbcea034340652e976e292cb017344f4a70f896c652e
hash_recurrent_prefix_validation=4ae7b6ea4217c0ee45303aab37e6793c9c17fe18a2acbbece8a7556332f92a3c
hash_token_validation=d78464058dec32de49e67c5263fca290751acdaf4252a7785ce56b9654e63aed
hash_token_hold_recheck=781ab93300d1a8418cd752ae0728090c9e2c470a53320eafe22e4f3d3b8706d2
hash_token_root_frontend=5f2fc63de2c4413d54b7087e5b923113246bc7a7f09747d0d79915bf950bf039
hash_token_hidden_readout=b74f4bfa5583aaf0cf8272d5136c73336c1db0f93e168e7212277f97705a92cc
hash_host_phase_diagnostic=acf33ea4279ca6d36cbc4b90ae56e6da1fed219b431ae7c80c2693b3f1591583
hash_batch4_fetch_qualification=2fcf0b27039ed8e8e8114c632d51ba2f75ed2e6c28ef4a7081dd347d39cba9da
hash_batch_raw_qualification=c38e258cb76bf8013a721cacb73e98b1d7eb191badbfaed6076655284543d379
hash_frontend_qualification=f6998083749dcd63316f6ba25c333b7f979dc7d71e4d2ad668fce660065fcda2
hash_raw_unpack_cache_qualification=03001d15f4e6a8693a8565ddae1f495fff4399ef2d00c9ad05905aa45fd1791d
hash_tile_prefetch_qualification=d3445b26f4496e669d48a39068312096403bd6ac869ef7bd45d8d19575ab9990
hash_command_reuse_qualification=5640dd3dffb80fae0e6a3c9a6c2b91d83a1db86a0a5ab530d5932b3eb7b0cfc3
hash_peer_phase_qualification=e2a98809b9fc573c97e384920688b8bef53a7211493f8d2cb2933cfd13ed7f58
hash_q4_nibble_qualification=0f7213d98b12575a6bd4335f4424508040129940e5ae4aebe38f49d016e167b5
hash_q4_direct_qualification=37b36f9021fcef469eaae5db22f665845ceb429129c90659e3fb01fe3c5d7940
hash_local_q4_qualification=d714a8b13327978d1522c416beb79f75c460d66825ac5f41a4c71f7b622d64e4
guardsha=d1ce2aa4d4d0f83a711c848e911005622fcb684cd38edbb5cc9c104c0acc7229
clientsha=b0c851b302014e4677cee250d09634d8058be0eae223480a2aac45b3d285137f
taga=2026101900010001
tagb=2026101900010002
tagp=2026101900020001
tagq=2026101900020002
tagc=2026101900020003
tagf=2026101900020004
tagr=2026101900030001
tagx=2026101900030002
tagw=2026101900040002
tagwp=2026101900050001
tagws=2026101900050002
tagwc=2026101900050003
tagw4=2026101900050004
tagw4i=2026101900050005
tagwsp=2026101900060001
tagwspf=2026101900060002
tagwspc=2026101900060003
tagwspi=2026101900060004
tagwspq=2026101900060005
tagwsps=2026101900060006
tagwspa=2026101900060007
tagwspu=2026101900060008
tagwspd=2026101900060009
tagwspo=202610190006000a
tagwspj=202610190006000b
tagwspv=202610190006000c
tagwspg=202610190006000d
tagcurrentprofile=2026102000010001
tag_sparse_profile=2026102000010002
tag_sparse_export=2026102000010003
tag_normal_clock=2026102000010004
tag_sum_cache=2026102000010005
tag_zero_minimum=2026102000010006
tag_native_operand=2026102000010007
tag_compact_output=2026102000010008
tag_compact_profile=2026102000010009
tag_deferred_affine=202610200001000a
tag_ordered_extents=202610200001000b
tag_inline_config=202610200001000c
tag_postscale_delta=202610210001000d
tag_gdn_zero_diagnostic=202610210001000e
tag_resident_root_diagnostic=202610210001000f
tag_root_cycle_diagnostic=2026102100010010
tag_block_affine_qualification=2026102100010011
tag_scale_reuse_qualification=2026102100010012
tag_group32_qualification=2026102100010013
tag_native_q8_qualification=2026102100010014
tag_affine_rawdot_qualification=2026102100010015
tag_direct_srcb_qualification=2026102100010016
tag_sentinel_poll_qualification=2026102100010017
tag_graph_csr_diagnostic=2026102200010031
tag_ffn_code_diagnostic=2026102200010032
tag_ffn_peer_lifetime_diagnostic=2026102200010033
tag_ffn_root_lifetime_diagnostic=2026102200010034
tag_ffn_model_init=2026102200010035
tag_ffn_native_graph=2026102200010036
tag_ffn_host_profile=2026102200010037
tag_ffn_sealed_proof=2026102200010038
tag_embedding_validation=2026102200010040
tag_rms_validation=2026102200010041
tag_projection_validation=2026102200010042
tag_decoder_validation=2026102300010044
tag_gdn_input_validation=2026102300010046
tag_gdn_recurrent_validation=2026102400010049
tag_gdn_rendezvous_validation=2026102400010052
tag_conv_silu_precision_validation=2026102400010064
tag_resident_output_validation=2026102500010068
tag_resident_layer_validation=2026102600010069
tag_mixed_live_validation=2026102700010070
tag_recurrent_block_validation=2026102700010071
tag_recurrent_deadline_validation=2026102700010072
tag_recurrent_prefix_validation=2026102800010073
tag_token_validation=2026103000010074
tag_token_hold_recheck=2026103000010074
tag_token_root_frontend=2026103000010074
tag_token_hidden_readout=2026103100010075
tag_host_phase_diagnostic=2026102100010018
tag_batch4_fetch_qualification=2026102100010019
tag_batch_raw_qualification=2026102100010020
tag_frontend_qualification=2026102100010021
tag_raw_unpack_cache_qualification=2026102100010022
tag_tile_prefetch_qualification=2026102100010023
tag_command_reuse_qualification=2026102100010024
tag_peer_phase_qualification=2026102100010025
tag_q4_nibble_qualification=2026102100010026
tag_q4_direct_qualification=2026102100010027
tag_local_q4_qualification=2026102100010028
exact()
{
 test -f "$1" && test ! -L "$1" || return 1
 reload_hash=$(sha256sum "$1"); reload_hash=${reload_hash%% *}
 test "$reload_hash" = "$2"
}
parents_idle()
{
 reload_teams=$(ps -o Id -o Team) || return 1
 printf '%s\n' "$reload_teams" | awk '
  $1=="Id" && $2=="Team" && NF==2 {headers++;next}
  $1 ~ /^[0-9]+$/ && NF>=2 {teams++;next}
  NF {invalid++}
  END {if(headers!=1 || !teams || invalid) exit 1}
 ' || { echo STOP_COMPUTE_PARENT_INVENTORY_UNPROVEN; return 1; }
 printf '%s\n' "$reload_teams" | awk '
  /(^|[\/[:space:]])(Assistant|llama-|tenstorrent_haiku|inference-|pioneer_q38|pioneer-q|graph-csr-debug|source-check)/ {owner++}
  END {if(owner) exit 1}
 ' || { echo STOP_POSSIBLE_COMPUTE_PARENT; return 1; }
}
unloaded()
{
 reload_images=$(listimage 1) || return 1
 printf '%s\n' "$reload_images" | awk '
  /^KERNEL TEAM:$/ {headers++}
  $1 ~ /^[0-9]+$/ {images++; if($6=="kernel_riscv64") kernel++; if($6 ~ /tenstorrent|P100A/) card++}
  END {if(headers!=1 || kernel!=1 || images<30 || images>200 || card) exit 1}
 ' || { echo STOP_BOOTSTRAP_REQUIRES_FRESH_UNLOADED_DRIVER; return 1; }
 parents_idle
}
backup()
{
 if test -e "$1" || test -L "$1"; then exact "$1" "$2"
 else cp -p "$installed" "$1"; exact "$1" "$2"; fi
}
test -d "$stage" && test ! -L "$stage" || exit 1
test -L "$device" && test "$(readlink "$device")" = ../../bin/tenstorrent_pci_probe || exit 1
exact "$candidatea" "$hasha" && exact "$candidateb" "$hashb" && exact "$guardsource" "$guardsha" && exact "$client" "$clientsha" || exit 1
test ! -e "$incoming" && test ! -L "$incoming" && test ! -e "$guardincoming" && test ! -L "$guardincoming" || exit 1
if [ "$mode" = --bootstrap ]; then
 # Only this first migration requires a fresh boot. NEVER use it to erase
 # a failure latch in an old driver that lacks the new handshake.
 exact "$installed" "$original" && unloaded || exit 1
 backup "$stage/qualified55-before-hot-reload" "$original"
 if test -e "$guard" || test -L "$guard"; then exact "$guard" "$guardsha" || exit 1
 else
  cp -p "$guardsource" "$guardincoming"; chmod 755 "$guardincoming"
  exact "$guardincoming" "$guardsha" && unloaded || exit 1
  mkdir -p /boot/home/config/non-packaged/add-ons/kernel/generic
  mv "$guardincoming" "$guard"
 fi
 source=$candidatea; expected=$original; result=$hasha
else
 exact "$guard" "$guardsha" || exit 1
 if [ "$mode" = --apply-b ]; then source=$candidateb; expected=$hasha; result=$hashb; oldtag=$taga; newtag=$tagb
 elif [ "$mode" = --apply-profile ]; then source=$candidatep; expected=$hasha; result=$hashp; oldtag=$taga; newtag=$tagp
 elif [ "$mode" = --restore-profile ]; then source=$candidatea; expected=$hashp; result=$hasha; oldtag=$tagp; newtag=$taga
 elif [ "$mode" = --apply-batch ]; then source=$candidateq; expected=$hashp; result=$hashq; oldtag=$tagp; newtag=$tagq
 elif [ "$mode" = --restore-batch ]; then source=$candidatea; expected=$hashq; result=$hasha; oldtag=$tagq; newtag=$taga
 elif [ "$mode" = --apply-compact ]; then source=$candidatec; expected=$hashq; result=$hashc; oldtag=$tagq; newtag=$tagc
 elif [ "$mode" = --restore-compact ]; then source=$candidatea; expected=$hashc; result=$hasha; oldtag=$tagc; newtag=$taga
 elif [ "$mode" = --apply-fast ]; then source=$candidatef; expected=$hashc; result=$hashf; oldtag=$tagc; newtag=$tagf
 elif [ "$mode" = --restore-fast ]; then source=$candidatea; expected=$hashf; result=$hasha; oldtag=$tagf; newtag=$taga
 elif [ "$mode" = --apply-reuse ]; then source=$candidater; expected=$hashf; result=$hashr; oldtag=$tagf; newtag=$tagr
 elif [ "$mode" = --restore-reuse ]; then source=$candidatef; expected=$hashr; result=$hashf; oldtag=$tagr; newtag=$tagf
 elif [ "$mode" = --apply-cross-reuse ]; then source=$candidatex; expected=$hashr; result=$hashx; oldtag=$tagr; newtag=$tagx
 elif [ "$mode" = --restore-cross-reuse ]; then source=$candidater; expected=$hashx; result=$hashr; oldtag=$tagx; newtag=$tagr
 elif [ "$mode" = --apply-work ]; then source=$candidatew; expected=$hashx; result=$hashw; oldtag=$tagx; newtag=$tagw
 elif [ "$mode" = --restore-work ]; then source=$candidatex; expected=$hashw; result=$hashx; oldtag=$tagw; newtag=$tagx
 elif [ "$mode" = --apply-work-profile ]; then source=$candidatewp; expected=$hashw; result=$hashwp; oldtag=$tagw; newtag=$tagwp
 elif [ "$mode" = --restore-work-profile ]; then source=$candidatew; expected=$hashwp; result=$hashw; oldtag=$tagwp; newtag=$tagw
 elif [ "$mode" = --apply-work-posted ]; then source=$candidatews; expected=$hashw; result=$hashws; oldtag=$tagw; newtag=$tagws
 elif [ "$mode" = --restore-work-posted ]; then source=$candidatew; expected=$hashws; result=$hashw; oldtag=$tagws; newtag=$tagw
 elif [ "$mode" = --apply-work-cohort ]; then source=$candidatewc; expected=$hashws; result=$hashwc; oldtag=$tagws; newtag=$tagwc
 elif [ "$mode" = --restore-work-cohort ]; then source=$candidatews; expected=$hashwc; result=$hashws; oldtag=$tagwc; newtag=$tagws
 elif [ "$mode" = --apply-work-q4 ]; then source=$candidatew4; expected=$hashwc; result=$hashw4; oldtag=$tagwc; newtag=$tagw4
 elif [ "$mode" = --restore-work-q4 ]; then source=$candidatewc; expected=$hashw4; result=$hashwc; oldtag=$tagw4; newtag=$tagwc
 elif [ "$mode" = --apply-work-q4-inline ]; then source=$candidatew4i; expected=$hashw4; result=$hashw4i; oldtag=$tagw4; newtag=$tagw4i
 elif [ "$mode" = --restore-work-q4-inline ]; then source=$candidatew4; expected=$hashw4i; result=$hashw4; oldtag=$tagw4i; newtag=$tagw4
 elif [ "$mode" = --apply-work-span ]; then source=$candidatewsp; expected=$hashw4i; result=$hashwsp; oldtag=$tagw4i; newtag=$tagwsp
 elif [ "$mode" = --restore-work-span ]; then source=$candidatew4i; expected=$hashwsp; result=$hashw4i; oldtag=$tagwsp; newtag=$tagw4i
 elif [ "$mode" = --apply-work-fast-span ]; then source=$candidatewspf; expected=$hashwsp; result=$hashwspf; oldtag=$tagwsp; newtag=$tagwspf
 elif [ "$mode" = --restore-work-fast-span ]; then source=$candidatewsp; expected=$hashwspf; result=$hashwsp; oldtag=$tagwspf; newtag=$tagwsp
 elif [ "$mode" = --apply-work-span-code-reuse ]; then source=$candidatewspc; expected=$hashwspf; result=$hashwspc; oldtag=$tagwspf; newtag=$tagwspc
 elif [ "$mode" = --restore-work-span-code-reuse ]; then source=$candidatewspf; expected=$hashwspc; result=$hashwspf; oldtag=$tagwspc; newtag=$tagwspf
 elif [ "$mode" = --apply-work-int-sfpu ]; then source=$candidatewspi; expected=$hashwspc; result=$hashwspi; oldtag=$tagwspc; newtag=$tagwspi
 elif [ "$mode" = --restore-work-int-sfpu ]; then source=$candidatewspc; expected=$hashwspi; result=$hashwspc; oldtag=$tagwspi; newtag=$tagwspc
 elif [ "$mode" = --apply-work-pipelined ]; then source=$candidatewspq; expected=$hashwspi; result=$hashwspq; oldtag=$tagwspi; newtag=$tagwspq
 elif [ "$mode" = --restore-work-pipelined ]; then source=$candidatewspi; expected=$hashwspq; result=$hashwspi; oldtag=$tagwspq; newtag=$tagwspi
 elif [ "$mode" = --apply-work-static-vector ]; then source=$candidatewsps; expected=$hashwspq; result=$hashwsps; oldtag=$tagwspq; newtag=$tagwsps
 elif [ "$mode" = --restore-work-static-vector ]; then source=$candidatewspq; expected=$hashwsps; result=$hashwspq; oldtag=$tagwsps; newtag=$tagwspq
 elif [ "$mode" = --apply-work-cached-affine ]; then source=$candidatewspa; expected=$hashwsps; result=$hashwspa; oldtag=$tagwsps; newtag=$tagwspa
 elif [ "$mode" = --restore-work-cached-affine ]; then source=$candidatewsps; expected=$hashwspa; result=$hashwsps; oldtag=$tagwspa; newtag=$tagwsps
 elif [ "$mode" = --apply-work-replay-operand ]; then source=$candidatewspu; expected=$hashwspa; result=$hashwspu; oldtag=$tagwspa; newtag=$tagwspu
 elif [ "$mode" = --restore-work-replay-operand ]; then source=$candidatewspa; expected=$hashwspu; result=$hashwspa; oldtag=$tagwspu; newtag=$tagwspa
 elif [ "$mode" = --apply-work-dot ]; then source=$candidatewspd; expected=$hashwspu; result=$hashwspd; oldtag=$tagwspu; newtag=$tagwspd
 elif [ "$mode" = --restore-work-dot ]; then source=$candidatewspu; expected=$hashwspd; result=$hashwspu; oldtag=$tagwspd; newtag=$tagwspu
 elif [ "$mode" = --apply-work-output-dma ]; then source=$candidatewspo; expected=$hashwspd; result=$hashwspo; oldtag=$tagwspd; newtag=$tagwspo
 elif [ "$mode" = --restore-work-output-dma ]; then source=$candidatewspd; expected=$hashwspo; result=$hashwspd; oldtag=$tagwspo; newtag=$tagwspd
 elif [ "$mode" = --apply-work-queued-entry ]; then source=$candidatewspj; expected=$hashwspo; result=$hashwspj; oldtag=$tagwspo; newtag=$tagwspj
 elif [ "$mode" = --restore-work-queued-entry ]; then source=$candidatewspo; expected=$hashwspj; result=$hashwspo; oldtag=$tagwspj; newtag=$tagwspo
 elif [ "$mode" = --apply-local-delta ]; then source=$candidatewspv; expected=$hashwspj; result=$hashwspv; oldtag=$tagwspj; newtag=$tagwspv
 elif [ "$mode" = --restore-local-delta ]; then source=$candidatewspj; expected=$hashwspv; result=$hashwspj; oldtag=$tagwspv; newtag=$tagwspj
 elif [ "$mode" = --apply-local-delta-terminal ]; then source=$candidatewspg; expected=$hashwspv; result=$hashwspg; oldtag=$tagwspv; newtag=$tagwspg
 elif [ "$mode" = --restore-local-delta-terminal ]; then source=$candidatewspv; expected=$hashwspg; result=$hashwspv; oldtag=$tagwspg; newtag=$tagwspv
 elif [ "$mode" = --apply-current-profile ]; then source=$candidatecurrentprofile; expected=$hashwspg; result=$hashcurrentprofile; oldtag=$tagwspg; newtag=$tagcurrentprofile
 elif [ "$mode" = --restore-current-profile ]; then source=$candidatewspg; expected=$hashcurrentprofile; result=$hashwspg; oldtag=$tagcurrentprofile; newtag=$tagwspg
 elif [ "$mode" = --apply-sparse-profile ]; then source=$candidate_sparse_profile; expected=$hashcurrentprofile; result=$hash_sparse_profile; oldtag=$tagcurrentprofile; newtag=$tag_sparse_profile
 elif [ "$mode" = --restore-sparse-profile ]; then source=$candidatecurrentprofile; expected=$hash_sparse_profile; result=$hashcurrentprofile; oldtag=$tag_sparse_profile; newtag=$tagcurrentprofile
 elif [ "$mode" = --apply-sparse-export ]; then source=$candidate_sparse_export; expected=$hash_sparse_profile; result=$hash_sparse_export; oldtag=$tag_sparse_profile; newtag=$tag_sparse_export
 elif [ "$mode" = --restore-sparse-export ]; then source=$candidate_sparse_profile; expected=$hash_sparse_export; result=$hash_sparse_profile; oldtag=$tag_sparse_export; newtag=$tag_sparse_profile
 elif [ "$mode" = --apply-normal-clock ]; then source=$candidate_normal_clock; expected=$hash_sparse_export; result=$hash_normal_clock; oldtag=$tag_sparse_export; newtag=$tag_normal_clock
 elif [ "$mode" = --restore-normal-clock ]; then source=$candidate_sparse_export; expected=$hash_normal_clock; result=$hash_sparse_export; oldtag=$tag_normal_clock; newtag=$tag_sparse_export
 elif [ "$mode" = --apply-input-sum-cache ]; then source=$candidate_sum_cache; expected=$hash_normal_clock; result=$hash_sum_cache; oldtag=$tag_normal_clock; newtag=$tag_sum_cache
 elif [ "$mode" = --restore-input-sum-cache ]; then source=$candidate_normal_clock; expected=$hash_sum_cache; result=$hash_normal_clock; oldtag=$tag_sum_cache; newtag=$tag_normal_clock
 elif [ "$mode" = --apply-zero-minimum ]; then source=$candidate_zero_minimum; expected=$hash_sum_cache; result=$hash_zero_minimum; oldtag=$tag_sum_cache; newtag=$tag_zero_minimum
 elif [ "$mode" = --restore-zero-minimum ]; then source=$candidate_sum_cache; expected=$hash_zero_minimum; result=$hash_sum_cache; oldtag=$tag_zero_minimum; newtag=$tag_sum_cache
 elif [ "$mode" = --apply-native-operand ]; then source=$candidate_native_operand; expected=$hash_zero_minimum; result=$hash_native_operand; oldtag=$tag_zero_minimum; newtag=$tag_native_operand
 elif [ "$mode" = --restore-native-operand ]; then source=$candidate_zero_minimum; expected=$hash_native_operand; result=$hash_zero_minimum; oldtag=$tag_native_operand; newtag=$tag_zero_minimum
 elif [ "$mode" = --apply-compact-output ]; then source=$candidate_compact_output; expected=$hash_native_operand; result=$hash_compact_output; oldtag=$tag_native_operand; newtag=$tag_compact_output
 elif [ "$mode" = --restore-compact-output ]; then source=$candidate_native_operand; expected=$hash_compact_output; result=$hash_native_operand; oldtag=$tag_compact_output; newtag=$tag_native_operand
 elif [ "$mode" = --apply-compact-profile ]; then source=$candidate_compact_profile; expected=$hash_compact_output; result=$hash_compact_profile; oldtag=$tag_compact_output; newtag=$tag_compact_profile
 elif [ "$mode" = --restore-compact-profile ]; then source=$candidate_compact_output; expected=$hash_compact_profile; result=$hash_compact_output; oldtag=$tag_compact_profile; newtag=$tag_compact_output
 elif [ "$mode" = --apply-deferred-affine ]; then source=$candidate_deferred_affine; expected=$hash_compact_output; result=$hash_deferred_affine; oldtag=$tag_compact_output; newtag=$tag_deferred_affine
 elif [ "$mode" = --restore-deferred-affine ]; then source=$candidate_compact_output; expected=$hash_deferred_affine; result=$hash_compact_output; oldtag=$tag_deferred_affine; newtag=$tag_compact_output
 elif [ "$mode" = --apply-ordered-extents ]; then source=$candidate_ordered_extents; expected=$hash_compact_output; result=$hash_ordered_extents; oldtag=$tag_compact_output; newtag=$tag_ordered_extents
 elif [ "$mode" = --restore-ordered-extents ]; then source=$candidate_compact_output; expected=$hash_ordered_extents; result=$hash_compact_output; oldtag=$tag_ordered_extents; newtag=$tag_compact_output
 elif [ "$mode" = --apply-inline-config ]; then source=$candidate_inline_config; expected=$hash_deferred_affine; result=$hash_inline_config; oldtag=$tag_deferred_affine; newtag=$tag_inline_config
 elif [ "$mode" = --restore-inline-config ]; then source=$candidate_deferred_affine; expected=$hash_inline_config; result=$hash_deferred_affine; oldtag=$tag_inline_config; newtag=$tag_deferred_affine
 elif [ "$mode" = --apply-postscale-delta ]; then source=$candidate_postscale_delta; expected=$hash_inline_config; result=$hash_postscale_delta; oldtag=$tag_inline_config; newtag=$tag_postscale_delta
 elif [ "$mode" = --restore-postscale-delta ]; then source=$candidate_inline_config; expected=$hash_postscale_delta; result=$hash_inline_config; oldtag=$tag_postscale_delta; newtag=$tag_inline_config
 elif [ "$mode" = --apply-gdn-zero-diagnostic ]; then source=$candidate_gdn_zero_diagnostic; expected=$hash_postscale_delta; result=$hash_gdn_zero_diagnostic; oldtag=$tag_postscale_delta; newtag=$tag_gdn_zero_diagnostic
 elif [ "$mode" = --restore-gdn-zero-diagnostic ]; then source=$candidate_postscale_delta; expected=$hash_gdn_zero_diagnostic; result=$hash_postscale_delta; oldtag=$tag_gdn_zero_diagnostic; newtag=$tag_postscale_delta
 elif [ "$mode" = --apply-resident-root-diagnostic ]; then source=$candidate_resident_root_diagnostic; expected=$hash_gdn_zero_diagnostic; result=$hash_resident_root_diagnostic; oldtag=$tag_gdn_zero_diagnostic; newtag=$tag_resident_root_diagnostic
 elif [ "$mode" = --restore-resident-root-diagnostic ]; then source=$candidate_gdn_zero_diagnostic; expected=$hash_resident_root_diagnostic; result=$hash_gdn_zero_diagnostic; oldtag=$tag_resident_root_diagnostic; newtag=$tag_gdn_zero_diagnostic
 elif [ "$mode" = --apply-root-cycle-diagnostic ]; then source=$candidate_root_cycle_diagnostic; expected=$hash_resident_root_diagnostic; result=$hash_root_cycle_diagnostic; oldtag=$tag_resident_root_diagnostic; newtag=$tag_root_cycle_diagnostic
 elif [ "$mode" = --restore-root-cycle-diagnostic ]; then source=$candidate_resident_root_diagnostic; expected=$hash_root_cycle_diagnostic; result=$hash_resident_root_diagnostic; oldtag=$tag_root_cycle_diagnostic; newtag=$tag_resident_root_diagnostic
 elif [ "$mode" = --apply-block-affine-qualification ]; then source=$candidate_block_affine_qualification; expected=$hash_root_cycle_diagnostic; result=$hash_block_affine_qualification; oldtag=$tag_root_cycle_diagnostic; newtag=$tag_block_affine_qualification
 elif [ "$mode" = --restore-block-affine-qualification ]; then source=$candidate_root_cycle_diagnostic; expected=$hash_block_affine_qualification; result=$hash_root_cycle_diagnostic; oldtag=$tag_block_affine_qualification; newtag=$tag_root_cycle_diagnostic
 elif [ "$mode" = --apply-scale-reuse-qualification ]; then source=$candidate_scale_reuse_qualification; expected=$hash_block_affine_qualification; result=$hash_scale_reuse_qualification; oldtag=$tag_block_affine_qualification; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --restore-scale-reuse-qualification ]; then source=$candidate_block_affine_qualification; expected=$hash_scale_reuse_qualification; result=$hash_block_affine_qualification; oldtag=$tag_scale_reuse_qualification; newtag=$tag_block_affine_qualification
 elif [ "$mode" = --apply-group32-qualification ]; then source=$candidate_group32_qualification; expected=$hash_scale_reuse_qualification; result=$hash_group32_qualification; oldtag=$tag_scale_reuse_qualification; newtag=$tag_group32_qualification
 elif [ "$mode" = --restore-group32-qualification ]; then source=$candidate_scale_reuse_qualification; expected=$hash_group32_qualification; result=$hash_scale_reuse_qualification; oldtag=$tag_group32_qualification; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-native-q8-qualification ]; then source=$candidate_native_q8_qualification; expected=$hash_scale_reuse_qualification; result=$hash_native_q8_qualification; oldtag=$tag_scale_reuse_qualification; newtag=$tag_native_q8_qualification
 elif [ "$mode" = --restore-native-q8-qualification ]; then source=$candidate_scale_reuse_qualification; expected=$hash_native_q8_qualification; result=$hash_scale_reuse_qualification; oldtag=$tag_native_q8_qualification; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-affine-rawdot-qualification ]; then source=$candidate_affine_rawdot_qualification; expected=$hash_scale_reuse_qualification; result=$hash_affine_rawdot_qualification; oldtag=$tag_scale_reuse_qualification; newtag=$tag_affine_rawdot_qualification
 elif [ "$mode" = --restore-affine-rawdot-qualification ]; then source=$candidate_scale_reuse_qualification; expected=$hash_affine_rawdot_qualification; result=$hash_scale_reuse_qualification; oldtag=$tag_affine_rawdot_qualification; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-direct-srcb-qualification ]; then source=$candidate_direct_srcb_qualification; expected=$hash_scale_reuse_qualification; result=$hash_direct_srcb_qualification; oldtag=$tag_scale_reuse_qualification; newtag=$tag_direct_srcb_qualification
 elif [ "$mode" = --restore-direct-srcb-qualification ]; then source=$candidate_scale_reuse_qualification; expected=$hash_direct_srcb_qualification; result=$hash_scale_reuse_qualification; oldtag=$tag_direct_srcb_qualification; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-sentinel-poll-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_direct_srcb_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_direct_srcb_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --restore-sentinel-poll-qualification ]; then source=$candidate_direct_srcb_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_direct_srcb_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_direct_srcb_qualification
 # Isolated fixed-read CSR qualifier:transition directly from/to the measured
 # best tag12. Restore is allowed ONLY with clean idle metadata,NEVER UNKNOWN.
 elif [ "$mode" = --apply-graph-csr-diagnostic ]; then source=$candidate_graph_csr_diagnostic; expected=$hash_scale_reuse_qualification; result=$hash_graph_csr_diagnostic; oldtag=$tag_scale_reuse_qualification; newtag=$tag_graph_csr_diagnostic
 elif [ "$mode" = --restore-graph-csr-diagnostic ]; then source=$candidate_scale_reuse_qualification; expected=$hash_graph_csr_diagnostic; result=$hash_scale_reuse_qualification; oldtag=$tag_graph_csr_diagnostic; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-ffn-code-diagnostic ]; then source=$candidate_ffn_code_diagnostic; expected=$hash_scale_reuse_qualification; result=$hash_ffn_code_diagnostic; oldtag=$tag_scale_reuse_qualification; newtag=$tag_ffn_code_diagnostic
 elif [ "$mode" = --restore-ffn-code-diagnostic ]; then source=$candidate_scale_reuse_qualification; expected=$hash_ffn_code_diagnostic; result=$hash_scale_reuse_qualification; oldtag=$tag_ffn_code_diagnostic; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-ffn-peer-lifetime-diagnostic ]; then source=$candidate_ffn_peer_lifetime_diagnostic; expected=$hash_scale_reuse_qualification; result=$hash_ffn_peer_lifetime_diagnostic; oldtag=$tag_scale_reuse_qualification; newtag=$tag_ffn_peer_lifetime_diagnostic
 elif [ "$mode" = --restore-ffn-peer-lifetime-diagnostic ]; then source=$candidate_scale_reuse_qualification; expected=$hash_ffn_peer_lifetime_diagnostic; result=$hash_scale_reuse_qualification; oldtag=$tag_ffn_peer_lifetime_diagnostic; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-ffn-root-lifetime-diagnostic ]; then source=$candidate_ffn_root_lifetime_diagnostic; expected=$hash_scale_reuse_qualification; result=$hash_ffn_root_lifetime_diagnostic; oldtag=$tag_scale_reuse_qualification; newtag=$tag_ffn_root_lifetime_diagnostic
 elif [ "$mode" = --restore-ffn-root-lifetime-diagnostic ]; then source=$candidate_scale_reuse_qualification; expected=$hash_ffn_root_lifetime_diagnostic; result=$hash_scale_reuse_qualification; oldtag=$tag_ffn_root_lifetime_diagnostic; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-ffn-model-init ]; then source=$candidate_ffn_model_init; expected=$hash_scale_reuse_qualification; result=$hash_ffn_model_init; oldtag=$tag_scale_reuse_qualification; newtag=$tag_ffn_model_init
 elif [ "$mode" = --restore-ffn-model-init ]; then source=$candidate_scale_reuse_qualification; expected=$hash_ffn_model_init; result=$hash_scale_reuse_qualification; oldtag=$tag_ffn_model_init; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-ffn-native-graph ]; then source=$candidate_ffn_native_graph; expected=$hash_scale_reuse_qualification; result=$hash_ffn_native_graph; oldtag=$tag_scale_reuse_qualification; newtag=$tag_ffn_native_graph
 elif [ "$mode" = --restore-ffn-native-graph ]; then source=$candidate_scale_reuse_qualification; expected=$hash_ffn_native_graph; result=$hash_scale_reuse_qualification; oldtag=$tag_ffn_native_graph; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-ffn-host-profile ]; then source=$candidate_ffn_host_profile; expected=$hash_scale_reuse_qualification; result=$hash_ffn_host_profile; oldtag=$tag_scale_reuse_qualification; newtag=$tag_ffn_host_profile
 elif [ "$mode" = --restore-ffn-host-profile ]; then source=$candidate_scale_reuse_qualification; expected=$hash_ffn_host_profile; result=$hash_scale_reuse_qualification; oldtag=$tag_ffn_host_profile; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-ffn-sealed-proof ]; then source=$candidate_ffn_sealed_proof; expected=$hash_scale_reuse_qualification; result=$hash_ffn_sealed_proof; oldtag=$tag_scale_reuse_qualification; newtag=$tag_ffn_sealed_proof
 elif [ "$mode" = --restore-ffn-sealed-proof ]; then source=$candidate_scale_reuse_qualification; expected=$hash_ffn_sealed_proof; result=$hash_scale_reuse_qualification; oldtag=$tag_ffn_sealed_proof; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-embedding-validation ]; then source=$candidate_embedding_validation; expected=$hash_scale_reuse_qualification; result=$hash_embedding_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_embedding_validation
 elif [ "$mode" = --restore-embedding-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_embedding_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_embedding_validation; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-rms-validation ]; then source=$candidate_rms_validation; expected=$hash_scale_reuse_qualification; result=$hash_rms_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_rms_validation
 elif [ "$mode" = --restore-rms-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_rms_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_rms_validation; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-projection-validation ]; then source=$candidate_projection_validation; expected=$hash_scale_reuse_qualification; result=$hash_projection_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_projection_validation
 elif [ "$mode" = --restore-projection-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_projection_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_projection_validation; newtag=$tag_scale_reuse_qualification
 # Separate fixed DEBUG44 qualification. Apply from normal041/tag12 ONLY;
 # explicit restore requires known-clean idle metadata,NEVER UNKNOWN.
 elif [ "$mode" = --apply-decoder-validation ]; then source=$candidate_decoder_validation; expected=$hash_scale_reuse_qualification; result=$hash_decoder_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_decoder_validation
 elif [ "$mode" = --restore-decoder-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_decoder_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_decoder_validation; newtag=$tag_scale_reuse_qualification
 # Separate fixed DEBUG46;ONLY stable041 <-> exact qualified32a. Never
 # apply/restore through an unknown owner,cold latch or arbitrary same tag.
 elif [ "$mode" = --apply-gdn-input-validation ]; then source=$candidate_gdn_input_validation; expected=$hash_scale_reuse_qualification; result=$hash_gdn_input_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_gdn_input_validation
 elif [ "$mode" = --restore-gdn-input-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_gdn_input_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_gdn_input_validation; newtag=$tag_scale_reuse_qualification
 # Separate DEBUG49,never old same-tag prototypes. Physical selection still
 # requires completed original-model/client/public-lifecycle qualification.
 elif [ "$mode" = --apply-gdn-recurrent-validation ]; then source=$candidate_gdn_recurrent_validation; expected=$hash_scale_reuse_qualification; result=$hash_gdn_recurrent_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_gdn_recurrent_validation
 elif [ "$mode" = --apply-gdn-rendezvous-validation ]; then source=$candidate_gdn_rendezvous_validation; expected=$hash_scale_reuse_qualification; result=$hash_gdn_rendezvous_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_gdn_rendezvous_validation
 elif [ "$mode" = --restore-gdn-recurrent-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_gdn_recurrent_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_gdn_recurrent_validation; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --restore-gdn-rendezvous-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_gdn_rendezvous_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_gdn_rendezvous_validation; newtag=$tag_scale_reuse_qualification
 # Separate precise-convolution DEBUG64. ONLY clean exact041 <-> exact64;
 # never replace old52 under its old tag or recover an unknown/cold owner.
 elif [ "$mode" = --apply-conv-silu-precision-validation ]; then source=$candidate_conv_silu_precision_validation; expected=$hash_scale_reuse_qualification; result=$hash_conv_silu_precision_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_conv_silu_precision_validation
 elif [ "$mode" = --restore-conv-silu-precision-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_conv_silu_precision_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_conv_silu_precision_validation; newtag=$tag_scale_reuse_qualification
 # NEW fixed combined DEBUG68,not compile-only67 or a Work driver. ONLY
 # exact known-clean041 <-> byte-pinned68;UNKNOWN can never restore/retry.
 elif [ "$mode" = --apply-resident-output-validation ]; then source=$candidate_resident_output_validation; expected=$hash_scale_reuse_qualification; result=$hash_resident_output_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_resident_output_validation
 elif [ "$mode" = --restore-resident-output-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_resident_output_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_resident_output_validation; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-resident-layer-validation ]; then source=$candidate_resident_layer_validation; expected=$hash_scale_reuse_qualification; result=$hash_resident_layer_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_resident_layer_validation
 elif [ "$mode" = --restore-resident-layer-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_resident_layer_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_resident_layer_validation; newtag=$tag_scale_reuse_qualification
 # Separate standalone dense DEBUG70. No old69/NODRV/Work alias. ONLY
 # exact known-clean041 <-> byte-pinned70. UNKNOWN cannot restore/retry.
 elif [ "$mode" = --apply-mixed-live-validation ]; then source=$candidate_mixed_live_validation; expected=$hash_scale_reuse_qualification; result=$hash_mixed_live_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_mixed_live_validation
 elif [ "$mode" = --restore-mixed-live-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_mixed_live_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_mixed_live_validation; newtag=$tag_scale_reuse_qualification
 # Separate recurrent DEBUG71;NEVER an old dense/NODRV/Work alias.
 # Native/runtime qualification is REQUIRED BEFORE invoking this mode.
 # Only exact known-clean041 <-> exact87d0,with the SAME fail-closed
 # owner/quarantine/reload fences. UNKNOWN cannot restore or retry.
 elif [ "$mode" = --apply-recurrent-block-validation ]; then source=$candidate_recurrent_block_validation; expected=$hash_scale_reuse_qualification; result=$hash_recurrent_block_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_recurrent_block_validation
 elif [ "$mode" = --restore-recurrent-block-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_recurrent_block_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_recurrent_block_validation; newtag=$tag_scale_reuse_qualification
 # Distinct DEBUG72/namespace23 with qualified per-transition deadlines.
 # Only stable041 <-> exact3e00;never substitute old71 or cold recovery.
 # Runtime/original-model qualification remains REQUIRED before invocation.
 elif [ "$mode" = --apply-recurrent-deadline-validation ]; then source=$candidate_recurrent_deadline_validation; expected=$hash_scale_reuse_qualification; result=$hash_recurrent_deadline_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_recurrent_deadline_validation
 elif [ "$mode" = --restore-recurrent-deadline-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_recurrent_deadline_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_recurrent_deadline_validation; newtag=$tag_scale_reuse_qualification
 # Distinct three-layer DEBUG73/namespace24,NOT a retagged layer0 image.
 # Build/hash identity does NOT qualify runtime:complete current-source
 # original/readback/control/client gates BEFORE invoking. Only exact clean
 # stable041 <-> pinned4ae7;UNKNOWN cannot restore/retry or cold-recover.
 elif [ "$mode" = --apply-recurrent-prefix-validation ]; then source=$candidate_recurrent_prefix_validation; expected=$hash_scale_reuse_qualification; result=$hash_recurrent_prefix_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_recurrent_prefix_validation
 elif [ "$mode" = --restore-recurrent-prefix-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_recurrent_prefix_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_recurrent_prefix_validation; newtag=$tag_scale_reuse_qualification
 # Distinct bounded full-token diagnostic ONLY. This mapping is not
 # qualification to run a model. Exact clean041 <-> pinned d784 only;
 # no Work route or restoration through retained/cold/unknown resources.
 elif [ "$mode" = --apply-token-validation ]; then source=$candidate_token_validation; expected=$hash_scale_reuse_qualification; result=$hash_token_validation; oldtag=$tag_scale_reuse_qualification; newtag=$tag_token_validation
 elif [ "$mode" = --restore-token-validation ]; then source=$candidate_scale_reuse_qualification; expected=$hash_token_validation; result=$hash_scale_reuse_qualification; oldtag=$tag_token_validation; newtag=$tag_scale_reuse_qualification
 # New bounded terminal-to-HOLD recheck, still diagnostic ONLY. Runtime
 # and original-model qualification must pass BEFORE invoking this mode.
 # Exact clean041 <-> pinned781ab ONLY; d784 shares the numeric tag but
 # is NEVER an acceptable source/installed image for this transition.
 # This does not recover retained/cold/unknown resources or enable Work.
 elif [ "$mode" = --apply-token-hold-recheck ]; then source=$candidate_token_hold_recheck; expected=$hash_scale_reuse_qualification; result=$hash_token_hold_recheck; oldtag=$tag_scale_reuse_qualification; newtag=$tag_token_hold_recheck
 elif [ "$mode" = --restore-token-hold-recheck ]; then source=$candidate_scale_reuse_qualification; expected=$hash_token_hold_recheck; result=$hash_scale_reuse_qualification; oldtag=$tag_token_hold_recheck; newtag=$tag_scale_reuse_qualification
 # ROOT snapshots are now diagnostic DATA;current boot/terminal/code/FIFO
 # seals authorize closure. Runtime/original-model qualification is still
 # REQUIRED before invoking. Exact clean041 <-> pinned5f2fc ONLY;neither
 # d784 nor781ab is accepted by a numeric tag or a previous backup.
 # No Work alias,retained-owner recovery,reset or automatic rollback.
 elif [ "$mode" = --apply-token-root-frontend ]; then source=$candidate_token_root_frontend; expected=$hash_scale_reuse_qualification; result=$hash_token_root_frontend; oldtag=$tag_scale_reuse_qualification; newtag=$tag_token_root_frontend
 elif [ "$mode" = --restore-token-root-frontend ]; then source=$candidate_scale_reuse_qualification; expected=$hash_token_root_frontend; result=$hash_scale_reuse_qualification; oldtag=$tag_token_root_frontend; newtag=$tag_scale_reuse_qualification
 # Distinct tag75 held-hidden diagnostic,NOT old74 or Work. This mapping
 # is NOT a runtime/numerical grant and cannot recover retained resources.
 elif [ "$mode" = --apply-token-hidden-readout ]; then source=$candidate_token_hidden_readout; expected=$hash_scale_reuse_qualification; result=$hash_token_hidden_readout; oldtag=$tag_scale_reuse_qualification; newtag=$tag_token_hidden_readout
 elif [ "$mode" = --restore-token-hidden-readout ]; then source=$candidate_scale_reuse_qualification; expected=$hash_token_hidden_readout; result=$hash_scale_reuse_qualification; oldtag=$tag_token_hidden_readout; newtag=$tag_scale_reuse_qualification
 elif [ "$mode" = --apply-host-phase-diagnostic ]; then source=$candidate_host_phase_diagnostic; expected=$hash_sentinel_poll_qualification; result=$hash_host_phase_diagnostic; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_host_phase_diagnostic
 elif [ "$mode" = --restore-host-phase-diagnostic ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_host_phase_diagnostic; result=$hash_sentinel_poll_qualification; oldtag=$tag_host_phase_diagnostic; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-batch4-fetch-qualification ]; then source=$candidate_batch4_fetch_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_batch4_fetch_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_batch4_fetch_qualification
 elif [ "$mode" = --restore-batch4-fetch-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_batch4_fetch_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_batch4_fetch_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-batch-raw-qualification ]; then source=$candidate_batch_raw_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_batch_raw_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_batch_raw_qualification
 elif [ "$mode" = --restore-batch-raw-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_batch_raw_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_batch_raw_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-frontend-qualification ]; then source=$candidate_frontend_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_frontend_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_frontend_qualification
 elif [ "$mode" = --restore-frontend-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_frontend_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_frontend_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-raw-unpack-cache-qualification ]; then source=$candidate_raw_unpack_cache_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_raw_unpack_cache_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_raw_unpack_cache_qualification
 elif [ "$mode" = --restore-raw-unpack-cache-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_raw_unpack_cache_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_raw_unpack_cache_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-tile-prefetch-qualification ]; then source=$candidate_tile_prefetch_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_tile_prefetch_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_tile_prefetch_qualification
 elif [ "$mode" = --restore-tile-prefetch-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_tile_prefetch_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_tile_prefetch_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-command-reuse-qualification ]; then source=$candidate_command_reuse_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_command_reuse_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_command_reuse_qualification
 elif [ "$mode" = --restore-command-reuse-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_command_reuse_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_command_reuse_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-peer-phase-qualification ]; then source=$candidate_peer_phase_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_peer_phase_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_peer_phase_qualification
 elif [ "$mode" = --restore-peer-phase-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_peer_phase_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_peer_phase_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-q4-nibble-qualification ]; then source=$candidate_q4_nibble_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_q4_nibble_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_q4_nibble_qualification
 elif [ "$mode" = --restore-q4-nibble-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_q4_nibble_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_q4_nibble_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-q4-direct-qualification ]; then source=$candidate_q4_direct_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_q4_direct_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_q4_direct_qualification
 elif [ "$mode" = --restore-q4-direct-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_q4_direct_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_q4_direct_qualification; newtag=$tag_sentinel_poll_qualification
 elif [ "$mode" = --apply-local-q4-qualification ]; then source=$candidate_local_q4_qualification; expected=$hash_sentinel_poll_qualification; result=$hash_local_q4_qualification; oldtag=$tag_sentinel_poll_qualification; newtag=$tag_local_q4_qualification
 elif [ "$mode" = --restore-local-q4-qualification ]; then source=$candidate_sentinel_poll_qualification; expected=$hash_local_q4_qualification; result=$hash_sentinel_poll_qualification; oldtag=$tag_local_q4_qualification; newtag=$tag_sentinel_poll_qualification
 else source=$candidatea; expected=$hashb; result=$hasha; oldtag=$tagb; newtag=$taga; fi
 exact "$source" "$result" || exit 1
 exact "$installed" "$expected" || exit 1
 # An idle driver is not enough:a live CPU-authentication/model parent may
 # not have opened it yet. Do not replace code during ANY such lifetime.
 parents_idle || exit 1
 metadata=$("$client" --status) || exit 1
 printf '%s\n' "$metadata"
 generation=$(printf '%s\n' "$metadata" | awk -v tag="tag$oldtag" '
  $1=="P100A_RELOAD" && $2=="driver55" && $3==tag && $5=="quiesced0" && $6=="cold0" && $7=="active_owner0" && $8=="open_count1" && $9 ~ /^guard1;/ {
   value=$4; sub(/^generation/,"",value); if(value ~ /^[1-9][0-9]*$/) {print value; good++}}
  END {if(good!=1)exit 1}
 ') || exit 1
 generationhex=$(printf '%016x' "$generation")
 reload_backup=$stage/qualified-reload-$oldtag
 case "$mode" in
  --apply-token-hidden-readout|--restore-token-hidden-readout) reload_backup=$reload_backup-$expected;;
  --apply-token-validation|--restore-token-validation|--apply-token-hold-recheck|--restore-token-hold-recheck|--apply-token-root-frontend|--restore-token-root-frontend) reload_backup=$reload_backup-$expected;;
  --apply-mixed-live-validation|--restore-mixed-live-validation|--apply-recurrent-block-validation|--restore-recurrent-block-validation|--apply-recurrent-deadline-validation|--restore-recurrent-deadline-validation|--apply-recurrent-prefix-validation|--restore-recurrent-prefix-validation) reload_backup=$reload_backup-$expected;;
  # DEBUG44 has two byte-distinct images. Preserve the previous tag-only
  # backup; never overwrite it or confuse it with this exact input image.
  --apply-decoder-validation|--restore-decoder-validation|--apply-gdn-input-validation|--restore-gdn-input-validation|--apply-gdn-recurrent-validation|--restore-gdn-recurrent-validation|--apply-gdn-rendezvous-validation|--restore-gdn-rendezvous-validation|--apply-conv-silu-precision-validation|--restore-conv-silu-precision-validation|--apply-resident-output-validation|--restore-resident-output-validation|--apply-resident-layer-validation|--restore-resident-layer-validation) reload_backup=$reload_backup-$expected;;
 esac
 backup "$reload_backup" "$expected"
fi
cp -p "$source" "$incoming"; chmod 755 "$incoming"
exact "$incoming" "$result" && exact "$installed" "$expected" && exact "$guard" "$guardsha" || exit 1
if [ "$mode" = --bootstrap ]; then unloaded || exit 1
else parents_idle && "$client" --quiesce "$oldtag" || exit 1; fi
exact "$incoming" "$result" && exact "$installed" "$expected" || exit 1
# The staging file is OUTSIDE watched drivers/bin. A single rename INTO
# that directory generates Haiku's normal legacy-driver update event.
mv -f "$incoming" "$installed"
sync
exact "$installed" "$result" && exact "$guard" "$guardsha" || exit 1
if [ "$mode" = --bootstrap ]; then
 echo BOOTSTRAP_HOT_RELOAD55_A_INSTALLED_NO_GPU_EXECUTION
else
 "$client" --wait "$newtag" "$generationhex" || { echo STOP_NOT_VERIFIED_NO_AUTOMATIC_ROLLBACK_OR_RESET; exit 1; }
 echo P100A_HOT_SWAP_COMPLETE_NO_REBOOT
fi
