from __future__ import print_function
if 1: ## imports / admin
    import os,sys,pdb # pdb.set_trace()
    sys.path.insert(0, 'utils')
    from _helper_basics_ import *
    import _Shazam_ as Shazam
    # 
    START_TIME=datetime.datetime.now()
    datetime.datetime.now() - START_TIME
    print("===========\npython {}\n Start_Time:{}\n===========".format(' '.join(sys.argv),str( START_TIME )))
    # 
    print('############ Printing Config Params ############')
    if True:
        import argparse
        parser=argparse.ArgumentParser()
        parser.add_argument('-conf_qry')
        parser.add_argument('-conf_sr')
        parser.add_argument('-conf_hash')
        # 
        args=parser.parse_args()
        conf_qry=get_conf(args.conf_qry)
        conf_sr=get_conf(args.conf_sr)
        conf_hash=get_conf(args.conf_hash)
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
## Read wav.scp
utt2wavpath_DICT = dict( (i.replace('\n','').split(' ')) for i in open(conf_qry.wav_scp,'r') )

#################################################################
""" Write hash to conf_qry.exp_hashdir
"""
print('==============================================')
os.makedirs(conf_qry.exp_hashdir,exist_ok=True)
kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)
print("\tWriting to ",conf_qry.shash_scp)
f_w = open(conf_qry.shash_scp,"w")
for utt,wav_path in utt2wavpath_DICT.items():
    ## Read Audio
    x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
    ## Hash Audio
    x_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
    ## Save to file
    hash_path = os.path.join(conf_qry.exp_hashdir, utt+".shash")
    dump_load_pickle(hash_path, "dump", x_shash)
    ## Write to hash_scp
    f_w.write("{} {}\n".format(utt,hash_path))
f_w.close()
print("\tWritten to ",conf_qry.shash_scp)

#################################################################
END_TIME=datetime.datetime.now()
print("===========\n Done python {}\n Start_Time:{}\n   End_Time:{}\n Duration  :{}\n===========".format(
    ' '.join(sys.argv),
    str( START_TIME ),
    str( END_TIME ),
    str( END_TIME-START_TIME ),
    ))

"""
!import code; code.interact(local=vars())

1, pp.pprint(kwargs)
"""
