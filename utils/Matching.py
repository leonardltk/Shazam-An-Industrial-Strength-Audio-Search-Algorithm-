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
        parser.add_argument('-Qry_type')
        parser.add_argument('-jid')
        parser.add_argument('-cont', default=False)
        # 
        args=parser.parse_args()
        DB_type=args.DB_type
        Qry_type=args.Qry_type
        jid=args.jid
        cont=args.cont
        if cont=='True' : cont=True
        elif cont=='False' : cont=False
        # 
        conf_db=conf.DB_conf(DB_type)
        conf_qry=conf.QRY_conf(Qry_type, jid)
        conf_hash=conf.Shazam_conf()
        conf_sr=conf.SR_conf()
        if not cont:
            str(conf_db)
            str(conf_qry)
            str(conf_hash)
            str(conf_sr)
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
def milestone():
    if idx%100 == 1 :
        checkpoint_saving()
        print(f"\tTime taken so far : {datetime.datetime.now()-START_TIME}")
def checkpoint_saving():
    dump_load_pickle(conf_qry.split_Unmatched_Dict, "dump", Unmatched_Dict)
    dump_load_pickle(conf_qry.split_Matched_Dict, "dump", Matched_Dict)
    dump_load_pickle(conf_qry.split_Matched_Shash_Dict, "dump", Matched_Shash_Dict)
    # 
    with open(conf_qry.split_qry2db,"w") as f_w:
        for utt,MatchDet_c in Matched_Dict.items():
            f_w.write(f"{utt} {MatchDet_c['utt_DB']}\n")

#################################################################
print('========== Read db =================')
if True : ## Read db
    ## Read hash.scp
    ## Load hashes & LUT
    if cont:
        utt2hashpath_DICT_DB = dump_load_pickle(conf_db.exp_utt2hashpath_DICT,"load")
        utt2shash_DICT_DB = dump_load_pickle(conf_db.exp_utt2shash_DICT,"load")
    else:
        utt2hashpath_DICT_DB = dict( (i.replace('\n','').split(' ')) for i in open(conf_db.shash_scp,'r') )
        dump_load_pickle(conf_db.exp_utt2hashpath_DICT,"dump", utt2hashpath_DICT_DB)
        utt2shash_DICT_DB = dict( (utt,dump_load_pickle(hash_path, "load")) for utt,hash_path in utt2hashpath_DICT_DB.items() )
        dump_load_pickle(conf_db.exp_utt2shash_DICT,"dump", utt2shash_DICT_DB)
    print(f'\tThere are {len(utt2hashpath_DICT_DB)} files in {conf_db.shash_scp}')
    print(f'\tThere are {len(utt2shash_DICT_DB)} hashes')
    LUT = dump_load_pickle(conf_db.exp_LUT, "load")
    print(f'\tThere are {len(LUT)} keys in LUT')
print('========== Read qry =================')
if True : ## Read qry
    ## Read split/wav.scp
    if cont:
        utt2wavpath_DICT_qry = dump_load_pickle(conf_qry.split_wavscp_dict, "load")
        #
        print("\tContinue Updating ",conf_qry.split_qry2db)
        Unmatched_Dict = dump_load_pickle(conf_qry.split_Unmatched_Dict, "load")
        Matched_Dict = dump_load_pickle(conf_qry.split_Matched_Dict, "load")
        Matched_Shash_Dict = dump_load_pickle(conf_qry.split_Matched_Shash_Dict, "load")
    else:
        utt2wavpath_DICT_qry = dict( (i.replace('\n','').split(' ')) for i in open(conf_qry.split_wav_scp,'r') )
        dump_load_pickle(conf_qry.split_wavscp_dict, "dump", utt2wavpath_DICT_qry)
        #
        print("\tWriting to ",conf_qry.split_qry2db)
        Unmatched_Dict = {}
        Matched_Dict = {}
        Matched_Shash_Dict = {}
    print(f'\tThere are {len(utt2wavpath_DICT_qry)} files in {conf_qry.split_wav_scp}')
    print(f"\tUnmatched_Dict     has length ={len(Unmatched_Dict)}")
    print(f"\tMatched_Dict       has length ={len(Matched_Dict)}")
    print(f"\tMatched_Shash_Dict has length ={len(Matched_Shash_Dict)}")

#################################################################
print('========== Matching =================')
kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)
for idx,utt__wav_path in enumerate(utt2wavpath_DICT_qry.items(), 1):
    utt,wav_path = utt__wav_path
    if utt in Unmatched_Dict : continue
    print(f" {idx}/{len(utt2wavpath_DICT_qry)} utt={utt},wav_path={wav_path}")
    ## Read shash
    try : 
        # hash_path = os.path.join(conf_qry.exp_hashdir, utt+".shash")
        # qry_shash = dump_load_pickle(hash_path, "load")
        ## Read Audio
        x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
        ## Hash Audio
        qry_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])
        ## Save to file
        # dump_load_pickle(hash_path, "dump", qry_shash)
        if qry_shash[0] is None : 
            print(f"\t{utt} has no hash")
            Unmatched_Dict[utt] = {'remarks':f'{utt} : {wav_path} has no shash extractable, possibly due to silence'}
            milestone()
            continue
    except subprocess.CalledProcessError: 
        print(f"\t{utt} has corrupted mp3")
        Unmatched_Dict[utt] = {'remarks':f'{utt} : {wav_path} has corrupted mp3'}
        milestone()
        continue

    ## Matching
    CandidateSet = Shazam.searchLUT(LUT, qry_shash, utt)
    max_so_far=-1
    for utt_DB, num_union in CandidateSet:
        shash_DB = utt2shash_DICT_DB[utt_DB]
        num_matches=Shazam.count_number_match(shash_DB, qry_shash, bins=100, N=1, verbose=0)
        # Append to Matched
        if num_matches > conf_hash.th:
            print(f"\t{utt} matched with {utt_DB}")
            Matched_Dict[utt] = {'utt_DB':utt_DB, 'num_matches':num_matches, 'num_union':num_union}
            Matched_Shash_Dict[utt] = qry_shash
            break
        # Append to Unmatched (if there exist a match, this will show the 2nd best match)
        if num_matches > max_so_far:
            max_so_far = num_matches
            Unmatched_Dict[utt] = {'utt_DB':utt_DB, 'num_matches':num_matches, 'num_union':num_union}

    ## Milestone
    if not utt in Unmatched_Dict:
        Unmatched_Dict[utt] = {'remarks':f'{utt} : {wav_path} has no candidates from DB_type={conf_db.DB_type}'}
    milestone()

#################################################################
print('========== Done =================')
checkpoint_saving()
print(f"\tUnmatched_Dict     has length ={len(Unmatched_Dict)}")
print(f"\tMatched_Dict       has length ={len(Matched_Dict)}")
print(f"\tMatched_Shash_Dict has length ={len(Matched_Shash_Dict)}")
print(f"\tWritten to {conf_qry.split_Unmatched_Dict}")
print(f"\tWritten to {conf_qry.split_Matched_Dict}")
print(f"\tWritten to {conf_qry.split_Matched_Shash_Dict}")
print(f"\tWritten to {conf_qry.split_qry2db}")

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
