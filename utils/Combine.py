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
if True : ## Read qry
    ## Read wav.scp
    try :
        wavscp_DICT_qry=dump_load_pickle(conf_qry.exp_wavscp_dict, "load")
    except :
        ## First time we read the full wav.scp, as the previous Matching.py is done on the split wav.scp(s)
        wavscp_DICT_qry = dict( (i.replace('\n','').split('\t')) for i in open(conf_qry.wav_scp,'r') )
        dump_load_pickle(conf_qry.exp_wavscp_dict, "dump", wavscp_DICT_qry)
    print(f'There are {len(wavscp_DICT_qry)} files in {conf_qry.wav_scp}')

print('========== Combining =================')

print(f'\n-- Combining to {conf_qry.exp_Matched_Dict}')
Matched_Dict={}
for jid in range(1, nj+1):
    jid=f"{jid}".zfill(zfill)
    split_Matched_Dict_path=os.path.join(conf_qry.split_exp,f'Matched_Dict.{jid}')
    split_Matched_Dict=dump_load_pickle(split_Matched_Dict_path, 'load')
    Matched_Dict.update(split_Matched_Dict)
    for utt,MatchDet_c in split_Matched_Dict.items():
        print(f"\tutt:{utt}, \tutt_DB:{MatchDet_c['utt_DB']}, \tnum_matches:{MatchDet_c['num_matches']}, \tnum_union:{MatchDet_c['num_union']}")
dump_load_pickle(conf_qry.exp_Matched_Dict, "dump", Matched_Dict)
print('Matched_Dict',len(Matched_Dict))


print(f'\n-- Combining to {conf_qry.qry2db} {conf_qry.db2qry}')
# conf_qry.qry2db
qry2db_DICT={}
db2qry_DICT={}
with open(conf_qry.qry2db,"w") as f_w:
    for utt, MatchDet_c in Matched_Dict.items():
        dbutt = MatchDet_c['utt_DB']
        qry2db_DICT[utt]=dbutt
        if not dbutt in db2qry_DICT : db2qry_DICT[dbutt]=set()
        db2qry_DICT[dbutt].add(utt)
        f_w.write(f"{utt}\t{dbutt}\n")
print('Written to',conf_qry.qry2db)
pdb.set_trace()
dump_load_pickle(conf_qry.exp_qry2db_dict, "dump", qry2db_DICT)
# conf_qry.db2qry
with open(conf_qry.db2qry,"w") as f_w:
    for dbutt, utt_set in db2qry_DICT.items():
        print_utt_set='\t'.join(utt_set)
        f_w.write(f"{dbutt}\t{print_utt_set}\n")
print('Written to',conf_qry.db2qry)
dump_load_pickle(conf_qry.exp_db2qry_dict, "dump", db2qry_DICT)



print(f'\n-- Combining to {conf_qry.exp_Unmatched_Dict}')
Unmatched_Dict={}
for jid in range(1, nj+1):
    jid=f"{jid}".zfill(zfill)
    split_Unmatched_Dict_path=os.path.join(conf_qry.split_exp,f'Unmatched_Dict.{jid}')
    split_Unmatched_Dict=dump_load_pickle(split_Unmatched_Dict_path, 'load')
    Unmatched_Dict.update(split_Unmatched_Dict)
    for utt,UnmatchDet_c in split_Unmatched_Dict.items():
        if 'remarks' in UnmatchDet_c:
            print(f"\tutt:{utt}, \tremarks:{UnmatchDet_c['remarks']}")
        else:
            print(f"\tutt:{utt}, \tutt_DB:{UnmatchDet_c['utt_DB']}, \tnum_matches:{UnmatchDet_c['num_matches']}, \tnum_union:{UnmatchDet_c['num_union']}")
dump_load_pickle(conf_qry.exp_Unmatched_Dict, "dump", Unmatched_Dict)
print('Unmatched_Dict',len(Unmatched_Dict))

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
