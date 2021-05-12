from __future__ import print_function
if 1: ## imports / admin
    import os,sys, pdb,datetime, argparse, pprint
    sys.path.insert(0, 'utils')
    from _helper_basics_ import *
    from _Shazam_ import *
    START_TIME=datetime.datetime.now()
    print(f"===========\npython {' '.join(sys.argv)}\n    Start_Time:{START_TIME}\n===========")
    #
    pp = pprint.PrettyPrinter(indent=4)
    if True:
        parser=argparse.ArgumentParser()
        parser.add_argument('-wavpath',)
        parser.add_argument('-spec_path',)
        args=parser.parse_args()
        wavpath=args.wavpath
        spec_path=args.spec_path
    print('############ End of Config Params ##############')

#################################################################
n_fft=512
#################################################################

print(f"Reading from {wavpath}")
x,sr_out=read_audio(wavpath, mode="librosa", sr=8000, mean_norm=False)
win_length=n_fft
hop_length=n_fft//2
kwargs_STFT={
    'pad_mode':True,
    'mode':'librosa',
        'n_fft':n_fft,
        'win_length':n_fft,
        'hop_length':n_fft//2,
        'fs':sr_out,
}
x_MAG, _,_,x_LPS=wav2LPS(x, **kwargs_STFT)

if   1 : # Look as plots from c++'s fft and python's fft
    #################################################################
    for numframes, _ in enumerate(open(spec_path, 'r'), 1): numframes
    y_MAG = np.zeros((numframes, n_fft//2))
    for idx, curr_frame_str in enumerate(open(spec_path, 'r')): y_MAG[idx] = np.array(curr_frame_str.strip(' \n').split(' '))
    y_MAG = y_MAG.T
    #################################################################
    plt.figure(figsize=(8,8));
    tmp = x_LPS[1:,1:-1]
    tmp = tmp[:,1:-1]
    print(tmp.shape)
    cmap = librosa.display.cmap(tmp)
    plt.subplot(2,1,1); librosa.display.specshow(tmp, sr=sr_out, hop_length=hop_length, x_axis='time', y_axis='hz', cmap=cmap)
    plt.title("python")
    #
    tmp = np.log((y_MAG)+1e-6)
    tmp = tmp[:,1:-1]
    print(tmp.shape)
    cmap = librosa.display.cmap(tmp)
    plt.subplot(2,1,2); librosa.display.specshow(tmp, sr=sr_out, hop_length=hop_length, x_axis='time', y_axis='hz', cmap=cmap)
    plt.title("c++")
    #
    # tmp = np.log((y_MAG)**.5+1e-6)
    # cmap = librosa.display.cmap(tmp)
    # plt.subplot(3,1,3); librosa.display.specshow(tmp, sr=sr_out, hop_length=hop_length, x_axis='time', y_axis='hz', cmap=cmap)
    #
    plt.tight_layout();plt.show()
    plt.savefig('plot_path.png')
    plt.close()
elif 0 : # Look as plots different n_fft
    k=3;col=1;l=1; curr_fig=plt.figure(figsize=(9*col,3*k));
    kwargs_plot={'colour_to_set':'black', 'curr_fig':curr_fig, 'sr':8000}
    #
    n_fft=256
    kwargs_STFT={
        'pad_mode':True,
        'mode':'librosa',
            'n_fft':n_fft,
            'win_length':n_fft,
            'hop_length':n_fft//2,
            'fs':sr_out,
        }
    kwargs_plot['hop_length']=n_fft//2
    x_MAG, _,_,x_LPS=wav2LPS(x, **kwargs_STFT)
    x_peaks = get_raw_constellation_pts(x_MAG, f_dim1=10, t_dim1=10, threshold_abs=1, base=20)
    x_peaks = [(curr_peak[0],curr_peak[1],10) for curr_peak in x_peaks]
    # display_audio(x_MAG, n_fft,   'spec', kcoll=[k,col,l], **kwargs_plot); l+=1
    display_audio([x_LPS, x_peaks], 'Peaks', 'constellation_points', kcoll=[k,col,l], **kwargs_plot); l+=1
    #
    n_fft=372
    kwargs_STFT={
        'pad_mode':True,
        'mode':'librosa',
            'n_fft':n_fft,
            'win_length':n_fft,
            'hop_length':n_fft//2,
            'fs':sr_out,
        }
    kwargs_plot['hop_length']=n_fft//2
    x_MAG, _,_,x_LPS=wav2LPS(x, **kwargs_STFT)
    x_peaks = get_raw_constellation_pts(x_MAG, f_dim1=10, t_dim1=10, threshold_abs=1, base=20)
    x_peaks = [(curr_peak[0],curr_peak[1],10) for curr_peak in x_peaks]
    # display_audio(x_MAG, n_fft,   'spec', kcoll=[k,col,l], **kwargs_plot); l+=1
    display_audio([x_LPS, x_peaks], 'Peaks', 'constellation_points', kcoll=[k,col,l], **kwargs_plot); l+=1
    #
    n_fft=512
    kwargs_STFT={
        'pad_mode':True,
        'mode':'librosa',
            'n_fft':n_fft,
            'win_length':n_fft,
            'hop_length':n_fft//2,
            'fs':sr_out,
        }
    kwargs_plot['hop_length']=n_fft//2
    x_MAG, _,_,x_LPS=wav2LPS(x, **kwargs_STFT)
    x_peaks = get_raw_constellation_pts(x_MAG, f_dim1=10, t_dim1=10, threshold_abs=1, base=20)
    x_peaks = [(curr_peak[0],curr_peak[1],10) for curr_peak in x_peaks]
    # display_audio(x_MAG, n_fft,   'spec', kcoll=[k,col,l], **kwargs_plot); l+=1
    display_audio([x_LPS, x_peaks], 'Peaks', 'constellation_points', kcoll=[k,col,l], **kwargs_plot); l+=1
    #
    plt.tight_layout();plt.show()
    plt.savefig('plot_path.png')
    plt.close()
elif 0 : # plot peaks from python peaks
    #################################################################
    n_fft=512
    kwargs_STFT={
        'pad_mode':True,
        'mode':'librosa',
            'n_fft':n_fft,
            'win_length':n_fft,
            'hop_length':n_fft//2,
            'fs':sr_out,
        }
    kwargs_plot['hop_length']=n_fft//2
    x_MAG, _,_,x_LPS=wav2LPS(x, **kwargs_STFT)
    #################################################################
    k=3;col=1;l=1; curr_fig=plt.figure(figsize=(9*col,3*k));
    kwargs_plot={'colour_to_set':'black', 'curr_fig':curr_fig, 'sr':8000}
    x_peaks = get_raw_constellation_pts(x_MAG, f_dim1=10, t_dim1=10, threshold_abs=1, base=20)
    x_peaks = [(curr_peak[0],curr_peak[1],10) for curr_peak in x_peaks]
    # display_audio(x_MAG, n_fft,   'spec', kcoll=[k,col,l], **kwargs_plot); l+=1
    display_audio([x_LPS, x_peaks], 'Peaks', 'constellation_points', kcoll=[k,col,l], **kwargs_plot); l+=1
    plt.tight_layout();plt.show()
    plt.savefig('plot_path.png')
    plt.close()
    #################################################################
elif 1 : # Plot filtered by threshold
    #################################################################
    for numframes, _ in enumerate(open(spec_path, 'r'), 1): numframes
    y_MAG = np.zeros((numframes, n_fft//2))
    for idx, curr_frame_str in enumerate(open(spec_path, 'r')): y_MAG[idx] = np.array(curr_frame_str.strip(' \n').split(' '))
    y_MAG = y_MAG.T
    #################################################################
    thabs_qry = 0.25*(2**15)
    thabs_qry = 0.5*(2**15)
    tmp = y_MAG * (y_MAG>thabs_qry)
    # tmp = np.ma.array(y_MAG, mask = y_MAG>thabs_qry)
    tmp = np.log((tmp)+1e-6)
    print(tmp.shape)
    plt.figure(figsize=(8,8));
    librosa.display.specshow(tmp, sr=sr_out, hop_length=hop_length, x_axis='time', y_axis='hz')
    plt.title(f"thabs_qry={thabs_qry}")
    plt.tight_layout();plt.show()
    # plt.savefig('plot_path.png')
    plt.savefig('plot_path1.png')
    plt.close()
    #################################################################

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

python UnitTesting/VisualiseFFT.py -wavpath $wavpath -spec_path $spec_path

"""
