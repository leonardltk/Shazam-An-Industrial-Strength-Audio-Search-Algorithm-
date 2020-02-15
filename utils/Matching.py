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
        parser.add_argument('-conf_db')
        parser.add_argument('-conf_qry')
        parser.add_argument('-conf_sr')
        parser.add_argument('-conf_hash')
        # 
        args=parser.parse_args()
        conf_db=get_conf(args.conf_db)
        conf_qry=get_conf(args.conf_qry)
        conf_sr=get_conf(args.conf_sr)
        conf_hash=get_conf(args.conf_hash)
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
if True : ## Read hash
    ## Read hash.scp
    utt2hashpath_DICT_DB = dict( (i.replace('\n','').split(' ')) for i in open(conf_db.shash_scp,'r') )
    utt2hashpath_DICT = dict( (i.replace('\n','').split(' ')) for i in open(conf_qry.shash_scp,'r') )
    ## Get hashes
    utt2shash_DICT_DB = dict( (utt,dump_load_pickle(hash_path, "load")) for utt,hash_path in utt2hashpath_DICT_DB.items() )


#################################################################
print('========== Matching =================')
print("\tWriting to ",conf_qry.qry2db)
f_w = open(conf_qry.qry2db,"w")
for utt,hash_path in utt2hashpath_DICT.items():
    ## Read shash
    qry_shash=dump_load_pickle(hash_path, "load")

    ## Matching
    for utt_DB,shash_DB in utt2shash_DICT_DB.items():
        curr_dec=Shazam.make_decision(shash_DB, qry_shash, bins=100, N=1, verbose=0)
        if curr_dec is not None:
            f_w.write("{} {}\n".format(utt,utt_DB))
            break
    if curr_dec is None:
        f_w.write("{} {}\n".format(utt,"-"))
f_w.close()
print("\tWritten to ",conf_qry.qry2db)
print("\tcat ",conf_qry.qry2db)

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

1, pp.pprint(utt2shash_DICT_DB)
"""
