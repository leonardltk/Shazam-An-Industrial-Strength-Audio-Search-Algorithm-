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
        import argparse
        parser=argparse.ArgumentParser()
        parser.add_argument('-DB_type')
        parser.add_argument('-overwrite', type=int, default=0, help='Set 1 to overwrite shash even if it exists. defaults to 0')
        # 
        args=parser.parse_args()
        DB_type=args.DB_type
        overwrite=args.overwrite
        # 
        conf_db=conf.DB_conf(DB_type); str(conf_db)
        conf_hash=conf.Shazam_conf(); str(conf_hash)
        conf_sr=conf.SR_conf(); str(conf_sr)
        # 
        pp = pprint.PrettyPrinter(indent=4)
    print('############ End of Config Params ##############')


class Timer:
    def __init__(self, List_in, description):
        self.List_in = List_in
        self.description = description
    def __enter__(self):
        self.start = time.perf_counter()
        return self
    def __exit__(self, *args):
        self.duration = time.perf_counter() - self.start
        self.List_in.append(self.duration)
        print(f"\tThe time for '{self.description}' took: {self.duration}.")

#################################################################
print('============== Read wav.scp ==============')
utt2wavpath_DICT_DB = dict( (i.strip('\n').split('\t') ) for i in open(conf_db.wav_scp,'r') )
dump_load_pickle(conf_db.exp_wavscp_dict, "dump", utt2wavpath_DICT_DB)
print(f"Saved to {conf_db.exp_wavscp_dict}, there are {len(utt2wavpath_DICT_DB)} hashes extracted from {conf_db.wav_scp}")
utt2hashpath_DICT_DB={}
utt2shash_DICT_DB={}

#################################################################
print(f'============== Writing shash, LUT ==============')
kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)
print("Writing to ",conf_db.shash_scp)
LUT={}
f_w = open(conf_db.shash_scp,"w")
flag_err = False

Time_ReadAudio = []
Time_Downsample = []
Time_HashAudio = []
Time_SaveHash = []
Time_AddtoLUT = []

for utt,wav_path in utt2wavpath_DICT_DB.items():
    print(f"utt={utt}\twav_path={wav_path}")
    hash_path = os.path.join(conf_db.exp_hashdir, utt+".shash")
    try : 
        ## Read Audio
        # x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
        
        with Timer(Time_ReadAudio, "Read Audio") as t:  
            x_wav, sound_fs = af.read(wav_path)
            if len(x_wav.shape)>1: 
                print('\tmean of multiple channels')
                x_wav = np.mean(x_wav, axis=0)
        
        with Timer(Time_Downsample, "Downsampling") as t:
            if sound_fs!=conf_sr.sr:
                x_wav = resampy.resample(x_wav, sound_fs, conf_sr.sr, axis=0)

        ## Hash Audio
        with Timer(Time_HashAudio, "Hash Audio") as t:  
            x_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])

        ## Save to file
        with Timer(Time_SaveHash, "Save Hash") as t:  
            dump_load_pickle(hash_path, "dump", x_shash)
            print("\tWritten to",hash_path)
    
        ## Update hash dicts
        utt2hashpath_DICT_DB[utt]=hash_path
        utt2shash_DICT_DB[utt]=x_shash
    except:
        print(); traceback.print_exc(); print()
        flag_err = True
        continue
    ## Write to hash_scp
    f_w.write("{}\t{}\n".format(utt,hash_path))
    ## Add to LUT
    with Timer(Time_AddtoLUT, "Add to LUT") as t:  
        LUT = Shazam.addLUT(LUT, x_shash, utt)

    print("\tlen(x_shash[0])=",len(x_shash[0]))
    print(f'\tThere are {len(LUT)} keys in LUT')
    print(f'\tThere are {len(utt2hashpath_DICT_DB)} files in {conf_db.shash_scp}')
    print(f'\tThere are {len(utt2shash_DICT_DB)} hashes')
f_w.close()


for des_LIST, lstname in zip(
    [Time_ReadAudio,Time_Downsample,Time_HashAudio,Time_SaveHash,Time_AddtoLUT],
    ["Time_ReadAudio","Time_Downsample","Time_HashAudio","Time_SaveHash","Time_AddtoLUT"],
    ):
    curr_sum    =sum(des_LIST)
    curr_len    =len(des_LIST)
    curr_min    =min(des_LIST)
    curr_max    =max(des_LIST)
    curr_mean   =statistics.mean(des_LIST)
    curr_median =statistics.median(des_LIST)
    print(f'{lstname} : {curr_len}\tcurr_sum={curr_sum:0.3f}\tcurr_min={curr_min:0.3f}\tcurr_max={curr_max:0.3f}\tcurr_mean={curr_mean:0.3f}\tcurr_median={curr_median:0.3f}\n')

dump_load_pickle(conf_db.exp_LUT, "dump", LUT)
dump_load_pickle(conf_db.exp_utt2hashpath_DICT,"dump", utt2hashpath_DICT_DB)
dump_load_pickle(conf_db.exp_utt2shash_DICT,"dump", utt2shash_DICT_DB)
print(f'\tThere are {len(LUT)} keys in LUT')
print(f'\tThere are {len(utt2hashpath_DICT_DB)} files in {conf_db.shash_scp}')
print(f'\tThere are {len(utt2shash_DICT_DB)} hashes')
print("Written to ",conf_db.shash_scp)
print("Written to ",conf_db.exp_utt2hashpath_DICT)
print("Written to ",conf_db.exp_utt2shash_DICT)
print("Written to ",conf_db.exp_LUT)

#################################################################
if flag_err: print("\n!!! There are some error observed, pls check. !!!\n")

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
