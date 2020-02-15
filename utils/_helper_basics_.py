#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import print_function
if 1 : ## Imports
    import sys, os, datetime, traceback, pprint, pdb # pdb.set_trace()
    import subprocess, itertools, importlib , math, glob, time, random, shutil, csv, statistics
    import collections
    from operator import itemgetter
    import numpy as np
    import pickle

    ## Plots
    import matplotlib
    import matplotlib.pyplot as plt
    from matplotlib.collections import LineCollection

    ## Audio
    import wave, python_speech_features#, pyaudio
    import librosa, librosa.display
    import scipy, scipy.io, scipy.io.wavfile, scipy.signal
    import soundfile as sf
    import resampy
    import audiofile as af

    import sklearn

    import struct
    if   sys.version_info >= (3,0): ## Python 3
        1
        # import smart_open as so
    elif sys.version_info >= (2,0): ## Python 2
        import smart_open as so

if True: ## Admin
    ## Best practice in reading config
    def get_conf(conf_path):
        print('Config file :',conf_path)
        conf_dir=os.path.dirname(conf_path)
        conf_name=os.path.basename(conf_path).replace('.py','')
        sys.path.insert(0, conf_dir)
        return importlib.import_module(conf_name)
    
    ## Saving/Loading 
    def dump_load_pickle(file_Name, mode, a=None):
        if mode == 'dump':
            # open the file for writing
            fileObject = open(file_Name,'wb') 
            
            # this writes the object a to the file named 'testfile'
            pickle.dump(a,fileObject, protocol=2)   
            # cPickle.dump(a,fileObject, protocol=2)   
            
            # here we close the fileObject
            fileObject.close()
            b = 'dumped '+file_Name
        elif mode == 'load':
            # we open the file for reading
            fileObject = open(file_Name,'rb')  
            
            # load the object from the file into var b
            b = pickle.load(fileObject)  
            # b = cPickle.load(fileObject)  
            
            # here we close the fileObject
            fileObject.close()
        return b

##########################################################################################################################################################
## Audio - Admin
if True:
    def read_audio(filename_in, mode="librosa", sr=None, mean_norm=False):
        """
            Input : file name
            Return : waveform in np.float16, range [-1,1]

            mode=="scipy" : will read in int16, which we will convert to float and divide by 2**15
            
            mean_norm=True if want to return the mean_normalised waveform
            
            matlab reads in the same way as librosa do.
            
            If read audio is in different channels : use def make_single_channel()
        """

        if mode=="wave":
            with wave.open(filename_in) as w:
                data = w.readframes(w.getnframes())
            sig = np.frombuffer(data, dtype='<i2').reshape(-1, channels)
            normalized = utility.pcm2float(sig, np.float32)
            sound = normalized
            # 
            # sound_wav = wave.open(filename_in)
            # n = sound_wav.getnframes()
            # sound = sound_wav.readframes(n)
            # debug_cannot_extract_array_values
            # 
            sound_fs = sound_wav.getframerate()
        elif mode=="scipy":
            [sound_fs, sound] = scipy.io.wavfile.read(filename_in)
            if sr: 
                sound = resampy.resample(sound, sound_fs, sr, axis=0)
                sound_fs=sr
            assert sound.dtype=='int16'
            sound = 1.*sound
        elif mode=="librosa":
            # must define sr=None to get native sampling rate
            sound, sound_fs = librosa.load(filename_in,sr=sr)
            # sound *= 2**15
        elif mode=="soundfile":
            sound, sound_fs = sf.read(filename_in)
            if sr!=sound_fs: 
                sound = resampy.resample(sound, sound_fs, sr, axis=0)
                sound_fs=sr
        elif mode=="audiofile":
            sound, sound_fs = af.read(filename_in)
            if sr!=sound_fs: 
                sound = resampy.resample(sound, sound_fs, sr, axis=0)
                sound_fs=sr
        
        if mean_norm: sound -= sound.mean()
        
        return sound, sound_fs
    def display_audio(sig, title, 
        mode='wave', sr=None, hop_length=None, 
            fmin=None, fmax=None,
            x_axis='time', y_axis='hz', 
            num_bins=None,
        xlims=None,ylims=None,clims=None,
        autoplay=False, colorbar=None, 
        colour_to_set='white', curr_fig=None, kcoll=None):

        if curr_fig and kcoll: 
            ax = curr_fig.add_subplot(kcoll[0],kcoll[1],kcoll[2])

        ## Modes ----------------------------------------------
        if mode=='wave':
            librosa.display.waveplot(sig, sr=sr)
            if not title==None: plt.title(title) 
        if mode=='plot':
            plt.plot(sig)
            if not title==None: plt.title(title) 
        elif mode=='spec':
            librosa.display.specshow(sig, 
                sr=sr, hop_length=hop_length,
                fmin=fmin, fmax=fmax,
                x_axis=x_axis, y_axis=y_axis)
            if not title==None: plt.title(title) 
        elif mode=='matplot':
            plt.imshow(sig)
            if not title==None: plt.title(title) 
        elif mode=='audio':
            
            import IPython.display as ipd
            # ipd.display( ipd.Audio(""+"hello.wav") )
            # ipd.display( ipd.Audio(spkr, rate=sr) )

            if not title==None: print(title)
            ipd.display( ipd.Audio(sig, rate=sr, autoplay=autoplay) )
        elif mode=='audio_terminal':
            if not title==None: print(title)
            play_audio(sig,CHUNK=1024)
        elif mode=='image':
            import IPython.display as ipd
            # ipd.display( ipd.Image("LSTM.png") )
            # ipd.display( ipd.Image(x, format='png') )

            ipd.display( ipd.Image(sig, format='png') )
            if not title==None: plt.title(title) 
        elif mode=='constellation_points':
            """ Usage : 

                ## Read Audio
                x,sr_out=read_audio(song_path, mode="librosa", sr=sr, mean_norm=False)
                
                ## Get feats
                kwargs_STFT={
                    'pad_mode':True,
                    'mode':'librosa',
                        'n_fft':conf_sr.n_fft,
                        'win_length':conf_sr.win_length,
                        'hop_length':conf_sr.hop_length,
                        'nfft':conf_sr.nfft,
                        'winstep':conf_sr.winstep,
                        'winlen':conf_sr.winlen,
                        'fs':conf_sr.sr,
                }
                x_MAG, _,_,x_LPS=wav2LPS(x, **kwargs_STFT)
                
                ## ...Dropbox/Work/BIGO/2_Projects/002_MusicHashing/jja178/Combined/v1_baseline/utils/_Shazam_.py
                import _Shazam_ as Shazam
                kwargs_hashPeaks={
                    'num_tgt_pts':3,
                    "delay_time" : seconds_to_frameidx(1, conf_sr.hop_length, conf_sr.n_fft, conf_sr.sr),
                    "delta_time" : seconds_to_frameidx(5, conf_sr.hop_length, conf_sr.n_fft, conf_sr.sr),
                    "delta_freq" : Hz_to_freqidx(1500, conf_sr.num_freq_bins, conf_sr.sr),
                    }
                raw_peaks = Shazam.get_raw_constellation_pts(x_MAG) 
                filt_peaks = Shazam.filter_peaks(raw_peaks, conf_sr.n_fft, high_peak_percentile=75,low_peak_percentile=60)
                filt_peaks_large = [(curr_peak[0],curr_peak[1],10) for curr_peak in filt_peaks]
                # hashMatrix = Shazam.hashPeaks(filt_peaks, conf_sr, **kwargs_hashPeaks)
                [(curr_peak[0],curr_peak[1],10) for curr_peak in filt_peaks]
                
                ## Plot
                k=3;col=1;l=1; curr_fig=plt.figure(figsize=(6*col,3*k)); 
                kwargs_plot={'colour_to_set':'black','hop_length':conf_sr.hop_length,'sr':conf_sr.sr,'curr_fig':curr_fig,}
                display_audio(x,     'x',       'wave', kcoll=[k,col,l], **kwargs_plot); l+=1
                display_audio(x_LPS, 'x_LPS',   'spec', kcoll=[k,col,l], **kwargs_plot); l+=1
                display_audio([x_LPS,filt_peaks_large], 'Peaks', 'constellation_points', kcoll=[k,col,l], **kwargs_plot); l+=1
                plt.tight_layout();plt.show()
                plt.savefig('plot_path.png')
            """
            curr_lps,curr_peaks = sig
            librosa.display.specshow(curr_lps, sr=sr, hop_length=hop_length)
            plt.scatter(*zip(*curr_peaks), marker='.', color='blue', alpha=0.5)
            if not title==None: plt.title(title) 
        elif mode=='histogram':
            """ Usage : 
                x = [i for i in range(100)]
                
                ## Plot
                k=3;col=1;l=1; curr_fig=plt.figure(figsize=(6*col,3*k)); 
                kwargs_plot={'colour_to_set':'black', 'mode':histogram, 'curr_fig':curr_fig,}
                display_audio(x,     'x',       'histogram', kcoll=[k,col,l], **kwargs_plot); l+=1
                plt.tight_layout();plt.show()
                plt.savefig('plot_path.png')
            """
            plt.hist(sig, bins=num_bins)
            if not title==None: plt.title(title) 
        ## Modes ----------------------------------------------
        
        if not colorbar==None: plt.colorbar() 
        if not xlims==None: plt.xlim(xlims) 
        if not ylims==None: plt.ylim(ylims) 
        if not clims==None: plt.clim(clims) 

        if colour_to_set and curr_fig and kcoll:
            ax.spines['bottom'].set_color(colour_to_set)
            ax.spines['top'].set_color(colour_to_set)
            ax.yaxis.label.set_color(colour_to_set)
            ax.xaxis.label.set_color(colour_to_set)
            ax.tick_params(axis='both', colors=colour_to_set)
            ax.title.set_color(colour_to_set)

## Audio - Feature Extraction
if True:
    def get_paddedx_stft(x_in, n_fft):
        x_pad=x_in+0.
        if 1: # Manual
            # pad_len = n_fft // 2

            pad_left = n_fft//2
            pad_right = n_fft-pad_left
            
            ## This  pads both left and right end of the wave
            x_pad = np.append(x_in,np.zeros(pad_right))
            x_pad = np.append(np.zeros(pad_left),x_pad)
            
        elif 0:
            x_pad = librosa.util.pad_center(x_pad, len(x_pad)+n_fft, mode='constant')
        elif 0:
            ## This only pads the right-end of the wave
            x_pad = librosa.util.fix_length(x_in, len(x_in)+n_fft)

        return x_pad
    def seconds_to_frameidx(secs, hop_length, n_fft, sr):
        num_samples = secs*sr
        frame_idx = (num_samples-n_fft)/hop_length
        return int(frame_idx)+1
    def Hz_to_freqidx(hz_in, num_freq_bins, sr):
        num_freq_res = (sr/2)/num_freq_bins
        return int(hz_in / num_freq_res)
    def frameidx_to_samples(frameidx, hop_length, n_fft, sr):    return int(((frameidx-1)*hop_length+n_fft))
    def frameidx_to_seconds(frameidx, hop_length, n_fft, sr):    return int(((frameidx-1)*hop_length+n_fft)/sr)
    def frameidx_to_minsec(frameidx, hop_length, n_fft, sr):    
        Total_secs = frameidx_to_seconds(frameidx, hop_length, n_fft, sr)
        num_sec=Total_secs%60
        num_min=(Total_secs-num_sec)/60
        return [int(num_min),int(num_sec)]

    ## STFT, LPS
    def wav2LPS(x_in, spect_det=None, mode="librosa",
        n_fft=None, win_length=None, hop_length=None, nfft=None, winstep=None, winlen=None,
        pad_mode=False, fs=None, center_in=True, _window='hann'):
        """
            x_in       : shape (length)
            win_length : length of window function (111)
        """
        # n_fft, win_length, hop_length, n_mels, n_mfcc, nfft, winstep, winlen, nfilt = spect_det
        if spect_det is not None:
            return wav2LPS_v1(x_in, spect_det, pad_mode=pad_mode, mode=mode, fs=fs, center_in=center_in, _window=_window)
        if mode == "librosa":
            """
                librosa.stft(y, n_fft=2048, hop_length=None, win_length=None, 
                    window='hann', center=True, dtype=<class 'numpy.complex64'>, 
                    pad_mode='reflect')
                
                ## How to use other window functions
                https://docs.scipy.org/doc/scipy/reference/generated/scipy.signal.get_window.html#scipy.signal.get_window
                For Gaussian : window_fx = scipy.signal.get_window(('gaussian',std_dev), win_length)
                
                ## to specify filter window
                window_fx = scipy.signal.get_window(window_fx, win_length) # , window=window_fx
                
                ## To get the nearest power of 2
                if n_fft==None: 
                  n_fft = int( 2**math.ceil(math.log(win_length,2)) )
            """
            x_tmp = x_in+0.
            if pad_mode: 
                spect_det = n_fft, win_length, hop_length, 0, 0, nfft, winstep, winlen, 0
                x_tmp = get_paddedx_stft(x_tmp,n_fft)
            x_stft = librosa.stft(y=x_tmp, n_fft=n_fft, win_length=win_length, hop_length=hop_length, center=center_in, window=_window)
            x_stft_mag = np.abs(x_stft);    
            x_stft_pha = np.angle(x_stft);  
            # x_PS = (x_stft_mag**2)/n_fft
            # x_PS = np.where(x_PS == 0,np.finfo(float).eps,x_PS) # replace 0 with eps
            # x_LPS = 10*np.log10(x_PS)
        elif mode=="scipy": # This one we follow qd_gatech version of window, scaling, size
            x_tmp = x_in+0.
            if pad_mode: x_tmp = get_paddedx_stft(x_tmp,n_fft)
            hamming_window=scipy.signal.hamming(win_length)
            curr_scale=1/sum(hamming_window)**2
            curr_scale=np.sqrt(curr_scale)
            _, _, x_stft = scipy.signal.stft(x=x_tmp, fs=fs, window=hamming_window, nperseg=win_length, noverlap=hop_length, nfft=n_fft, boundary=None, padded=False)
            x_stft/=curr_scale
            x_stft_mag = np.abs(x_stft); 
            # x_stft_mag/=curr_scale
            x_stft_pha = np.angle(x_stft);  
        elif mode=="tf": # This one we follow qd_gatech version of window, scaling, size
            x_tmp = x_in+0.
            if pad_mode: x_tmp = get_paddedx_stft(x_tmp,n_fft)

            ## Make the wave into tf format
            # x_tmp = x_in.astype(np.float32)
            x_tmp = tf.reshape(x_tmp.astype(np.float32), [1,-1])
            ## Get stft
            if _window=='hann':
                curr_window=tf.signal.hann_window
            stft_out = tf.signal.stft(x_tmp,
                frame_length=n_fft, 
                frame_step=hop_length,
                window_fn=curr_window,
                pad_end=False)
            ## Get magnitude/phase
            abs_out=tf.math.abs(stft_out)
            phase_out=tf.math.angle(stft_out)
            ## Get lps
            eps1e6=tf.constant(1e-6)
            lps_out = tf.math.log(abs_out + eps1e6)
            return abs_out.numpy()[0].T, phase_out.numpy()[0].T, stft_out.numpy()[0].T, lps_out.numpy()[0].T
        ## Prevent issues with log 0
        # x_stft_mag = np.where(x_stft_mag == 0,np.finfo(float).eps,x_stft_mag) # replace 0 with eps
        # x_stft_mag += np.finfo(float).eps
        x_stft_mag += 1e-6
        ## Log
        # x_LPS = 2*np.log10( np.abs(x_stft_mag) )
        x_LPS = 2*np.log( np.abs(x_stft_mag) )
        return x_stft_mag, x_stft_pha, x_stft, x_LPS
