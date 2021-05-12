from __future__ import print_function
if True: ## imports / admin
    import os,sys,pdb # pdb.set_trace()
    sys.path.insert(0, 'utils')
    from _helper_basics_ import *
    import conf
    # 
    START_TIME=datetime.datetime.now()
    datetime.datetime.now() - START_TIME
    python_file_verbose = ' '.join(sys.argv)
    python_file_verbose = python_file_verbose.replace(' -','\n\t-')
    print(f"===========\npython {python_file_verbose}\n    Start_Time:{START_TIME}\n===========")
    # 
    print('############ Printing Config Params ############')
    if True:
        import argparse
        parser=argparse.ArgumentParser()
        parser.add_argument('-DB_type')
        parser.add_argument('-Qry_type')
        # Unused
        parser.add_argument('-qry_wavscp')
        parser.add_argument('-qrydb_dir')
        # Read From
        parser.add_argument('-qry2db')
        parser.add_argument('-qry2wavpath')
        parser.add_argument('-qry2reftime')
        parser.add_argument('-db2qry')
        # Write to
        parser.add_argument('-SortedStats')
        parser.add_argument('-ModWavDir')
        # 
        args=parser.parse_args()
        DB_type=args.DB_type
        Qry_type=args.Qry_type
        # 
        qry_wavscp=args.qry_wavscp
        qrydb_dir=args.qrydb_dir
        qry2db=args.qry2db
        qry2wavpath=args.qry2wavpath
        qry2reftime=args.qry2reftime
        db2qry=args.db2qry
        # 
        SortedStats=args.SortedStats
        ModWavDir=args.ModWavDir
        # fscgb_lst=args.fscgb_lst
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
print(f'\n-- Reading')
qry2nummatch_Dict = dict( line.strip('\n').split(' ')[::2] for line in open(qry2db, "r"))
# qry2reftime_Dict  = dict( line.strip('\n').split(' ') for line in open(qry2reftime, "r"))
if os.path.exists(qry2wavpath):
    qry2wavpath_Dict  = dict( line.strip('\n').split(' ') for line in open(qry2wavpath, "r"))
else:
    qry2wavpath_Dict={}
    start_tym = time.perf_counter()
    for line in open(qry_wavscp, "r") :
        uttid, wavpath = line.split('\t')
        if uttid in qry2nummatch_Dict:
            qry2wavpath_Dict[uttid] = wavpath.strip('\n')
    print(f"The time taken to get qry2wavpath_Dict : {time.perf_counter() - start_tym}.")

print(f"len(qry2nummatch_Dict)={len(qry2nummatch_Dict)}")
# print(f"len(qry2reftime_Dict) ={len(qry2reftime_Dict)}")
print(f"len(qry2wavpath_Dict) ={len(qry2wavpath_Dict)}")

print(f'\n-- Writing')
try:
    print( f"\tWriting to {SortedStats}" )
    with open( SortedStats, 'w') as f_w:
        # f_w.write( f"db_utt\tqry_url\tnum_matches\treference_time(mm:ss)\n" )
        f_w.write( f"db_utt\tqry_url\tnum_matches\n" )
        # for dbutt, qryutt_set in db2qry_Dict.items():
        for line in open(db2qry, "r"):
            dbutt = line.split(' ',1)[0]
            qryutt_set = line.strip('\n').split(' ')[1:]
            ## Appending num matches
            for qryutt in qryutt_set:
                wav_path = qry2wavpath_Dict[qryutt]
                qry_bn = os.path.basename(wav_path)
                num_match = qry2nummatch_Dict[qryutt]
                # start_sec = qry2reftime_Dict[qryutt]
                # f_w.write( f"{dbutt}\tfscgb/{qry_bn}\t{num_match}\t{start_sec}\n" )
                f_w.write( f"{dbutt}\tfscgb/{qry_bn}\t{num_match}\n" )
                # Copy to fscgb it
                fscgb_wavpath=os.path.join(ModWavDir, qry_bn)
                if not os.path.exists(fscgb_wavpath):
                    shutil.copy(wav_path, ModWavDir)
                    print( f"\t\t{wav_path} -> {ModWavDir}")
except:
    traceback.print_exc()
    pdb.set_trace()
    xtmp=12
print(f"\n\tWritten to {SortedStats}" )
print(  f"\tCopied wav files to {ModWavDir}" )

#################################################################
END_TIME=datetime.datetime.now()
print(f"\n===========\
    \nDone \
    \npython {' '.join(sys.argv)}\
    \nStart_Time  :{START_TIME}\
    \nEnd_Time    :{END_TIME}\
    \nDuration    :{END_TIME-START_TIME}\
\n===========")

"""
!import code; code.interact(local=vars())

python utils/ForModerationTeam.py \
        -DB_type $DB_type -Qry_type $Qry_type \
        -qry_wavscp $qry_wavscp -qrydb_dir $qrydb_dir \
        -qry2db $qry2db -qry2wavpath $qry2wavpath -qry2reftime $qry2reftime \
        -db2qry $db2qry \
        -SortedStats $SortedStats -ModWavDir $ModWavDir

"""
