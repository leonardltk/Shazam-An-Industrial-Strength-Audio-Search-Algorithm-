from __future__ import print_function
import os

verbose_conf=True
# verbose_conf=False

mode='DB_v1'
############ Data Dir/Path ############
# Written in run0_SegmentGeneration.py
if 1:
    #### data
    data_dir=os.path.join('data',mode); os.makedirs(data_dir,exist_ok=True)
    ## Read details
    wav_scp=os.path.join(data_dir,'wav.scp')
    shash_scp=os.path.join(data_dir,'shash.scp')

    #### wav
    wav_dir=os.path.join('wav',mode)

    if verbose_conf:
        print('    mode                 :',mode)
        print('\n    ############ Data Dir/Path ############')
        print('    data_dir             :',data_dir)
        ## Read details
        print('    wav_scp              :',wav_scp)
        print('    wav_dir              :',wav_dir)

############ Hash Dir/Path ############
# Written in run1_Hashing.py
if 1:
    #### exp
    exp_dir=os.path.join('exp',mode)
    os.makedirs(exp_dir,exist_ok=True)
    ## From Data
    exp_wavscp_dict=os.path.join(exp_dir,'wavscp_DICT')
    ## Hash
    exp_hashdir=os.path.join(exp_dir,'hashes')

    if verbose_conf:
        print('\n    ############ Hash Dir/Path ############')
        print('    exp_dir              :',exp_dir)
        ## From Data
        print('      exp_wavscp_dict    :',exp_wavscp_dict)
        print('      exp_hashdir       :',exp_hashdir)
