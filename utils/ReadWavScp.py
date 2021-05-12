from __future__ import print_function
if True: ## imports / admin
    import os,sys,pdb # pdb.set_trace()
    sys.path.insert(0, 'utils')
    from _helper_basics_ import *
    import _Shazam_ as Shazam
    import conf
    #
    START_TIME=datetime.datetime.now()
    datetime.datetime.now() - START_TIME
    print(f"===========\npython {' '.join(sys.argv)}\n    Start_Time:{START_TIME}\n===========")
    #
    print('############ Printing Config Params ############')
    if True:
        conf_hash=conf.Shazam_conf(); str(conf_hash)
        conf_sr=conf.SR_conf(); str(conf_sr)
        db_wavscp=sys.argv[1]
        pp = pprint.PrettyPrinter(indent=4)
    print('############ End of Config Params ##############')

Dur_read_audio = []
Dur_wav2LPS = []
Dur_shashing = []
Dur_addLUT = []

def wav2LPS_speedtest(x_in, spect_det=None, mode="librosa",
    n_fft=None, win_length=None, hop_length=None, nfft=None, winstep=None, winlen=None,
    pad_mode=False, fs=None, center_in=True, _window='hann'):
    if mode == "librosa":
        x_tmp = x_in+0.
        x_stft = librosa.stft(y=x_tmp, n_fft=n_fft, win_length=win_length, hop_length=hop_length, center=center_in, window=_window)
    return np.abs(x_stft)

def report_stats(listin):
    print(f"Total   time taken : {sum(listin)}")
    print(f"Average time taken : {statistics.mean(listin)}")

kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)

# nfft=256
nfft=512
kwargs['kwargs_STFT']
kwargs['kwargs_STFT']['pad_mode']   = True
kwargs['kwargs_STFT']['mode']       = 'librosa'
kwargs['kwargs_STFT']['n_fft']      = nfft
kwargs['kwargs_STFT']['win_length'] = nfft
kwargs['kwargs_STFT']['hop_length'] = nfft//2
kwargs['kwargs_STFT']['nfft']       = None
kwargs['kwargs_STFT']['winstep']    = None
kwargs['kwargs_STFT']['winlen']     = None
kwargs['kwargs_STFT']['fs']         = 8000
kwargs['kwargs_peaks']
# 10    : 676
# 7     : 1869
# 5     : 1869
kwargs['kwargs_peaks']['f_dim1']   = 5
kwargs['kwargs_peaks']['t_dim1']   = 5
kwargs['kwargs_peaks']['threshold_abs']   = .25

set(kwargs)
1, pp.pprint(kwargs['kwargs_hashPeaks'])
1, pp.pprint(kwargs['kwargs_STFT'])
1, pp.pprint(kwargs['kwargs_peaks'])

LUT={}
for uttid2wavpath in open(db_wavscp, 'r'):
    utt, wav_path=uttid2wavpath.strip('\n').split('\t')
    # utt, wav_path=uttid2wavpath.strip('\n').split(' ')
    # print(wav_path)

    if 1 : ## Read Audio
        start = time.perf_counter()
        x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False) # 0.04155487194657326
        # x_wav,sr_out=read_audio(wav_path, mode='soundfile', sr=conf_sr.sr, mean_norm=False) # 0.0427104695700109
        # x_wav,sr_out=read_audio(wav_path, mode='librosa', sr=conf_sr.sr, mean_norm=False) # 0.04620204586535692
        end = time.perf_counter()
        Dur_read_audio.append(end - start)

    ## Shorten Duration
    # x_wav = x_wav[:5*8000]
    # x_wav = x_wav[:20*8000]
    # print(f"\t dur={len(x_wav)/sr_out}")
    print(f"\t len(x_wav)={len(x_wav)}")

    if 1 : ## Perform STFT
        start = time.perf_counter()
        x_MAG=wav2LPS_speedtest(x_wav, **kwargs['kwargs_STFT'])
        end = time.perf_counter()
        Dur_wav2LPS.append(end - start)

    if 1 : ## Perform Shash
        start = time.perf_counter()
        x_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
        end = time.perf_counter()
        Dur_shashing.append(end - start)

    if 1 : ## Add to LUT
        start = time.perf_counter()
        LUT = Shazam.addLUT(LUT, x_shash, utt)
        end = time.perf_counter()
        Dur_addLUT.append(end - start)
    print(f"\t len(x_shash[0])={len(x_shash[0])} | len(LUT)={len(LUT)}")

print(f"\t dur={len(x_wav)/sr_out}")
print(f"\t len(x_shash[0])={len(x_shash[0])} | len(LUT)={len(LUT)}")

print("\nDur_read_audio"); report_stats(Dur_read_audio)
print("\nDur_wav2LPS"); report_stats(Dur_wav2LPS)
print("\nDur_shashing"); report_stats(Dur_shashing)
print("\nDur_addLUT"); report_stats(Dur_addLUT)
# pdb.set_trace()
print(f"\
{statistics.mean(Dur_read_audio):.6f}\t\
{statistics.mean(Dur_wav2LPS):.6f}\t\
{statistics.mean(Dur_shashing):.6f}\t\
{statistics.mean(Dur_addLUT):.6f}\t\
")

#################################################################
END_TIME=datetime.datetime.now()
print(f"===========\
    \nDone \
    \npython {' '.join(sys.argv)}\
    \nStart_Time  :{START_TIME}\
    \nEnd_Time    :{END_TIME}\
    \nDuration    :{END_TIME-START_TIME}\
\n===========")

"""
!import code; code.interact(local=vars())
"""
