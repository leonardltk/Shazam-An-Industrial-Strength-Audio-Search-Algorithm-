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
        # Read From
        parser.add_argument('-segment2db')
        parser.add_argument('-qry2db')
        parser.add_argument('-db2qry')
        parser.add_argument('-qry2nummatch')
        # 
        args=parser.parse_args()
        segment2db=args.segment2db
        qry2db=args.qry2db
        db2qry=args.db2qry
        qry2nummatch=args.qry2nummatch
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
print(f'\n-- Backuping up')
if True:
    if not os.path.exists(f"{db2qry}.crop"):
        copy_log = shutil.copy(db2qry, f"{db2qry}.crop")
        print(f"Copied to {copy_log}")
    if not os.path.exists(f"{qry2db}.crop"):
        copy_log = shutil.copy(qry2db, f"{qry2db}.crop")
        print(f"Copied to {copy_log}")
    if not os.path.exists(f"{qry2nummatch}.crop"):
        copy_log = shutil.copy(qry2nummatch, f"{qry2nummatch}.crop")
        print(f"Copied to {copy_log}")

#################################################################
print(f'\n-- Reading')
if not os.path.exists(segment2db):
    f_w = open(segment2db, "w")
    for line in open(segment2db.replace("segment2db", "db2bin"), "r"):
        uttseg, _ = line.strip().split(' ')
        uttdb = "_".join(uttseg.split('_')[:-1])
        f_w.write(f"{uttseg}\t{uttdb}\n")
    f_w.close()
if True:
    segment2db_Dict = {}
    for line in open(segment2db, "r"):
        uttseg, uttdb = line.strip().split('\t')
        segment2db_Dict[uttseg]=uttdb

#################################################################
print(f'\n-- Writing')
if True:
    f_w = open(db2qry, "w")
    for line in open(f"{db2qry}.crop", "r"):
        uttdb, uttqrys = line.split(' ', 1)
        f_w.write(f"{segment2db_Dict[uttdb]} {uttqrys}")
    f_w.close()
    print(f"Finished writing to {db2qry}")

    f_w = open(qry2db, "w")
    for line in open(f"{qry2db}.crop", "r"):
        uttqry, uttdb, nummatch = line.split(' ')
        f_w.write(f"{uttqry} {segment2db_Dict[uttdb]} {nummatch}")
    f_w.close()
    print(f"Finished writing to {qry2db}")

    f_w = open(qry2nummatch, "w")
    for line in open(f"{qry2nummatch}.crop", "r"):
        uttqry, nummatch, uttdb = line.strip().split(' ')
        f_w.write(f"{uttqry} {nummatch} {segment2db_Dict[uttdb]}\n")
    f_w.close()
    print(f"Finished writing to {qry2nummatch}")

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

python utils/AlignCropOutputs.py \
    -segment2db $db_data_dir/segment2db \
    -qry2db $qry2db -db2qry $db2qry -qry2nummatch $qry2nummatch

head -n5 $qry2db $db2qry $qry2nummatch
head -n5 $qry2db.crop $db2qry.crop $qry2nummatch.crop

"""
