#!/usr/bin/bash
conda activate YOUR_ENVIRONMENT
# conda activate Py3_6_v1
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

## Directory names of database and queries
    DB_type=database
    Qry_type=query_short

## Their files
    db_data_dir=./data/$DB_type; mkdir -pv $db_data_dir
        db_wavscp=$db_data_dir/wav.scp
    qry_data_dir=./data/$Qry_type
        qry_wavscp=$qry_data_dir/wav.scp
        qry2db=$qry_data_dir/qry2db
        db2qry=$qry_data_dir/db2qry
        splitdir=$qry_data_dir/split; mkdir -pv $splitdir
    splitexp=./exp/$Qry_type/split; mkdir -pv $splitexp
echo
echo "DB_type           = $DB_type"
echo "    db_data_dir   = $db_data_dir"
echo "    db_wavscp     = $db_wavscp"
echo
echo "Qry_type          = $Qry_type"
echo "    qry_data_dir  = $qry_data_dir"
echo "    qry_wavscp    = $qry_wavscp"
echo "    qry2db        = $qry2db"
echo "    db2qry        = $db2qry"
echo "    splitdir      = $splitdir"
echo "splitexp          = $splitexp"
echo

logdir=./logs/$DB_type/$Qry_type; mkdir -pv $logdir
echo "logdir            = $logdir"

# nj=4; num_a=${#nj}
nj=32; num_a=${#nj}
echo "nj                = $nj"
echo "num_a             = $num_a"

## Matching Parameters
cont=False # Default, to start the Matching process from scratch
echo "cont              = $cont"
