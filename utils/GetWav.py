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
        parser.add_argument('-db_wavscp')
        parser.add_argument('-qry_wavscp')
        parser.add_argument('-db2qry')
        parser.add_argument('-wav_dir')
        # 
        args=parser.parse_args()
        db_wavscp = args.db_wavscp
        qry_wavscp = args.qry_wavscp
        db2qry = args.db2qry
        wav_dir = args.wav_dir
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
if True :
    ## Read db
    db_wavscp_DICT=dict((line.strip('\n').split('\t')) for line in open(db_wavscp, "r"))
    print(f'There are {len(db_wavscp_DICT)} files in {db_wavscp}')
    ## Read qry
    qry_wavscp_DICT=dict((line.strip('\n').split('\t')) for line in open(qry_wavscp, "r"))
    print(f'There are {len(qry_wavscp_DICT)} files in {qry_wavscp}')

print(f'\n-- Getting db2qry')
db2qry_Dict = dict((line.strip('\n').split(' ',1)) for line in open(db2qry, "r"))
print(f'There are {len(db2qry_Dict)} files in {db2qry}')

print(f'\n-- Displaying local stats')
for dbutt, qryutt_str in db2qry_Dict.items():
    # Copying db wav
    print(dbutt)
    output_dir = os.path.join(wav_dir, dbutt)
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, f"{dbutt}.wav")
    shutil.copy(db_wavscp_DICT[dbutt] , output_path)
    print(f"\tCopied to {output_path}")
    # 
    ## Appending num matches
    qryutt_set = set(qryutt_str.split(' '))
    for qryutt in qryutt_set:
        output_path = os.path.join(output_dir, f"{qryutt}.wav")
        shutil.copy(qry_wavscp_DICT[qryutt], output_path)
        print(f"\tCopied to {output_path}")

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

python utils/GetWav.py \
    -db_wavscp $db_wavscp \
    -qry_wavscp $qry_wavscp \
    -db2qry $db2qry \
    -wav_dir wav/tolisten

"""
