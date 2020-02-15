from __future__ import print_function
if 1: ## imports / admin
    import os

verbose_conf=True
# verbose_conf=False

############ Audio Wave Parameters ############
if 1:
    sr=sr_8k=8000

    n_fft_time=0.0464
    winlen=0.0464
    winstep=0.0232

    n_fft=int(sr*n_fft_time)+1      # int()+1 because want to round up 
    win_length=int(sr*winlen)+1     # int()+1 because want to round up 
    hop_length=int(sr*winstep)+1    # int()+1 because want to round up 

    # n_fft_time=1.*n_fft/sr
    # winlen=1.*win_length/sr
    # winstep=1.*hop_length/sr
    
    num_freq_bins=n_fft//2 +1
    
    if verbose_conf:
        print('\n    ############ Audio Wave Parameters ############')
        print('    sr               :',sr)
        print('    n_fft            :',n_fft)
        print('    win_length       :',win_length)
        print('    hop_length       :',hop_length)
        print('    winlen           :',winlen)
        print('    winstep          :',winstep)
        print('    num_freq_bins    :',num_freq_bins)
############ Audio Feat Parameters ############
if 1:
    n_mels=26
    n_mfcc=20
    nfft=n_fft+0
    nfilt=n_mels+0
    spect_det = [n_fft, win_length, hop_length, n_mels, n_mfcc, nfft,  winstep,    winlen,     nfilt,  ]
    if verbose_conf:
        print('\n    ############ Audio Feat Parameters ############')
        print('    n_mels           :',n_mels)
        print('    n_mfcc           :',n_mfcc)
        print('    nfft             :',nfft)
        print('    nfilt            :',nfilt)
        print('    spect_det        :',spect_det)
