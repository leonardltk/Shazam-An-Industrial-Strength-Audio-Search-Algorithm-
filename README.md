# Shazam-An-Industrial-Strength-Audio-Search-Algorithm-
### Requirements
```
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
```

### Hashing (Database)
```bash
conf_qry=./conf/DB_v1/conf2_db.py
conf_sr=./conf/Shazam/conf1_sr8k_Shazam.py
conf_hash=./conf/Shazam/conf3_Shazam.py

python -u ./utils/Hashing.py \
            -conf_qry $conf_qry \
            -conf_sr $conf_sr \
            -conf_hash $conf_hash
```

### Hashing (Queries)
```bash
conf_qry=./conf/Qry_v1/conf2_qry.py
conf_sr=./conf/Shazam/conf1_sr8k_Shazam.py
conf_hash=./conf/Shazam/conf3_Shazam.py

python -u ./utils/Hashing.py \
            -conf_qry $conf_qry \
            -conf_sr $conf_sr \
            -conf_hash $conf_hash
```


### Matching
```bash
conf_db=./conf/DB_v1/conf2_db.py
conf_qry=./conf/Qry_v1/conf2_qry.py
conf_sr=./conf/Shazam/conf1_sr8k_Shazam.py
conf_hash=./conf/Shazam/conf3_Shazam.py

python -u ./utils/Matching.py \
            -conf_db $conf_db \
            -conf_qry $conf_qry \
            -conf_sr $conf_sr \
            -conf_hash $conf_hash

```
