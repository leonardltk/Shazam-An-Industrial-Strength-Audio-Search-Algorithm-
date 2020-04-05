# Shazam : An Industrial Strength Audio Search Algorithm
```
Adapted from, hence credits to
https://github.com/bmoquist/Shazam
https://github.com/miishke/PyDataNYC2015
```

## Init
```bash
cd Shazam-An-Industrial-Strength-Audio-Search-Algorithm-
. path.sh
# Make sure your ./data/*/wav.scp are tab-seperated
```

*   ./utils/conf.py
    ```python
    class Shazam_conf():
    ...
    self.threshold_short=10 # For comparing short sequences ~10s
    self.threshold_long=300 # For comparing long sequences ~3min
    self.th=self.threshold_short
    ```

    By default, we assume we are matching short audio sequence against the database.
    If however, your audio is full song duration, set `self.th=self.threshold_long` to give less False Negatives.

# Database : Hashing & Setting Up Look-Up Table (LUT)
```bash
## Hashing the database, and storing them to a dictionary
python -u ./utils/Hashing.py -DB_type $DB_type &> $logdir/Hashing.log
```

# Queries : Hashing & Matching
## Data (Splitting the queries to $nj jobs for parallel processing)
```bash
split --verbose -a $num_a -d --numeric-suffixes=1 -n l/$nj $qry_wavscp $splitdir/wav.scp.
    wc -l $qry_wavscp
    wc -l $splitdir/wav.scp.*
```
## Matching (Queries)
```bash
# Set cont=True in path.sh if you are continuing from a prematurely stopped script
for jid in `seq -w $nj`; do
    python -u ./utils/Matching.py \
        -DB_type $DB_type \
        -Qry_type $Qry_type \
        -jid $jid \
        -cont $cont \
        &>> $logdir/Matching.$jid.log &
done
wait

# Do this if u want to observe what are some of the matches you have
grep " matched " $logdir/Matching.*
```

## Combine
```bash
# This combines all the outputs of the split tasks together
python -u ./utils/Combine.py \
    -DB_type $DB_type \
    -Qry_type $Qry_type \
    -nj $nj \
    -zfill $num_a \
    &> $logdir/Combine.log
wc -l $qry2db $db2qry
```
# GetStats
```bash
# Here we get the num_matches, for statistical overview of how good the matches are.
python -u ./utils/GetStats.py \
    -DB_type $DB_type \
    -Qry_type $Qry_type \
    &> $logdir/GetStats.log
```
