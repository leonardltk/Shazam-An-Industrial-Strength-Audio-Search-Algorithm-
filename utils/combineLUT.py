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
        parser.add_argument('-num_a', type=int)
        parser.add_argument('-nj', type=int)
        parser.add_argument('-db_LUT')
        parser.add_argument('-db_splitshash')
        # 
        args=parser.parse_args()
        num_a = args.num_a
        nj = args.nj
        db_LUT = args.db_LUT
        db_splitshash = args.db_splitshash
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################

print(f'\n-- Appending')
LUT = {}
for jid_int in range(1, nj+1):
    jid = str(jid_int).zfill(num_a)
    # Reading split LUT
    db_LUT_jid = os.path.join(db_splitshash, f"LUT.{jid}")
    ## Appending to the list
    for hashkey_numutt_uttstr in open(db_LUT_jid, 'r'):
        hashkey, _, uttstr = hashkey_numutt_uttstr.strip(' \n').split(' ', 2)
        uttset = set(uttstr.split(' '))
        if not hashkey in LUT :
            LUT[hashkey] = uttset
        else:
            LUT[hashkey].update(uttset)
print(f'LUT has size {len(LUT)}')

print(f'\n-- Combining')
f_w = open(db_LUT, 'w')
for hashkey, uttset in LUT.items(): f_w.write(f"{hashkey} {len(uttset)} {' '.join(uttset)}\n")
f_w.close()

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

python utils/combineLUT.py \
    -num_a $num_a \
    -nj $nj \
    -db_LUT $db_LUT \
    -db_splitshash $db_splitshash

"""
