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
        parser.add_argument('-qry2db')
        parser.add_argument('-qry2nummatch')
        parser.add_argument('-qry2db2label')
        # Write to
        parser.add_argument('-qry2Wrong')
        parser.add_argument('-qry2FalseNegatives')
        parser.add_argument('-qry2FalsePositives')
        # 
        args=parser.parse_args()
        # 
        qry2db=args.qry2db
        qry2nummatch=args.qry2nummatch
        qry2db2label=args.qry2db2label
        # 
        qry2Wrong=args.qry2Wrong
        qry2FalseNegatives=args.qry2FalseNegatives
        qry2FalsePositives=args.qry2FalsePositives
        # 
        pp = pprint.PrettyPrinter(indent=4)
        global_st = time.time()
    print('############ End of Config Params ##############')
#################################################################
print(f'\n-- Reading')
if True:
    qry2db_Dict = {}
    for line in open(qry2db, "r"):
        uttqry, uttdb, num_match = line.strip('\n').split(' ')
        qry2db_Dict[uttqry] = {uttdb : num_match}
    qry2nummatch2db_Dict={}
    for line in open(qry2nummatch, "r"):
        uttqry, num_match, uttdb = line.strip('\n').split(' ')
        if not uttqry in qry2nummatch2db_Dict : qry2nummatch2db_Dict[uttqry] = {}
        qry2nummatch2db_Dict[uttqry][uttdb] = num_match
    print(f"len(qry2db_Dict)={len(qry2db_Dict)}")
    print(f"len(qry2nummatch2db_Dict) ={len(qry2nummatch2db_Dict)}")

print(f'\n-- Calculating')
TP,FP = 0,0
TN,FN = 0,0
NumberCorrect=0
qry2Correct_Dict={}
qry2Wrong_Dict={}
qry2FalseNegatives_Dict={}
qry2FalsePositives_Dict={}
for idx, line in enumerate(open(qry2db2label, "r"), 1):
    flag=0
    uttqry, uttdb, label = line.strip('\n').split('\t')

    if (label=='0'):
        # Calc Recall
        if not uttqry in qry2db_Dict:    ## Right
            qry2Correct_Dict[uttqry]={uttdb :  0}
            TN += 1
            # print("TN += 1")
        elif   uttqry in qry2db_Dict:    ## Wrong
            qry2Wrong_Dict[uttqry] = qry2db_Dict[uttqry]
            FP += 1
            # print("FP += 1")
            flag=1
            ## Got it wrong, and below threshold also
            qry2FalsePositives_Dict[uttqry] = qry2db_Dict[uttqry]
            if uttdb in qry2nummatch2db_Dict[uttqry]:
                print(f"\t FalsePositives : uttqry={uttqry} uttdb={uttdb}")

    elif (label=='1'):
        ## Calc Precision

        # FN : didnt even flag out this
        case1 = not uttqry in qry2db_Dict
        # TP : this is flagged out, and guessed correctly
        case2 = (uttqry in qry2db_Dict) and (uttdb in qry2db_Dict[uttqry])
        # FP : this is flagged out, but guessed incorrectly
        case3 = (uttqry in qry2db_Dict) and (not uttdb in qry2db_Dict[uttqry])

        if   case1:      
            if (uttqry in qry2nummatch2db_Dict) and (uttdb in qry2nummatch2db_Dict[uttqry]):
                qry2Wrong_Dict[uttqry] = {uttdb :  qry2nummatch2db_Dict[uttqry][uttdb]}
            else:
                qry2Wrong_Dict[uttqry] = {uttdb :  0}
            # print("FN += 1")
            FN += 1
            flag=1
            ## Got it right, but below threshold
            qry2FalseNegatives_Dict[uttqry] = {uttdb:0}
            if uttqry in qry2nummatch2db_Dict:
                if (uttdb in qry2nummatch2db_Dict[uttqry]):
                    print(f"\t FalseNegatives : uttqry={uttqry} uttdb={uttdb}")
                    nummatches_Positive = qry2nummatch2db_Dict[uttqry].pop(uttdb)
                    qry2FalseNegatives_Dict[uttqry][uttdb] = nummatches_Positive
        elif case2:    
            qry2Correct_Dict[uttqry]=qry2db_Dict[uttqry]
            TP += 1
            # print("TP += 1")
        elif case3:
            qry2Wrong_Dict[uttqry] = qry2db_Dict[uttqry]
            FP += 1
            # print("FP += 1")
            flag=1
            ## Above threshold, but got it wrong
            qry2FalsePositives_Dict[uttqry] = qry2db_Dict[uttqry]
            if uttdb in qry2nummatch2db_Dict[uttqry]:
                print(f"\t FalsePositives : uttqry={uttqry} uttdb={uttdb}")
            pdb.set_trace()
        else:
            print(f"Why will come here ?")
            pdb.set_trace()
            abx=123

    ## Verbose
    if 0:
        ## 
        print("\n\n===================================")
        os.system(f"grep {uttqry} {qry2nummatch}")
        if uttqry in qry2nummatch2db_Dict : 1, pp.pprint(qry2nummatch2db_Dict[uttqry])
        ## 
        print("===================================")
        print(f"uttqry={uttqry}")
        print(f"uttdb={uttdb}")
        print(f"label={label}")
        if uttqry in qry2db_Dict : print(f"pred={qry2db_Dict[uttqry]}")

        print("===================================")
        print(f"TP={TP}\tFP={FP}")
        print(f"TN={TN}\tFN={FN}")
        print(f"NumberCorrect={NumberCorrect}")

        print("===================================")
        print("qry2Correct_Dict=")
        1, pp.pprint(qry2Correct_Dict)

        print("===================================")
        print("qry2Wrong_Dict=")
        1, pp.pprint(qry2Wrong_Dict)

        print("\n\n")
        if flag==1:         pdb.set_trace()

print(f"idx={idx}")
print(f"len(qry2Correct_Dict)={len(qry2Correct_Dict)}")
print(f"len(qry2Wrong_Dict)={len(qry2Wrong_Dict)}")
print(f"len(qry2FalseNegatives_Dict)={len(qry2FalseNegatives_Dict)}")
print(f"len(qry2FalsePositives_Dict)={len(qry2FalsePositives_Dict)}")
if (TN or FN) : print(f"TN/N = {TN}/{TN+FN} = {100*TN/(TN+FN):.2f}%")
if (TP or FP) : print(f"TP/P = {TP}/{TP+FP} = {100*TP/(TP+FP):.2f}%")
print(f"Accuracy = {100*(TP+TN)/(TP+FP+TN+FN):.2f}%")

print(f'\n-- Writing')
if True:

    with open(qry2Wrong, 'w') as f_w:
        for uttqry, WrongDetDict in qry2Wrong_Dict.items():
            for uttdb, nummatches in WrongDetDict.items():
                f_w.write(f"{uttqry} {uttdb} {nummatches}\n")
        print(f"\tWritten to {qry2Wrong}" )

    with open(qry2FalseNegatives, 'w') as f_w:
        for uttqry, FalseDetDict in qry2FalseNegatives_Dict.items():
            for uttdb, nummatches in FalseDetDict.items():
                f_w.write(f"{uttqry} {uttdb} {nummatches}\n")
        print(f"\tWritten to {qry2FalseNegatives}" )

    with open(qry2FalsePositives, 'w') as f_w:
        for uttqry, FalseDetDict in qry2FalsePositives_Dict.items():
            for uttdb, nummatches in FalseDetDict.items():
                f_w.write(f"{uttqry} {uttdb} {nummatches}\n")
        print(f"\tWritten to {qry2FalsePositives}" )

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

python utils/GetStats.py \
        -qry2db $qry2db \
        -qry2nummatch $qry2nummatch \
        -qry2db2label $qrydb_dir/qry2db2label \
            -qry2Wrong $qrydb_dir/qry2Wrong \
            -qry2FalsePositives $qrydb_dir/qry2FalsePositives \
            -qry2FalseNegatives $qrydb_dir/qry2FalseNegatives

"""
