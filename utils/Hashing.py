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
        # 
        args=parser.parse_args()
        DB_type=args.DB_type
        # 
        conf_db=conf.DB_conf(DB_type); str(conf_db)
        conf_hash=conf.Shazam_conf(); str(conf_hash)
        conf_sr=conf.SR_conf(); str(conf_sr)
        # 
        pp = pprint.PrettyPrinter(indent=4)
    print('############ End of Config Params ##############')
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
for utt,wav_path in utt2wavpath_DICT_DB.items():
    print(f"\tutt={utt}\twav_path={wav_path}")
    hash_path = os.path.join(conf_db.exp_hashdir, utt+".shash")
    try : 
        ## Read Audio
        x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
        ## Hash Audio
        x_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
        ## Save to file
        dump_load_pickle(hash_path, "dump", x_shash)
        ## Update hash dicts
        utt2hashpath_DICT_DB[utt]=hash_path
        utt2shash_DICT_DB[utt]=x_shash
    except:
        print(); traceback.print_exc(); print()
        flag_err = True
        continue
    ## Write to hash_scp
    f_w.write("{} {}\n".format(utt,hash_path))
    ## Add to LUT
    LUT = Shazam.addLUT(LUT, x_shash, utt)
f_w.close()
dump_load_pickle(conf_db.exp_LUT, "dump", LUT)
dump_load_pickle(conf_db.exp_utt2hashpath_DICT,"dump", utt2hashpath_DICT_DB)
dump_load_pickle(conf_db.exp_utt2shash_DICT,"dump", utt2shash_DICT_DB)
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
