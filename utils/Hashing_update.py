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
        parser=argparse.ArgumentParser(
            description="Updating hashes of current database.\
            \n\tFor -add : \
                accepts a path with text format same as wav.scp, \
                to indicate which uttid and its respective wave path to add to the database,\
                existing uttid will be replaced\
            \n\tFor -delete : \
                accepts a path with text format same as wav.scp, \
                2nd column (wave path) can be empty, as it will be ignored.\
                existing uttid names will be removed,\
                non-existing uttid names will be ignored.\
                ")
        parser.add_argument('-DB_type')
        parser.add_argument('-add', type=str, default=None)
        parser.add_argument('-delete', type=str, default=None)
        # 
        args=parser.parse_args()
        DB_type=args.DB_type
        if (args.add is None) and (args.delete is None):
            raise Exception("ArgumentError : indicate either -add or -delete with the desired path")
        # 
        conf_db=conf.DB_conf(DB_type); str(conf_db)
        conf_hash=conf.Shazam_conf(); str(conf_hash)
        conf_sr=conf.SR_conf(); str(conf_sr)
        # 
        pp = pprint.PrettyPrinter(indent=4)
    print('############ End of Config Params ##############')
#################################################################
if True:
    print('============== Read current database hashes ==============')
    utt2wavpath_DICT_DB = dump_load_pickle(conf_db.exp_wavscp_dict, "load")
    utt2hashpath_DICT_DB=dump_load_pickle(conf_db.exp_utt2hashpath_DICT,"load")
    utt2shash_DICT_DB=dump_load_pickle(conf_db.exp_utt2shash_DICT,"load")
    LUT=dump_load_pickle(conf_db.exp_LUT, "load")
    print(f"\tLoaded {conf_db.exp_wavscp_dict}, there are {len(utt2wavpath_DICT_DB)} wave paths in {conf_db.wav_scp}")
    print(f"\tLoaded {conf_db.exp_utt2hashpath_DICT}, there are {len(utt2hashpath_DICT_DB)} shash paths in")
    print(f"\tLoaded {conf_db.exp_utt2shash_DICT}, there are {len(utt2shash_DICT_DB)} shashes")
    print(f"\tLoaded {conf_db.exp_LUT}, there are {len(LUT)} elements")

#################################################################
if   args.add is not None:
    print(f'============== Adding new hashes from {args.add} ==============')
    kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)
    print("Updating to ",conf_db.shash_scp)
    flag_err = False
    for utt__wavpath in open(args.add, 'r'):
        utt, wav_path = utt__wavpath.strip('\n').split('\t')
        if (utt in utt2wavpath_DICT_DB) or (utt in utt2hashpath_DICT_DB) or (utt in utt2shash_DICT_DB):
            print(f'\tutt={utt}\texists, overwriting with wav_path={wav_path}\t...')
        else:
            print(f'\tutt={utt}\tis new, adding with wav_path={wav_path}\t...')
        hash_path = os.path.join(conf_db.exp_hashdir, utt+".shash")
        try : 
            ## Read Audio
            x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
            ## Hash Audio
            x_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
        except:
            print(); traceback.print_exc(); print()
            flag_err = True
            continue
        ## Save to file
        dump_load_pickle(hash_path, "dump", x_shash)
        ## Update hash dicts
        utt2wavpath_DICT_DB[utt]=wav_path
        utt2hashpath_DICT_DB[utt]=hash_path
        utt2shash_DICT_DB[utt]=x_shash
        ## Add to LUT
        LUT = Shazam.addLUT(LUT, x_shash, utt)

elif args.delete is not None:
    print(f'============== Removing hashes as specified in {args.delete} ==============')
    print("Updating to ",conf_db.shash_scp)
    flag_err = False
    for utt__wavpath in open(args.delete, 'r'):
        utt, wav_path = utt__wavpath.strip('\n').split('\t')
        if (utt in utt2wavpath_DICT_DB) or (utt in utt2hashpath_DICT_DB) or (utt in utt2shash_DICT_DB):
            print(f"\tutt={utt}\texists, removing...")
        else:
            print(f"\tutt={utt}\tdont exists, skipping...")
            continue

        ## Update hash dicts
        if utt in utt2wavpath_DICT_DB : utt2wavpath_DICT_DB.pop(utt)
        if utt in utt2hashpath_DICT_DB : utt2hashpath_DICT_DB.pop(utt)
        if utt in utt2shash_DICT_DB : 
            x_shash = utt2shash_DICT_DB.pop(utt)
            LUT = Shazam.removeLUT(LUT, x_shash, utt)

if True: ## Updating the database files
    if True : # Make a backup
        backupdir = os.path.join(conf_db.data_dir,'.backup'); os.makedirs(backupdir, exist_ok=True)
        for currfile in [conf_db.wav_scp, conf_db.shash_scp]:
            bn=os.path.basename(currfile)
            outputfile = shutil.copy( currfile, os.path.join(backupdir,bn))
            print(f'Backed up {currfile} -> {outputfile}')
    # 'data/*/shash.scp'
    with open(conf_db.wav_scp,"w") as f_w:
        for utt,wav_path in utt2wavpath_DICT_DB.items():
            f_w.write("{}\t{}\n".format(utt,wav_path))
        print(f"Updated {conf_db.wav_scp}")
    # 'data/*/shash.scp'
    with open(conf_db.shash_scp,"w") as f_w:
        for utt,hash_path in utt2hashpath_DICT_DB.items():
            f_w.write("{}\t{}\n".format(utt,hash_path))
        print(f"Updated {conf_db.shash_scp}")

    print()
    dumplog=dump_load_pickle(conf_db.exp_wavscp_dict,"dump", utt2wavpath_DICT_DB);          print(f"Updated {dumplog}, there are {len(utt2wavpath_DICT_DB)} wave paths in {conf_db.wav_scp}")
    dumplog=dump_load_pickle(conf_db.exp_utt2hashpath_DICT,"dump", utt2hashpath_DICT_DB);   print(f"Updated {dumplog}, there are {len(utt2hashpath_DICT_DB)} shash paths in")
    dumplog=dump_load_pickle(conf_db.exp_utt2shash_DICT,"dump", utt2shash_DICT_DB);         print(f"Updated {dumplog}, there are {len(utt2shash_DICT_DB)} shashes")
    dumplog=dump_load_pickle(conf_db.exp_LUT, "dump", LUT);                                 print(f"Updated {dumplog}, there are {len(LUT)} elements")

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
