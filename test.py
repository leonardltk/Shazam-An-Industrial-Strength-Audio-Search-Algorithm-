from __future__ import print_function
if 1: ## imports / admin
    import os,sys, pdb,datetime, argparse, pprint, shutil, traceback
    sys.path.insert(0, 'utils')
    START_TIME=datetime.datetime.now()
    print(f"===========\npython {' '.join(sys.argv)}\n    Start_Time:{START_TIME}\n===========")
    #
    pp = pprint.PrettyPrinter(indent=4)
#################################################################

DBid2uttid = {}
DButtid2id = {}

wav_scp_DB="data/AllDatabase/wav.scp"
new_wav_scp_DB="data/AllDatabase/wav.scp.new"

f_db = open(new_wav_scp_DB, "w")

for idx, wavefile in enumerate(open(wav_scp_DB, 'r'), 1):
    try:
        uttid, wavepath = wavefile.strip().split(' ')
        assert not idx in DBid2uttid
        assert not uttid in DButtid2id
        DBid2uttid[idx] = uttid
        DButtid2id[uttid] = idx
        new_wavpath = f'wav/AllDatabase/{idx}.wav'
        # err = shutil.move(wavepath , new_wavpath)
        # assert err == new_wavpath
        f_db.write(f"{idx}\t{new_wavpath}\n")
    except:
        traceback.print_exc()
        pdb.set_trace()
f_db.close()
print("Written to", new_wav_scp_DB)

#################################################################

id2uttid = {}
uttid2id = {}

wav_scp="data/KnownPositiveNegatives/wav.scp"
new_wav_scp="data/KnownPositiveNegatives/wav.scp.new"

f_qry = open(new_wav_scp, "w")

diff = 0
for idx, wavefile in enumerate(open(wav_scp, 'r'), 1000):
    try:
        uttid, wavepath = wavefile.strip().split('\t')
        assert not idx in id2uttid
        if uttid in uttid2id:
            diff += 1
            continue
        ActualID = idx - diff
        id2uttid[ActualID] = uttid
        uttid2id[uttid] = ActualID
        new_wavpath = f'wav/KnownPositiveNegatives/{ActualID}.wav'
        # try:
        #     err = shutil.move(wavepath , new_wavpath)
        #     assert err == new_wavpath
        # except:
        #     traceback.print_exc()
        #     wavepath = wavepath.replace('14Million/','').replace('2Million/','')
        #     err = shutil.move(wavepath , new_wavpath)
        #     assert err == new_wavpath
        f_qry.write(f"{ActualID}\t{new_wavpath}\n")
    except:
        traceback.print_exc()
        pdb.set_trace()
f_qry.close()
print("Written to", new_wav_scp)

#################################################################
labels="data/KnownPositiveNegatives/qry2db2label"
new_labels="data/KnownPositiveNegatives/qry2db2label.new"
f_w = open(new_labels, 'w')
for qry2db2label in open(labels, 'r'):
    try:

        qry, db, label = qry2db2label.strip().split('\t')
        printline = f"{uttid2id[qry]}\t{DButtid2id[db]}\t{label}\n"
        f_w.write(printline)

        # print(qry2db2label)
        # print(printline)
    except:
        traceback.print_exc()
        pdb.set_trace()

f_w.close()
print("Written to", new_labels)

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
pdb.set_trace()

python test.py

mv -v data/AllDatabase/wav.scp.new data/AllDatabase/wav.scp
mv -v data/KnownPositiveNegatives/wav.scp.new data/KnownPositiveNegatives/wav.scp
mv -v data/KnownPositiveNegatives/qry2db2label.new data/KnownPositiveNegatives/qry2db2label

"""
