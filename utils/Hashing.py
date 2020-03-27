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
        conf_qry=conf.DB_conf(DB_type); str(conf_qry)
        conf_hash=conf.Shazam_conf(); str(conf_hash)
        conf_sr=conf.SR_conf(); str(conf_sr)
        # 
        pp = pprint.PrettyPrinter(indent=4)
    print('############ End of Config Params ##############')
#################################################################
print('============== Read wav.scp ==============')
utt2wavpath_DICT = dict( (i.replace('\n','').split(' ')) for i in open(conf_qry.wav_scp,'r') )
dump_load_pickle(conf_qry.exp_wavscp_dict, "dump", utt2wavpath_DICT)
print(f"Saved to {conf_qry.exp_wavscp_dict}, there are {len(utt2wavpath_DICT)} hashes extracted from {conf_qry.wav_scp}")

#################################################################
print(f'============== Writing shash, LUT ==============')
kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)
print("\tWriting to ",conf_qry.shash_scp)
LUT={}
f_w = open(conf_qry.shash_scp,"w")
flag_err = False
for utt,wav_path in utt2wavpath_DICT.items():
    print(f"utt={utt},wav_path={wav_path}")
    hash_path = os.path.join(conf_qry.exp_hashdir, utt+".shash")
    try : 
        ## Read Audio
        x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
        ## Hash Audio
        x_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
        ## Save to file
        dump_load_pickle(hash_path, "dump", x_shash)
    except:
        print()
        traceback.print_exc()
        print()
        flag_err = True
        continue
    ## Write to hash_scp
    f_w.write("{} {}\n".format(utt,hash_path))
    ## Add to LUT
    LUT = Shazam.addLUT(LUT, x_shash, utt)
f_w.close()
dump_load_pickle(conf_qry.exp_LUT, "dump", LUT)
print("\tWritten to ",conf_qry.shash_scp)
print("\tWritten to ",conf_qry.exp_LUT)

if flag_err:
    print("\n!!! There are some error observed, pls check. !!!\n")

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
