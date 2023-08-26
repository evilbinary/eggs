static const AVFilter * const filter_list[] = {
    &ff_af_abench,
    &ff_af_acompressor,
    &ff_af_acontrast,
    &ff_af_acopy,
    &ff_af_acue,
    &ff_af_acrossfade,
    &ff_af_acrossover,
    &ff_af_acrusher,
    &ff_af_adeclick,
    &ff_af_adeclip,
    &ff_af_adelay,
    &ff_af_adenorm,
    &ff_af_aderivative,
    &ff_af_aecho,
    &ff_af_aemphasis,
    &ff_af_aeval,
    &ff_af_aexciter,
    &ff_af_afade,
    &ff_af_afftdn,
    &ff_af_afftfilt,
    &ff_af_afir,
    &ff_af_aformat,
    &ff_af_afreqshift,
    &ff_af_agate,
    &ff_af_aiir,
    &ff_af_aintegral,
    &ff_af_ainterleave,
    &ff_af_alimiter,
    &ff_af_allpass,
    &ff_af_aloop,
    &ff_af_amerge,
    &ff_af_ametadata,
    &ff_af_amix,
    &ff_af_amultiply,
    &ff_af_anequalizer,
    &ff_af_anlmdn,
    &ff_af_anlms,
    &ff_af_anull,
    &ff_af_apad,
    &ff_af_aperms,
    &ff_af_aphaser,
    &ff_af_aphaseshift,
    &ff_af_apulsator,
    &ff_af_arealtime,
    &ff_af_aresample,
    &ff_af_areverse,
    &ff_af_arnndn,
    &ff_af_aselect,
    &ff_af_asendcmd,
    &ff_af_asetnsamples,
    &ff_af_asetpts,
    &ff_af_asetrate,
    &ff_af_asettb,
    &ff_af_ashowinfo,
    &ff_af_asidedata,
    &ff_af_asoftclip,
    &ff_af_asplit,
    &ff_af_astats,
    &ff_af_astreamselect,
    &ff_af_asubboost,
    &ff_af_asubcut,
    &ff_af_asupercut,
    &ff_af_asuperpass,
    &ff_af_asuperstop,
    &ff_af_atempo,
    &ff_af_atrim,
    &ff_af_axcorrelate,
    &ff_af_bandpass,
    &ff_af_bandreject,
    &ff_af_bass,
    &ff_af_biquad,
    &ff_af_channelmap,
    &ff_af_channelsplit,
    &ff_af_chorus,
    &ff_af_compand,
    &ff_af_compensationdelay,
    &ff_af_crossfeed,
    &ff_af_crystalizer,
    &ff_af_dcshift,
    &ff_af_deesser,
    &ff_af_drmeter,
    &ff_af_dynaudnorm,
    &ff_af_earwax,
    &ff_af_ebur128,
    &ff_af_equalizer,
    &ff_af_extrastereo,
    &ff_af_firequalizer,
    &ff_af_flanger,
    &ff_af_haas,
    &ff_af_hdcd,
    &ff_af_headphone,
    &ff_af_highpass,
    &ff_af_highshelf,
    &ff_af_join,
    &ff_af_loudnorm,
    &ff_af_lowpass,
    &ff_af_lowshelf,
    &ff_af_mcompand,
    &ff_af_pan,
    &ff_af_replaygain,
    &ff_af_sidechaincompress,
    &ff_af_sidechaingate,
    &ff_af_silencedetect,
    &ff_af_silenceremove,
    &ff_af_speechnorm,
    &ff_af_stereotools,
    &ff_af_stereowiden,
    &ff_af_superequalizer,
    &ff_af_surround,
    &ff_af_treble,
    &ff_af_tremolo,
    &ff_af_vibrato,
    &ff_af_volume,
    &ff_af_volumedetect,
    &ff_asrc_aevalsrc,
    &ff_asrc_afirsrc,
    &ff_asrc_anoisesrc,
    &ff_asrc_anullsrc,
    &ff_asrc_hilbert,
    &ff_asrc_sinc,
    &ff_asrc_sine,
    &ff_asink_anullsink,
    &ff_vf_addroi,
    &ff_vf_alphaextract,
    &ff_vf_alphamerge,
    &ff_vf_amplify,
    &ff_vf_atadenoise,
    &ff_vf_avgblur,
    &ff_vf_bbox,
    &ff_vf_bench,
    &ff_vf_bilateral,
    &ff_vf_bitplanenoise,
    &ff_vf_blackdetect,
    &ff_vf_blackframe,
    &ff_vf_blend,
    &ff_vf_boxblur,
    &ff_vf_bwdif,
    &ff_vf_cas,
    &ff_vf_chromahold,
    &ff_vf_chromakey,
    &ff_vf_chromanr,
    &ff_vf_chromashift,
    &ff_vf_ciescope,
    &ff_vf_codecview,
    &ff_vf_colorbalance,
    &ff_vf_colorchannelmixer,
    &ff_vf_colorcontrast,
    &ff_vf_colorcorrect,
    &ff_vf_colorize,
    &ff_vf_colorkey,
    &ff_vf_colorhold,
    &ff_vf_colorlevels,
    &ff_vf_colormatrix,
    &ff_vf_colorspace,
    &ff_vf_colortemperature,
    &ff_vf_convolution,
    &ff_vf_convolve,
    &ff_vf_copy,
    &ff_vf_cover_rect,
    &ff_vf_crop,
    &ff_vf_cropdetect,
    &ff_vf_cue,
    &ff_vf_curves,
    &ff_vf_datascope,
    &ff_vf_dblur,
    &ff_vf_dctdnoiz,
    &ff_vf_deband,
    &ff_vf_deblock,
    &ff_vf_decimate,
    &ff_vf_deconvolve,
    &ff_vf_dedot,
    &ff_vf_deflate,
    &ff_vf_deflicker,
    &ff_vf_dejudder,
    &ff_vf_delogo,
    &ff_vf_derain,
    &ff_vf_deshake,
    &ff_vf_despill,
    &ff_vf_detelecine,
    &ff_vf_dilation,
    &ff_vf_displace,
    &ff_vf_dnn_processing,
    &ff_vf_doubleweave,
    &ff_vf_drawbox,
    &ff_vf_drawgraph,
    &ff_vf_drawgrid,
    &ff_vf_edgedetect,
    &ff_vf_elbg,
    &ff_vf_entropy,
    &ff_vf_epx,
    &ff_vf_eq,
    &ff_vf_erosion,
    &ff_vf_estdif,
    &ff_vf_exposure,
    &ff_vf_extractplanes,
    &ff_vf_fade,
    &ff_vf_fftdnoiz,
    &ff_vf_fftfilt,
    &ff_vf_field,
    &ff_vf_fieldhint,
    &ff_vf_fieldmatch,
    &ff_vf_fieldorder,
    &ff_vf_fillborders,
    &ff_vf_find_rect,
    &ff_vf_floodfill,
    &ff_vf_format,
    &ff_vf_fps,
    &ff_vf_framepack,
    &ff_vf_framerate,
    &ff_vf_framestep,
    &ff_vf_freezedetect,
    &ff_vf_freezeframes,
    &ff_vf_fspp,
    &ff_vf_gblur,
    &ff_vf_geq,
    &ff_vf_gradfun,
    &ff_vf_graphmonitor,
    &ff_vf_greyedge,
    &ff_vf_haldclut,
    &ff_vf_hflip,
    &ff_vf_histeq,
    &ff_vf_histogram,
    &ff_vf_hqdn3d,
    &ff_vf_hqx,
    &ff_vf_hstack,
    &ff_vf_hue,
    &ff_vf_hwdownload,
    &ff_vf_hwmap,
    &ff_vf_hwupload,
    &ff_vf_hysteresis,
    &ff_vf_identity,
    &ff_vf_idet,
    &ff_vf_il,
    &ff_vf_inflate,
    &ff_vf_interlace,
    &ff_vf_interleave,
    &ff_vf_kerndeint,
    &ff_vf_kirsch,
    &ff_vf_lagfun,
    &ff_vf_lenscorrection,
    &ff_vf_limiter,
    &ff_vf_loop,
    &ff_vf_lumakey,
    &ff_vf_lut,
    &ff_vf_lut1d,
    &ff_vf_lut2,
    &ff_vf_lut3d,
    &ff_vf_lutrgb,
    &ff_vf_lutyuv,
    &ff_vf_maskedclamp,
    &ff_vf_maskedmax,
    &ff_vf_maskedmerge,
    &ff_vf_maskedmin,
    &ff_vf_maskedthreshold,
    &ff_vf_maskfun,
    &ff_vf_mcdeint,
    &ff_vf_median,
    &ff_vf_mergeplanes,
    &ff_vf_mestimate,
    &ff_vf_metadata,
    &ff_vf_midequalizer,
    &ff_vf_minterpolate,
    &ff_vf_mix,
    &ff_vf_monochrome,
    &ff_vf_mpdecimate,
    &ff_vf_msad,
    &ff_vf_negate,
    &ff_vf_nlmeans,
    &ff_vf_nnedi,
    &ff_vf_noformat,
    &ff_vf_noise,
    &ff_vf_normalize,
    &ff_vf_null,
    &ff_vf_oscilloscope,
    &ff_vf_overlay,
    &ff_vf_owdenoise,
    &ff_vf_pad,
    &ff_vf_palettegen,
    &ff_vf_paletteuse,
    &ff_vf_perms,
    &ff_vf_perspective,
    &ff_vf_phase,
    &ff_vf_photosensitivity,
    &ff_vf_pixdesctest,
    &ff_vf_pixscope,
    &ff_vf_pp,
    &ff_vf_pp7,
    &ff_vf_premultiply,
    &ff_vf_prewitt,
    &ff_vf_pseudocolor,
    &ff_vf_psnr,
    &ff_vf_pullup,
    &ff_vf_qp,
    &ff_vf_random,
    &ff_vf_readeia608,
    &ff_vf_readvitc,
    &ff_vf_realtime,
    &ff_vf_remap,
    &ff_vf_removegrain,
    &ff_vf_removelogo,
    &ff_vf_repeatfields,
    &ff_vf_reverse,
    &ff_vf_rgbashift,
    &ff_vf_roberts,
    &ff_vf_rotate,
    &ff_vf_sab,
    &ff_vf_scale,
    &ff_vf_scale2ref,
    &ff_vf_scdet,
    &ff_vf_scroll,
    &ff_vf_select,
    &ff_vf_selectivecolor,
    &ff_vf_sendcmd,
    &ff_vf_separatefields,
    &ff_vf_setdar,
    &ff_vf_setfield,
    &ff_vf_setparams,
    &ff_vf_setpts,
    &ff_vf_setrange,
    &ff_vf_setsar,
    &ff_vf_settb,
    &ff_vf_shear,
    &ff_vf_showinfo,
    &ff_vf_showpalette,
    &ff_vf_shuffleframes,
    &ff_vf_shufflepixels,
    &ff_vf_shuffleplanes,
    &ff_vf_sidedata,
    &ff_vf_signalstats,
    &ff_vf_signature,
    &ff_vf_smartblur,
    &ff_vf_sobel,
    &ff_vf_split,
    &ff_vf_spp,
    &ff_vf_sr,
    &ff_vf_ssim,
    &ff_vf_stereo3d,
    &ff_vf_streamselect,
    &ff_vf_super2xsai,
    &ff_vf_swaprect,
    &ff_vf_swapuv,
    &ff_vf_tblend,
    &ff_vf_telecine,
    &ff_vf_thistogram,
    &ff_vf_threshold,
    &ff_vf_thumbnail,
    &ff_vf_tile,
    &ff_vf_tinterlace,
    &ff_vf_tlut2,
    &ff_vf_tmedian,
    &ff_vf_tmidequalizer,
    &ff_vf_tmix,
    &ff_vf_tonemap,
    &ff_vf_tpad,
    &ff_vf_transpose,
    &ff_vf_trim,
    &ff_vf_unpremultiply,
    &ff_vf_unsharp,
    &ff_vf_untile,
    &ff_vf_uspp,
    &ff_vf_v360,
    &ff_vf_vaguedenoiser,
    &ff_vf_vectorscope,
    &ff_vf_vflip,
    &ff_vf_vfrdet,
    &ff_vf_vibrance,
    &ff_vf_vif,
    &ff_vf_vignette,
    &ff_vf_vmafmotion,
    &ff_vf_vstack,
    &ff_vf_w3fdif,
    &ff_vf_waveform,
    &ff_vf_weave,
    &ff_vf_xbr,
    &ff_vf_xfade,
    &ff_vf_xmedian,
    &ff_vf_xstack,
    &ff_vf_yadif,
    &ff_vf_yaepblur,
    &ff_vf_zoompan,
    &ff_vsrc_allrgb,
    &ff_vsrc_allyuv,
    &ff_vsrc_cellauto,
    &ff_vsrc_color,
    &ff_vsrc_gradients,
    &ff_vsrc_haldclutsrc,
    &ff_vsrc_life,
    &ff_vsrc_mandelbrot,
    &ff_vsrc_mptestsrc,
    &ff_vsrc_nullsrc,
    &ff_vsrc_pal75bars,
    &ff_vsrc_pal100bars,
    &ff_vsrc_rgbtestsrc,
    &ff_vsrc_sierpinski,
    &ff_vsrc_smptebars,
    &ff_vsrc_smptehdbars,
    &ff_vsrc_testsrc,
    &ff_vsrc_testsrc2,
    &ff_vsrc_yuvtestsrc,
    &ff_vsink_nullsink,
    &ff_avf_abitscope,
    &ff_avf_adrawgraph,
    &ff_avf_agraphmonitor,
    &ff_avf_ahistogram,
    &ff_avf_aphasemeter,
    &ff_avf_avectorscope,
    &ff_avf_concat,
    &ff_avf_showcqt,
    &ff_avf_showfreqs,
    &ff_avf_showspatial,
    &ff_avf_showspectrum,
    &ff_avf_showspectrumpic,
    &ff_avf_showvolume,
    &ff_avf_showwaves,
    &ff_avf_showwavespic,
    &ff_vaf_spectrumsynth,
    &ff_avsrc_amovie,
    &ff_avsrc_movie,
    &ff_af_afifo,
    &ff_vf_fifo,
    &ff_asrc_abuffer,
    &ff_vsrc_buffer,
    &ff_asink_abuffer,
    &ff_vsink_buffer,
    NULL };
