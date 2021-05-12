from __future__ import print_function
if True: ## imports / admin
    import os,sys,pdb # pdb.set_trace()
    sys.path.insert(0, 'utils')
    from _helper_basics_ import *
    import conf
    import multiprocessing
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
        parser.add_argument('-rawpaths')
        parser.add_argument('-wavdir')
        # 
        args=parser.parse_args()
        rawpaths=args.rawpaths
        wavdir=args.wavdir
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
print(f'\n-- Reading')

for idx, uttid_RawPath in enumerate(open(rawpaths, 'r')):
    rawpath = uttid_RawPath.strip('\n')
    print('\n', idx, rawpath)

    uttid = os.path.basename(rawpath).replace('.raw','')
    wavpath_16k=os.path.join(wavdir, f"{uttid}__out16k_0.wav")
    wavpath=os.path.join(wavdir, f"{uttid}.wav")

    if not os.path.exists(rawpath):
        print("rawpath=", rawpath, "DNE")
        continue

    if os.path.exists(wavpath_16k):
        print("wavpath_16k=", wavpath_16k, "EXISTS")
        continue

    os.system(f"utils/no_user_info_decoder {rawpath} {wavdir}/")

#################################################################
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

python utils/ConvertRaw2Wav.py \
    -rawpaths $qry_splitdir/rawpaths.$jid \
    -wavdir wav/2Million

"""
