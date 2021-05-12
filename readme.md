# Shash in cpp


## Installations
```bash
## For Reading audio file
# install libsndfile-1.0.28
# install sndfile-tools-1.04
sudo apt-get install libsndfile-dev

## For FFT
cd utils
wget http://fftw.org/fftw-3.3.8.tar.gz
# https://forums.developer.nvidia.com/t/fftw3-build-with-sse-options/129981
tar -xf fftw-3.3.8.tar.gz
cd fftw-3.3.8
./configure --enable-sse2 # SSE2 instructions for faster fft processing
make -j16
sudo make install -j16
mv -v fftw-3.3.8/ utils/

## Conda Environment
pip install -r requirements.txt
```


# Init
reads a database wav.scp, hashing them, add to LUT, and write to file.
```bash
conda activate Shazam
cd /data3/lootiangkuan/Projects/AudioHashing/shash_cpp

DB_type=Database
Qry_type=Queries

DB_type=Register_5850.crop
Qry_type=200k

. path_shash.sh $DB_type $Qry_type
# Assert : thabs_db=1
# Assert : threshold=110

wc -l $db_wavscp $qry_wavscp
```


# Hashing
## Compiling
```bash
g++ -o ./bin/Shazam_hashing.out -Ofast -Wall -Wextra \
    -std=c++11 "Shazam_hashing.cpp" \
    "fir_api.c" "AudioLL.cpp" "Shash.cpp" \
    -lavformat -lavcodec -lavutil -lswresample \
    $(pkg-config --libs --cflags sndfile) \
    $(pkg-config --libs --cflags fftw3) \
&& echo "Finished compiling"
```
## nj=1
Reads a database wav.scp, hashing them, add to LUT, and write to file.
```bash
./bin/Shazam_hashing.out \
    $db_wavscp $db_shash_dir $db_LUT $db_db2bin \
    $nfft $MAXLISTWinSize $thabs_db $verbose $readmode \
|& tee $logdir_db/Shazam_hashing.log
wc -l $db_LUT $db_db2bin
```
## nj>1
Additional things to do here : split wav.scp, and writes to split LUT, and combine to single LUT.
```bash
## Splitting db data
split --verbose -a $num_a -d --numeric-suffixes=1 -n l/$nj $db_wavscp $db_splitdir/wav.scp.
wc -l $db_splitdir/wav.scp.*
wc -l $db_wavscp

## Run in parallel
for jid in `seq -w $nj`; do
    # ./bin/Shazam_hashing.old.8k.cropped.out \
    ./bin/Shazam_hashing.out \
        $db_splitdir/wav.scp.$jid $db_shash_dir $db_splitshash/LUT.$jid $db_splitshash/db2bin.$jid \
        $nfft $MAXLISTWinSize $thabs_db $verbose $readmode \
    > $logdir_db/Shazam_hashing.$jid.log  &
done
wait
tail $logdir_db/Shazam_hashing.*.log | grep "Duration\|logs"

## Combines the split LUT to a single LUT
conda activate Shazam
python -u utils/combineLUT.py \
    -num_a $num_a \
    -nj $nj \
    -db_LUT $db_LUT \
    -db_splitshash $db_splitshash \
> $logdir_db/combineLUT.log && \
cat $db_splitshash/db2bin.* > $db_db2bin && \
wc -l $db_LUT $db_db2bin
```
Some TODOs to improve/optimise hashing
```
(Todo) Is filtering the short TimeOffset faster?
(Todo) Use unordered_set instead of set?
(Todo) get_Timeoffsets_v2() : Get intersection of two unordered_map faster ?
(Todo) Investigate combining all database into 1 song, and shash it.
    (+) Reduce need for multiple matching.
    (-) More chance to perform matching for all queries, and it is against the whole database instead of per song.
    (?)
    How this affect matching speed?
    How this affect Precision PushRate QPS?
    What about time taken to find the position of the song, and thus the song id?
```



# Querying
## Compiling
```bash
# g++ -o ./bin/Shazam_querying.out -Ofast -Wall -Wextra \
# g++ -o ./bin/Shazam_querying_Full.out -Ofast -Wall -Wextra \
g++ -o ./bin/Shazam_querying.longinput.out -Ofast -Wall -Wextra \
    -std=c++11 "Shazam_querying.longinput.cpp" \
    "fir_api.c" "AudioLL.cpp" "Shash.cpp" \
    -lavformat -lavcodec -lavutil -lswresample \
    $(pkg-config --libs --cflags sndfile) \
    $(pkg-config --libs --cflags fftw3) \
&& echo "Finished compiling"
```

## (nj=1)
Reads query wav.scp, hash them, find candidates from db LUT, and matches them.
```bash
./bin/Shazam_querying.longinput.out \
    $db_wavscp $db_shash_dir $db_LUT $db_db2bin \
    $qry_wavscp $qry2db $qry_shash_dir $qry2nummatch $qry2wavpath \
    $nfft $MAXLISTWinSize $thabs_qry $threshold \
    $TopN $min_duration $verbose \
|& tee $logdir_qry/Shazam_querying.log

perl utils/utt2spk_to_spk2utt.pl <(cut -d' ' -f-2 $qry2db) | sort > $db2qry
wc -l $qry2db $db2qry
```

## (nj>1)
Additional things to do here : Combine split qry2db to single qry2db.
```bash
## Split
split --verbose -a $num_a -d --numeric-suffixes=1 -n l/$nj $qry_wavscp $qry_splitdir/wav.scp.
wc -l $qry_splitdir/wav.scp.* | sort -n
wc -l $qry_wavscp

## Matching
for jid in `seq -w $nj`; do
    ./bin/Shazam_querying.longinput.out \
    $db_wavscp $db_shash_dir $db_LUT $db_db2bin \
    $qry_splitdir/wav.scp.$jid $qrydb_splitdir/qry2db.$jid $qry_shash_dir $qrydb_splitdir/qry2nummatch.$jid $qrydb_splitdir/qry2wavpath.$jid \
    $nfft $MAXLISTWinSize $thabs_qry \
    $threshold $TopN $min_duration $verbose \
    >> $logdir_qry/Shazam_querying.longinput.$jid.log &
done
wait

wc -l $qry_splitdir/wav.scp.*
wc -l $qrydb_splitdir/qry2nummatch.*
wc -l $qrydb_splitdir/qry2db.*
wc -l $qrydb_splitdir/qry2wavpath.*

## Combine the results
if true; then
    cat $qrydb_splitdir/qry2nummatch.* | sed "/ 0/d" > $qry2nummatch && wc -l $qry2nummatch & # qry2nummatch : Ignore NULL matches
    cat $qrydb_splitdir/qry2db.* > $qry2db && wc -l $qry2db
    perl utils/utt2spk_to_spk2utt.pl <(cut -d' ' -f-2 $qry2db) | sort > $db2qry && wc -l $db2qry
    cat $qrydb_splitdir/qry2wavpath.* > $qry2wavpath && wc -l $qry2wavpath
    wait
    sort -rn -k2 $qry2nummatch > $qry2nummatch.sorted && head $qry2nummatch.sorted && wc -l $qry2nummatch.sorted &
    wait
    wc -l $qry2db $db2qry $qry2wavpath
fi
```

## Align matched SegmentID to WavID
```bash
cp -v $qry2db $qry2db.crop
cp -v $db2qry $db2qry.crop
cp -v $qry2nummatch $qry2nummatch.crop
##
conda activate Shazam
python -u utils/AlignCropOutputs.py \
    -segment2db $db_data_dir/segment2db \
    -qry2db $qry2db -db2qry $db2qry -qry2nummatch $qry2nummatch \
|& tee $logdir_qry/AlignCropOutputs.Total.log
##
wc -l $db_data_dir/segment2db $qry2db       $db2qry       $qry2nummatch
wc -l                         $qry2db.crop  $db2qry.crop  $qry2nummatch.crop
```

## View the results
```bash
# nj=1
wc -l $db_wavscp $qry_wavscp $db2qry $qry2db $qry2nummatch
# nj>1
wc -l $db2qry $qry2db $qry2wavpath $qry2nummatch $qry2nummatch.sorted
```

## Calculate QPS
```bash
if true; then
    ## Overall Stats
    NumPositive=$(wc -l $qry2db | cut -d' ' -f1)
    NumFiles=$(wc -l $qry_wavscp | cut -d' ' -f1)
    PushRate=$(bc -l <<< "100 * $NumPositive/$NumFiles")
    echo "PushRate=$PushRate"

    TimeSpent=$(tail -n3 $logdir_qry/Shazam_querying.* | grep Duration | sort -k3 -n | tail -n1 | cut -d" " -f6)
    # TimeSpent=$(bc -l <<< "$TimeSpent - 3.9") # account for reading database
    NumFiles=$(wc -l $qry_wavscp | cut -d' ' -f1)
    echo "TimeSpent=$TimeSpent"
    echo "NumFiles=$NumFiles"

    QPS_ALL=$(bc -l <<< "$NumFiles/$TimeSpent")
    echo "QPS_ALL=$QPS_ALL"
fi

grep "Cannot" $logdir_qry/Shazam_querying.*.log | wc -l
grep "hash_DICT_qry.size" $logdir_qry/Shazam_querying.*.log | wc -l
QPS_Proc=$(bc -l <<< "($NumFiles-7180)/$TimeSpent")
echo "QPS_Proc=$QPS_Proc"

NumberOfFiles=$(cat $qrydb_splitdir/qry2nummatch.* | cut -d' ' -f1 | uniq | wc -l)
QPS_Matched=$(bc -l <<< "$NumberOfFiles/$TimeSpent")
echo "QPS_Matched=$QPS_Matched"
```
-------
-------


# Convert to excel for results
```bash
# Write details to $SortedStats
# Copy wave files to $ModWavDir
conda activate Shazam
python -u utils/ForModerationTeam.py \
    -DB_type $DB_type -Qry_type $Qry_type \
    -qry_wavscp $qry_wavscp -qrydb_dir $qrydb_dir \
    -qry2db $qry2db -qry2wavpath $qry2wavpath -qry2reftime $qry2reftime -db2qry $db2qry \
    -SortedStats $SortedStats -ModWavDir $ModWavDir \
|& tee $logdir_qry/ForModerationTeam.Total.log

ps -ef | grep "lootian" | grep port
port=1235
audio_extension=wav # mp3
nohup ./utils/fscgb resume --port=$port  --extensions=$audio_extension --rootpath=$ModWavDir --output=$fscgb_lst |& tee $logdir_qry/fscgb.log &

ip_add=127.00.000.00
sed -i "s,\tfscgb,\thttp://${ip_add}:$port,g" $SortedStats
head  $SortedStats
wc -l $SortedStats $fscgb_lst
```

## Calculate Precision/Accuracy...
```bash
# Write details to $qrydb_dir/{qry2Wrong, qry2FalsePositives, qry2FalseNegatives}
wc -l $SortedStats $qrydb_dir/qry2db2label
head $SortedStats $qrydb_dir/qry2db2label

conda activate Py3_debug
python -u utils/GetStats.py \
    -qry2db $qry2db \
    -qry2nummatch $qry2nummatch \
    -qry2db2label $qrydb_dir/qry2db2label \
        -qry2Wrong $qrydb_dir/qry2Wrong \
        -qry2FalsePositives $qrydb_dir/qry2FalsePositives \
        -qry2FalseNegatives $qrydb_dir/qry2FalseNegatives \
|& tee $logdir_qry/GetStats.log

wc -l $qrydb_dir/{qry2Wrong,qry2FalsePositives,qry2FalseNegatives}
head  $qrydb_dir/{qry2Wrong,qry2FalsePositives,qry2FalseNegatives}
```

Some TODOs to improve/optimise matching
```
(Todo) How to improve the CandidateSet selection.
    Currently the longer audio are more likely be on higher queue.
    How to use density of matched hashes instead of just absolute number of hashes.
(Todo) check why get_adjPts_v1() get_adjPts_v2() get_adjPts_v3() gives different results.
(Todo) Find accurate time reference stamp
(To try) Identify peak in histogram as a ratio of local max, instead of a fixed threshold.
```
-------
-------
