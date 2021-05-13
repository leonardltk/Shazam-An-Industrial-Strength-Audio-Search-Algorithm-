import os,sys,pdb

GTpath = "qry2db2label"
PosLog = sys.argv[1]
NegLog = sys.argv[2]

PositiveGT = {}
NegativeGT = set()

for vid_uttid_c in open(GTpath, 'r'):
    QryVid, DBVid, PosOrNeg = vid_uttid_c.strip().split("\t", 2)
    if PosOrNeg == "1":
        if (QryVid in PositiveGT ) and (PositiveGT[QryVid] != DBVid):
            print(f"QryVid={QryVid} DBVid(old)={PositiveGT[QryVid]} DBVid(new)={DBVid}")
        PositiveGT[QryVid] = DBVid
    else:
        NegativeGT.add(QryVid)

FP1,TP,FN = 0,0,0
FP1 = 0 # it is positive, but wrong cluster identified
for idx, log_c in enumerate(open(PosLog, 'r')):
    if idx == 0 : continue
    QryVid, cid, DbWavId = log_c.strip().split("\t",2)
    cid = int(cid)
    DBVid = DbWavId.replace(".wav", "")
    if 1 :
        ## Positive prediction
        if cid > 0:
            if PositiveGT[QryVid] != DBVid:
                FP1 += 1
            else:
                assert PositiveGT[QryVid] == DBVid
                TP += 1
        ## Negative prediction
        else:
            FN += 1

TN  = 0
FP2 = 0 # supposed to be negative but is identified
for idx, log_c in enumerate(open(NegLog, 'r')):
    if idx == 0 : continue
    QryVid, cid, DbWavId = log_c.strip().split("\t",2)
    cid = int(cid)
    DBVid = DbWavId.replace(".wav", "")
    if 1 :
        ## Positive prediction
        if cid > 0:
            FP2 += 1
        ## Negative prediction
        else:
            TN += 1

FP = FP1 + FP2

print(f"FP1 = {FP1}")
print(f"FP2 = {FP2}")
print(f"TP  = {TP}")
print(f"FP  = {FP}")

print(f"FN  = {FN}")
print(f"TN  = {TN}")

Precision   = TP / (FP+TP)
Precision1  = (TP+FP1) / (FP+TP)
Recall      = TP / (FN+TP)
Specificty  = TN / (FN+TN)
print(f"Precision   = {100 * Precision:0.2f}")
print(f"Precision1  = {100 * Precision1:0.2f}")
print(f"Recall      = {100 * Recall:0.2f}")
print(f"Specificty  = {100 * Specificty:0.2f}")


"""
!import code; code.interact(local=vars())

python checkTP.py \
    out_file.Pos.txt \
    out_file.Neg.txt

"""
