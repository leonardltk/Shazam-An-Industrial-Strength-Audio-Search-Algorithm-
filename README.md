# Shazam : An Industrial Strength Audio Search Algorithm
```
Adapted from, hence credits to
https://github.com/bmoquist/Shazam
https://github.com/miishke/PyDataNYC2015
```

## Init
```bash
cd ./Shazam-An-Industrial-Strength-Audio-Search-Algorithm-
. path.sh
```

## Data (Splitting the queries to $nj jobs for parallel processing)
```bash
mkdir -pv $splitdir
split --verbose -a $num_a -d --numeric-suffixes=1 -n l/$nj $qry_wavscp $splitdir/wav.scp.
    wc -l $qry_wavscp
    wc -l $splitdir/wav.scp.*
```

## Hashing (Database)
```bash
## Hashing the database, and storing them to a dictionary
python -u ./utils/Hashing.py -DB_type $DB_type |& tee $logdir/Hashing.log
```

## Matching (Queries)
```bash
# cont=True # Set to True if you are continuing from a prematurely stopped script
for jid in `seq -w $nj`; do
    python -u ./utils/Matching.py \
        -DB_type $DB_type \
        -Qry_type $Qry_type \
        -jid $jid \
        -cont $cont \
        &>> $logdir/Matching.$jid.log &
done
wait

grep " matched " $logdir/Matching.*
```

## Combine
```bash
python -u ./utils/Combine.py \
    -DB_type $DB_type \
    -Qry_type $Qry_type \
    -nj $nj \
    -zfill $num_a \
    |& tee $logdir/Combine.log

perl utils/utt2spk_to_spk2utt.pl $qry2db | sort > $db2qry
    wc -l $qry2db $db2qry
```
