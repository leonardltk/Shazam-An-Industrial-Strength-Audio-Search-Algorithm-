#!/usr/bin/bash

conda activate YOUR_ENVIRONMENT
<< COMMENTS
    python 3.6
    audiofile              0.2.2
    audioread              2.1.8
    librosa                0.7.0
    matplotlib             3.0.3
    numpy                  1.17.2
    python-speech-features 0.6
    scikit-image           0.15.0
    scikit-learn           0.21.3
    scipy                  1.3.1
    SoundFile              0.10.3.post1
    sox                    1.3.7
COMMENTS

DB_type=database
Qry_type=query_short
# Qry_type=query_long
    qry_data_dir=./data/$Qry_type
        qry_wavscp=$qry_data_dir/wav.scp
        qry2db=$qry_data_dir/qry2db
        db2qry=$qry_data_dir/db2qry
        splitdir=$qry_data_dir/split
    splitexp=./exp/Qry_v1/split

nj=3; num_a=${#nj}

logdir=./logs/$DB_type/$Qry_type; mkdir -pv $logdir

## Hashing Parameters
## Matching Parameters
cont=False # Default, to start the Matching process from scratch
