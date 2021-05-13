#!/usr/bin/bash

echo "Doing : $0 $@"

if [ $# -lt 2 ]; then
    echo "Usage: . path.sh <database-name> <query-name>"
    echo "e.g.: . path_shash.sh test_db test_qry"
else
    ## Params
    nj=30; num_a=${#nj}
    cont=False

    ## Directory names of database and queries
    DB_type=$1
    Qry_type=$2

    ## db data files
    db_data_dir=./data/$DB_type; mkdir -pv $db_data_dir
    db_wavscp=$db_data_dir/wav.scp
    db_db2bin=$db_data_dir/db2bin
    db_splitdir=$db_data_dir/split; mkdir -pv $db_splitdir

    ## qry data files
    qry_data_dir=./data/$Qry_type; mkdir -pv $qry_data_dir
    qry_wavscp=$qry_data_dir/wav.scp
    qry_splitdir=$qry_data_dir/split; mkdir -pv $qry_splitdir

    ## db files within qry
    qrydb_dir=$qry_data_dir/$DB_type; mkdir -pv $qrydb_dir
    qry2db=$qrydb_dir/qry2db
    qry2nummatch=$qrydb_dir/qry2nummatch
    qry2wavpath=$qrydb_dir/qry2wavpath
    qry2reftime=$qrydb_dir/qry2reftime
    db2qry=$qrydb_dir/db2qry
    qrydb_splitdir=$qrydb_dir/split; mkdir -pv $qrydb_splitdir
    SortedStats=$qrydb_dir/SortedStats
    fscgb_lst=$qrydb_dir/fscgb_lst

    ## shash files
    db_shash_dir=./shash/$DB_type; mkdir -pv $db_shash_dir
    db_LUT=$db_shash_dir/LUT.txt
    db_splitshash=$db_shash_dir/split; mkdir -pv $db_splitshash
    qry_shash_dir=./shash/$Qry_type; mkdir -pv $qry_shash_dir

    ## matched wav files to copy to
    DdWavDir=./wav/$DB_type; mkdir -pv $DdWavDir
    QryWavDir=./wav/$Qry_type; mkdir -pv $QryWavDir
    ModWavDir=$QryWavDir/$DB_type; mkdir -pv $ModWavDir

    ## log files are stored here
    logdir_db=./logs/$DB_type; mkdir -pv $logdir_db
    logdir_qry=./logs/$Qry_type/$DB_type; mkdir -pv $logdir_qry

    # Parameteres
    min_duration=5
    nfft=512;
    MAXLISTWinSize=7; # Reduced (15->7)
    thabs_db=1;
    thabs_qry=8192 # Accounted for short int, instead of float (0.25 * 2^15 == 8192)
    threshold=110;
    TopN=2;

    echo -e "
    nj                = $nj
    num_a             = $num_a
    cont              = $cont

    DB_type           = $DB_type
        db_data_dir   = $db_data_dir
        db_wavscp     = $db_wavscp
        db_db2bin     = $db_db2bin
        db_splitdir   = $db_splitdir

    Qry_type          = $Qry_type
        qry_data_dir  = $qry_data_dir
        qry_wavscp    = $qry_wavscp
        qry_splitdir  = $qry_splitdir
        qrydb_dir     = $qrydb_dir
          qry2db        = $qry2db
          qry2nummatch  = $qry2nummatch
          qry2wavpath   = $qry2wavpath
          qry2reftime   = $qry2reftime
          db2qry        = $db2qry
          qrydb_splitdir= $qrydb_splitdir
          SortedStats   = $SortedStats
          fscgb_lst     = $fscgb_lst

    db_shash_dir      = $db_shash_dir
        db_LUT        = $db_LUT
        db_splitshash = $db_splitshash
    qry_shash_dir     = $qry_shash_dir

    ModWavDir         = $ModWavDir

    logdir_db         = $logdir_db
    logdir_qry        = $logdir_qry

    Default Input Configs:
    ## Audio/Spectrogram
        nfft=$nfft                      \t# fft size
        min_duration=$min_duration    \t\t# Audio shorter than this number of seconds is ignored.
    ## Hashing
        MAXLISTWinSize=$MAXLISTWinSize\t\t# window length to search for peaks during hashing
        thabs_db=$thabs_db              \t# Threshold for database
        thabs_qry=$thabs_qry            \t# Threshold for query
    ## Matching
        threshold=$threshold            \t# num_matches more than this number is be considered a match
        TopN=$TopN                      \t# Early stop criteria when doing matching
    ## Misc
    "
fi

<< Parameters_search_需要检测歌曲
    MAXLISTWinSize=15; thabs_db=9;      # 0.91 MB
    MAXLISTWinSize=7; thabs_db=1;       # 0.91 MB

    ll shash/AllData*/LUT.txt
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 198288486 Sep 11 12:11 shash/AllDatabase_crop/LUT.txt
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 111968005 Aug 24 16:37 shash/AllDatabase/LUT.txt
        -rw-rw-r-- 1 lootiangkuan lootiangkuan  15096590 Sep  4 15:32 shash/AllDatabase_shashv4/LUT.txt
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 117165010 Sep 10 15:35 shash/AllDatabase_thabsDB=1/LUT.txt
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 117867997 Sep 16 17:17 shash/AllDatabase_threshold=110/LUT.txt
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 117867997 Sep 16 15:29 shash/AllDatabase_threshold=140/LUT.txt


    ll shash/AllData*/3、北京晚报（阴三儿）.shash
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 444750 Aug 24 16:37 shash/AllDatabase/3、北京晚报（阴三儿）.shash
        -rw-rw-r-- 1 lootiangkuan lootiangkuan  49143 Sep  4 15:32 shash/AllDatabase_shashv4/3、北京晚报（阴三儿）.shash
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 611398 Sep 10 15:35 shash/AllDatabase_thabsDB=1/3、北京晚报（阴三儿）.shash
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 611398 Sep 16 17:17 shash/AllDatabase_threshold=110/3、北京晚报（阴三儿）.shash
        -rw-rw-r-- 1 lootiangkuan lootiangkuan 611398 Sep 16 15:28 shash/AllDatabase_threshold=140/3、北京晚报（阴三儿）.shash
Parameters_search_需要检测歌曲
