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
        parser.add_argument('-nj', type=int)
        parser.add_argument('-zfill', type=int)
        # 
        args=parser.parse_args()
        DB_type=args.DB_type
        Qry_type=args.Qry_type
        nj=args.nj
        zfill=args.zfill
        # 
        conf_db=conf.DB_conf(DB_type)
        conf_qry=conf.QRY_conf(Qry_type)
        conf_hash=conf.Shazam_conf()
        conf_sr=conf.SR_conf()
        str(conf_db)
        str(conf_qry)
        str(conf_hash)
        str(conf_sr)
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
if True :
    ## Read db
    wavscp_DICT=dump_load_pickle(conf_db.exp_wavscp_dict, "load")
    print(f'There are {len(wavscp_DICT)} files in {conf_db.exp_wavscp_dict}')
    ## Read qry
    wavscp_DICT_qry=dump_load_pickle(conf_qry.exp_wavscp_dict, "load")
    Matched_Dict = dump_load_pickle(conf_qry.exp_Matched_Dict, "load")
    print(f'There are {len(wavscp_DICT_qry)} files in {conf_qry.wav_scp}')
    print('Matched_Dict',len(Matched_Dict))

print(f'\n-- Getting db2qry')
qry2db_Dict = dump_load_pickle(conf_qry.exp_qry2db_dict, "load")
db2qry_Dict = dump_load_pickle(conf_qry.exp_db2qry_dict, "load")

print(f'\n-- Displaying local stats')
global_qryutt_lst = []
global_num_matches_lst = []
os.makedirs(conf_qry.sorted_stats, exist_ok=True)
for dbutt, qryutt_set in db2qry_Dict.items():
    ## Appending num matches
    qryutt_lst = list(qryutt_set)
    num_matches_lst = []
    for qryutt in qryutt_lst:
        num_matches_lst.append( Matched_Dict[qryutt]['num_matches'] )
    ## Writing sorted info to file
    curr_sorted_stats=os.path.join(conf_qry.sorted_stats, dbutt)
    with open( curr_sorted_stats, 'w') as f_w:
        f_w.write( f"dbutt\tqryutt\tnum_matches\n" )
        for idx in reversed(sorted(range(len(num_matches_lst)), key=num_matches_lst.__getitem__)):
            global_qryutt_lst.append(qryutt_lst[idx])
            global_num_matches_lst.append(num_matches_lst[idx])
            f_w.write( f"{qryutt_lst[idx]}\t{num_matches_lst[idx]}\n" )
    print( f"\tWritten to {curr_sorted_stats}" )

print(f'\n-- Displaying global stats')
global_sorted_stats=''.join([conf_qry.sorted_stats, '_global'])
with open( global_sorted_stats, 'w') as f_w:
    f_w.write( f"dbutt\tqryutt\tnum_matches\n" )
    for idx in reversed(sorted(range(len(global_num_matches_lst)), key=global_num_matches_lst.__getitem__)):
        f_w.write( f"{qry2db_Dict[global_qryutt_lst[idx]]}\t{global_qryutt_lst[idx]}\t{global_num_matches_lst[idx]}\n" )
print( f"\tWritten to {global_sorted_stats}" )

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
"""
