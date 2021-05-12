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
        parser.add_argument('-milestone_idx', type=int, default=100)
        # 
        args=parser.parse_args()
        DB_type=args.DB_type
        Qry_type=args.Qry_type
        jid=args.jid
        cont=args.cont
        if cont=='True' : cont=True
        elif cont=='False' : cont=False
        milestone_idx=args.milestone_idx
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
    if idx%milestone_idx == 1 :
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
print('========== Read db =================')
if True : ## Read db
    utt2hashpath_DICT_DB = dump_load_pickle(conf_db.exp_utt2hashpath_DICT,"load")
    print(f'\tThere are {len(utt2hashpath_DICT_DB)} files in {conf_db.shash_scp}')
    utt2shash_DICT_DB    = dump_load_pickle(conf_db.exp_utt2shash_DICT,"load")
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
        utt2wavpath_DICT_qry = dict( (i.replace('\n','').split('\t')) for i in open(conf_qry.split_wav_scp,'r') )
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

Time_ReadAudio = []
Time_Downsample = []
Time_HashAudio = []
Time_SaveHash = []

Time_FinaliseMatching = []
Time_SearchLUT = []
Time_CountNoMatches = []
Time_GetRefTiming = []
Time_GetRefTiming_2nd = []

kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)
for idx,utt__wav_path in enumerate(utt2wavpath_DICT_qry.items(), 1):
    utt,wav_path = utt__wav_path
    if utt in Unmatched_Dict : continue
    print(f"\n{idx}/{len(utt2wavpath_DICT_qry)} utt={utt},wav_path={wav_path}")
    ## Read shash
    try : 
        hash_path = os.path.join(conf_qry.exp_hashdir, utt+".shash")
        # qry_shash = dump_load_pickle(hash_path, "load")
        
        ## Read Audio
        # x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
        with Timer(Time_ReadAudio, "Time_ReadAudio") as t:  
            x_wav, sound_fs = af.read(wav_path)
            if len(x_wav.shape)>1: 
                x_wav = np.mean(x_wav, axis=0)

        with Timer(Time_Downsample, "Time_Downsample") as t:
            print('\tDownsampling')
            x_wav = resampy.resample(x_wav, sound_fs, conf_sr.sr, axis=0)

        ## Hash Audio
        with Timer(Time_HashAudio, "Time_HashAudio") as t:  
            qry_shash=Shazam.get_Shazam_hash(x_wav, kwargs["kwargs_peaks"], kwargs["kwargs_hashPeaks"], kwargs["kwargs_STFT"])

        ## Save to file
        with Timer(Time_SaveHash, "Time_SaveHash") as t:  
            dump_load_pickle(hash_path, "dump", qry_shash)
        
        if qry_shash[0] is None : 
            print(f"\t{utt} has no hash")
            Unmatched_Dict[utt] = {'remarks':f'{utt}\thas no shash extractable, possibly due to silence  : {wav_path} '}
            milestone()
            continue
    except subprocess.CalledProcessError: 
        print(f"\t{utt} has corrupted mp3")
        Unmatched_Dict[utt] = {'remarks':f'{utt}\thas corrupted mp3 : {wav_path}'}
        milestone()
        pdb.set_trace()
        debug123=1

    ## Matching
    with Timer(Time_FinaliseMatching, "Time_FinaliseMatching") as t:  
        with Timer(Time_SearchLUT, "Time_SearchLUT") as t:  
            CandidateSet = Shazam.searchLUT(LUT, qry_shash, utt)
        max_so_far=-1
        for utt_DB, num_union in CandidateSet:
            ## Perform matching
            shash_DB = utt2shash_DICT_DB[utt_DB]

            with Timer(Time_CountNoMatches, "Time_CountNoMatches") as t:  
                num_matches, hist, bin_edges = Shazam.count_number_match(shash_DB, qry_shash, bins=shash_DB[1]//100, N=1)

            ## Append to Matched
            if num_matches > conf_hash.th:
                with Timer(Time_GetRefTiming, "Time_GetRefTiming") as t:  
                    Matched_Shash_Dict[utt] = qry_shash
                    ## Get approx detected time
                    time_frame, start_sec = Shazam.get_ref_timing(hist, bin_edges, conf_sr)
                    Matched_Dict[utt] = {'utt_DB':utt_DB, 'num_matches':num_matches, 'num_union':num_union, 'start_sec':start_sec, 'time_frame':time_frame}
                    ## Verbose
                    print(f"\t{utt}   matched with {utt_DB}\tnum_matches:{num_matches}\tnum_union:{num_union}\tDetected Time:{start_sec}secs|{start_sec//60}mins{start_sec%60}secs")
                    break
            ## Append to Unmatched
            if num_matches > max_so_far:
                with Timer(Time_GetRefTiming_2nd, "Time_GetRefTiming_2nd") as t:  
                    ## Get approx detected time
                    time_frame, start_sec = Shazam.get_ref_timing(hist, bin_edges, conf_sr)
                    Unmatched_Dict[utt] = {'utt_DB':utt_DB, 'num_matches':num_matches, 'num_union':num_union, 'start_sec':start_sec, 'time_frame':time_frame}
                    # If there exist candidates, this will show the seconds best match that did not
                    #   1) cross the threshold.
                    #   2) did not get better match than the one that did cross the threshold.
                    # We save this anyway because, just in case we want 2nd best.
                    max_so_far = num_matches

    ## Verbose for Unmatched
    if len(CandidateSet)==0:
        Unmatched_Dict[utt] = {'remarks':f'{utt}\t has no candidates from DB_type={conf_db.DB_type}'}
        print(f"\t{utt} remarks:{Unmatched_Dict[utt]['remarks']}")
    elif not utt in Unmatched_Dict:
        Unmatched_Dict[utt] = {'remarks':f'{utt}\t has no additional candidates from DB_type={conf_db.DB_type}'}
        print(f"\t{utt} remarks:{Unmatched_Dict[utt]['remarks']}")
    elif (not utt in Matched_Dict) or (Unmatched_Dict[utt]['utt_DB'] != Matched_Dict[utt]['utt_DB']):
        ## (show best that did not qualify)
        print(f"\t{utt} unmatched with {Unmatched_Dict[utt]['utt_DB']}\tnum_matches:{Unmatched_Dict[utt]['num_matches']}\tnum_union:{Unmatched_Dict[utt]['num_union']}")

    ## Milestone
    milestone()

for des_LIST, lstname in zip(
    [Time_ReadAudio, Time_Downsample, Time_HashAudio, Time_SaveHash, Time_FinaliseMatching, Time_SearchLUT, Time_CountNoMatches, Time_GetRefTiming, Time_GetRefTiming_2nd, ],
    ["Time_ReadAudio", "Time_Downsample", "Time_HashAudio", "Time_SaveHash", "Time_FinaliseMatching", "Time_SearchLUT", "Time_CountNoMatches", "Time_GetRefTiming", "Time_GetRefTiming_2nd", ],
    ):
    if len(des_LIST) == 0 : 
        print(f'{lstname} : 0')
        continue
    curr_sum    =sum(des_LIST)
    curr_len    =len(des_LIST)
    curr_min    =min(des_LIST)
    curr_max    =max(des_LIST)
    curr_mean   =statistics.mean(des_LIST)
    curr_median =statistics.median(des_LIST)
    print(f'{lstname} : {curr_len}\tcurr_sum={curr_sum:0.3f}\tcurr_min={curr_min:0.3f}\tcurr_max={curr_max:0.3f}\tcurr_mean={curr_mean:0.3f}\tcurr_median={curr_median:0.3f}')
print()


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
