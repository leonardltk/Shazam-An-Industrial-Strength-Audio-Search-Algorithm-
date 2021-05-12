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
    python_file_verbose = ' '.join(sys.argv)
    python_file_verbose = python_file_verbose.replace(' ','\n\t')
    print(f"===========\npython {python_file_verbose}\n    Start_Time:{START_TIME}\n===========")
    #
    print('############ Printing Config Params ############')
    if True:
        conf_hash=conf.Shazam_conf(); # str(conf_hash)
        conf_sr=conf.SR_conf(); # str(conf_sr)
        DB_type=sys.argv[1]
        qry_shash_dir=sys.argv[2]
        qry2db=sys.argv[3]
        db_wavscp=sys.argv[4]
        qry_wavdir=sys.argv[5]
        pp = pprint.PrettyPrinter(indent=4)
    print('############ End of Config Params ##############')
#################################################################
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
#################################################################

Dur_read_audio = []
Dur_wav2LPS = []
Dur_shashing = []
Dur_findTimePairs = []
Dur_ComputeHistogram = []

#################################################################

kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)

kwargs['kwargs_STFT']['n_fft']   = 512
kwargs['kwargs_STFT']['nfft']   = 512
kwargs['kwargs_STFT']['win_length']   = 512
kwargs['kwargs_STFT']['hop_length']   = 512//2

kwargs['kwargs_peaks']['f_dim1']   = 15
kwargs['kwargs_peaks']['t_dim1']   = 15
kwargs['kwargs_peaks']['threshold_abs']   = 3

set(kwargs)
1, pp.pprint(kwargs['kwargs_hashPeaks'])
1, pp.pprint(kwargs['kwargs_STFT'])
1, pp.pprint(kwargs['kwargs_peaks'])
#################################################################

print('========== Read db =================')
conf_db=conf.DB_conf(DB_type)
if os.path.exists(conf_db.exp_utt2shash_DICT) : ## Read db
    utt2shash_DICT_DB = dump_load_pickle(conf_db.exp_utt2shash_DICT, "load")
else:
    dbwavdir = f"./wav/{DB_type}"
    utt2shash_DICT_DB={}
    for uttdb__wavpath in open(conf_db.wav_scp, 'r'):
        uttdb, wavpath = uttdb__wavpath.strip('\n').split('\t')
        x_wav,sr_out=read_audio(wavpath, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
        x_MAG=wav2LPS_speedtest(x_wav, **kwargs['kwargs_STFT'])
        xhash = Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
        utt2shash_DICT_DB[uttdb] = xhash
        print(f'\t{uttdb} has len {len(xhash[0])}')
    dump_load_pickle(conf_db.exp_utt2shash_DICT, "dump", utt2shash_DICT_DB)
print(f'\tThere are {len(utt2shash_DICT_DB)} hashes')

#################################################################
kwargs['kwargs_peaks']['threshold_abs']   = .25**2
1, pp.pprint(kwargs['kwargs_peaks'])
for qry2db_c in open(qry2db, 'r'):
    qry_utt, db_utt, _ = qry2db_c.strip('\n').split(' ')
    wav_path = os.path.join(qry_wavdir, f"{qry_utt}.raw.mp3")
    if not os.path.exists(wav_path):
        wav_path = os.path.join(qry_wavdir, f"{qry_utt}.wav")

    if 1 : ## Read Audio
        start = time.perf_counter()
        x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False) # 0.04155487194657326
        # x_wav,sr_out=read_audio(wav_path, mode='soundfile', sr=conf_sr.sr, mean_norm=False) # 0.0427104695700109
        # x_wav,sr_out=read_audio(wav_path, mode='librosa', sr=conf_sr.sr, mean_norm=False) # 0.04620204586535692
        end = time.perf_counter()
        Dur_read_audio.append(end - start)

    if 1 : ## Perform STFT
        start = time.perf_counter()
        x_MAG=wav2LPS_speedtest(x_wav, **kwargs['kwargs_STFT'])
        end = time.perf_counter()
        Dur_wav2LPS.append(end - start)

    if 1 : ## Perform Shash
        start = time.perf_counter()
        x_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
        print(f'\t{qry_utt} has len {len(x_shash[0])}')
        end = time.perf_counter()
        Dur_shashing.append(end - start)

    if 1 : ## MAtching
        start = time.perf_counter()
        Time_offsets = Shazam.findTimePairs(utt2shash_DICT_DB[db_utt], x_shash)
        print(f"Time_offsets={len(Time_offsets)}")
        Dur_findTimePairs.append(time.perf_counter() - start)

    if 1 : ## MAtching
        start = time.perf_counter()
        hist, bin_edges = np.histogram(Time_offsets, bins=100)
        print(f"np.max(hist)={np.max(hist)}")
        Dur_ComputeHistogram.append(time.perf_counter() - start)

print("\nDur_read_audio"); report_stats(Dur_read_audio)
print("\nDur_wav2LPS"); report_stats(Dur_wav2LPS)
print("\nDur_shashing"); report_stats(Dur_shashing)
print("\nDur_findTimePairs"); report_stats(Dur_findTimePairs)
print("\nDur_ComputeHistogram"); report_stats(Dur_ComputeHistogram)
# pdb.set_trace()
print(f"\
{statistics.mean(Dur_read_audio):.6f}\t\
{statistics.mean(Dur_wav2LPS):.6f}\t\
{statistics.mean(Dur_shashing):.6f}\t\
{statistics.mean(Dur_findTimePairs):.6f}\t\
{statistics.mean(Dur_ComputeHistogram):.6f}\t\
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
conda activate Py3_debug

python -u UnitTesting/MatchingTimer.py \
  $DB_type \
  $qry_shash_dir \
  ./data/sample_3_qry/thresholdSearch/qry2db.v1.3808s \
  $db_wavscp \
  wav/sample_3_qry \
|& tee $logdir_qry/MatchingTimer.py.log

python -u UnitTesting/MatchingTimer.py \
  $DB_type \
  $qry_shash_dir \
  $qry2db \
  $db_wavscp \
  wav/2Million \
|& tee $logdir_qry/MatchingTimer.py.log

tail -n40 $logdir_qry/MatchingTimer.* | less

"""
