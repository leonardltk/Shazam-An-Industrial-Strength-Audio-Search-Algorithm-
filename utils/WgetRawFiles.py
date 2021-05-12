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
        parser.add_argument('-rawhttppath')
        parser.add_argument('-raw_wavdir')
        parser.add_argument('-wavdir')
        # 
        args=parser.parse_args()
        rawhttppath=args.rawhttppath
        raw_wavdir=args.raw_wavdir
        wavdir=args.wavdir
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
print(f'\n-- Reading')

def download(http_path, raw_wavdir):

    # raw_wavpath = os.path.join(raw_wavdir, os.path.basename(http_path))
    # if not os.path.exists(raw_wavpath) :
    if not os.path.exists(
        os.path.join(raw_wavdir, 
            os.path.basename(http_path))
        ): os.system(f"wget --directory-prefix={raw_wavdir} {http_path} &> /dev/null")

    return

for idx, curr_http_path in enumerate( open(rawhttppath, 'r') ):
    try :
        print(idx, curr_http_path)
        httppath = curr_http_path.strip('\n').split('\t')[1]
        uttid = os.path.basename(httppath).replace('.raw','')
        rawpath = os.path.join(raw_wavdir, f"{uttid}.raw")
        wavpath_16k=os.path.join(wavdir, f"{uttid}__out16k_0.wav")
        print("\tuttid          =", uttid)
        print("\thttppath       =", httppath)
        print(f"\trawpath       = {rawpath}     \tExists={os.path.exists(rawpath)}")
        print(f"\twavpath_16k   = {wavpath_16k} \tExists={os.path.exists(wavpath_16k)}")

        ## If 16k exists, we ignore
        if os.path.exists(wavpath_16k):
            print("\twavpath_16k EXISTS, skipping")
            if os.path.exists(rawpath): os.remove(rawpath)
            continue

        ## If raw don't exists, we wget
        if not os.path.exists(rawpath):
            print("\trawpath DNE, wgetting first")
            bash_cmd=f"wget --directory-prefix={raw_wavdir} {httppath} && \
                utils/no_user_info_decoder {rawpath} {wavdir}/"
            print(bash_cmd)
            os.system(bash_cmd)
        ## If raw exists, we convert to 16k wav
        else:
            print("\trawpath EXISTS, directly convert to wav")
            bash_cmd=f"utils/no_user_info_decoder {rawpath} {wavdir}/"
            print(bash_cmd)
            os.system(bash_cmd)

        os.remove(rawpath)
    except:
        traceback.print_exc()
        print("Error\n\n")
        # pdb.set_trace()
    # pdb.set_trace()

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

python utils/WgetRawFiles.py \
    -rawhttppath $qry_data_dir/2Million \
    -raw_wavdir raw/2Million \
    -wavdir wav/2Million

python utils/WgetRawFiles.py \
    -rawhttppath $qry_splitdir/14Million_v${vrs}.$jid \
    -raw_wavdir $raw_wavdir \
    -wavdir $wav_wavdir


"""
