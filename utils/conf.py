from __future__ import print_function
import os

class DB_conf():
    def __init__(self, DB_type):
        ############ Data Dir/Path ############
        self.DB_type=DB_type
        #### data
        self.data_dir=os.path.join('data',DB_type); os.makedirs(self.data_dir,exist_ok=True)
        ## Read details
        self.wav_scp=os.path.join(self.data_dir,'wav.scp')
        self.shash_scp=os.path.join(self.data_dir,'shash.scp')

        ############ Wav Dir/Path ############
        #### wav
        self.wav_dir=os.path.join('wav',self.DB_type); os.makedirs(self.wav_dir,exist_ok=True)

        ############ Hash Dir/Path ############
        #### exp
        self.exp_dir=os.path.join('exp',DB_type); os.makedirs(self.exp_dir,exist_ok=True)
        ## From Data
        self.exp_wavscp_dict=os.path.join(self.exp_dir,'wavscp_DICT')
        ## Hash
        self.exp_utt2hashpath_DICT=os.path.join(self.exp_dir,'utt2hashpath_DICT')
        self.exp_utt2shash_DICT=os.path.join(self.exp_dir,'utt2shash_DICT')
        self.exp_hashdir=os.path.join(self.exp_dir,'hashes'); os.makedirs(self.exp_hashdir,exist_ok=True)
        self.exp_LUT=os.path.join(self.exp_dir,'LUT')

    def __str__(self):
        print('\n############ DB_conf ############')
        print('\t\tDB_type                  :',self.DB_type)
        print('\t############ Data Dir/Path ############')
        print('\t\tdata_dir                 :',self.data_dir)
        print('\t\twav_scp                  :',self.wav_scp)
        print('\t\tshash_scp                :',self.shash_scp)
        print('\t############ Wav Dir/Path ############')
        print('\t\twav_dir                  :',self.wav_dir)
        print('\t############ Hash Dir/Path ############')
        print('\t\texp_dir                  :',self.exp_dir)
        print('\t\t  exp_wavscp_dict        :',self.exp_wavscp_dict)
        print('\t\t  exp_utt2hashpath_DICT  :',self.exp_utt2hashpath_DICT)
        print('\t\t  exp_utt2shash_DICT     :',self.exp_utt2shash_DICT)
        print('\t\t  exp_hashdir            :',self.exp_hashdir)
        print('\t\t  exp_LUT                :',self.exp_LUT)
        return ""


class QRY_conf():
    def __init__(self, QRY_type, jid='00'):
        self.QRY_type=QRY_type
        ############ Data Dir/Path ############
        #### data
        self.data_dir=os.path.join('data',self.QRY_type); os.makedirs(self.data_dir,exist_ok=True)
        ## Read details
        self.wav_scp=os.path.join(self.data_dir,'wav.scp')
        ## Match details
        self.qry2db=os.path.join(self.data_dir,'qry2db')
        ## Split details
        self.split_dir=os.path.join(self.data_dir,'split')
        if 1:
            self.split_wav_scp=os.path.join(self.split_dir,f'wav.scp.{jid}')
            self.split_qry2db=os.path.join(self.split_dir,f'qry2db.{jid}')

        ############ Wav Dir/Path ############
        #### wav
        self.wav_dir=os.path.join('wav',self.QRY_type); os.makedirs(self.wav_dir,exist_ok=True)

        ############ Hash Dir/Path ############
        #### exp
        self.exp_dir=os.path.join('exp',self.QRY_type); os.makedirs(self.exp_dir,exist_ok=True)
        if 1:
            ## From Data
            self.exp_wavscp_dict=os.path.join(self.exp_dir,'wavscp_DICT')
            self.exp_qry2db_dict=os.path.join(self.exp_dir,'qry2db_DICT')
            ## Experiments
            self.exp_Unmatched_Dict=os.path.join(self.exp_dir,'Unmatched_Dict')
            self.exp_Matched_Dict=os.path.join(self.exp_dir,'Matched_Dict')
            self.exp_Matched_Shash_Dict=os.path.join(self.exp_dir,'Matched_Shash_Dict')
            ## Hash
            self.exp_hashdir=os.path.join(self.exp_dir,'hashes'); os.makedirs(self.exp_hashdir,exist_ok=True)
            ## Split details
            self.split_exp=os.path.join(self.exp_dir,'split'); os.makedirs(self.split_exp,exist_ok=True)
            if 1:
                self.split_wavscp_dict=os.path.join(self.split_exp,f'wavscp_DICT.{jid}')
                self.split_Unmatched_Dict=os.path.join(self.split_exp,f'Unmatched_Dict.{jid}')
                self.split_Matched_Dict=os.path.join(self.split_exp,f'Matched_Dict.{jid}')
                self.split_Matched_Shash_Dict=os.path.join(self.split_exp,f'Matched_Shash_Dict.{jid}')

    def __str__(self):
        print('\n############ QRY_conf ############')
        print('\t\tQRY_type             :',self.QRY_type)
        print('\t############ Data Dir/Path ############')
        print('\t\tdata_dir             :',self.data_dir)
        print('\t\twav_scp              :',self.wav_scp)
        print('\t\tqry2db               :',self.qry2db)
        print('\t\tsplit_dir            :',self.split_dir)
        print('\t\t  split_wav_scp      :',self.split_wav_scp)
        print('\t\t  split_qry2db       :',self.split_qry2db)
        print('\t############ Wav Dir/Path ############')
        print('\t\twav_dir                  :',self.wav_dir)
        print('\t############ Hash Dir/Path ############')
        print('\t\texp_dir                  :',self.exp_dir)
        print('\t\t  exp_hashdir            :',self.exp_hashdir)
        print('\t\t  exp_wavscp_dict        :',self.exp_wavscp_dict)
        print('\t\t  exp_qry2db_dict        :',self.exp_qry2db_dict)
        print('\t\t  exp_Unmatched_Dict     :',self.exp_Unmatched_Dict)
        print('\t\t  exp_Matched_Dict       :',self.exp_Matched_Dict)
        print('\t\t  exp_Matched_Shash_Dict :',self.exp_Matched_Shash_Dict)
        print('\t\t  split_exp              :',self.split_exp)
        print('\t\t    split_wavscp_dict    :',self.split_wavscp_dict)
        print('\t\t    split_Unmatched_Dict :',self.split_Unmatched_Dict)
        print('\t\t    split_Matched_Dict   :',self.split_Matched_Dict)
        print('\t\t    split_Matched_Shash_Dict   :',self.split_Matched_Shash_Dict)
        return ""


class Shazam_conf():
    def __init__(self):
        ############ Shazam Constellation Point Details ############
        if 1:
            ## get_raw_constellation_pts()
            self.running_percentile_val=50
            """ running_percentile_val : range from [0,100]
                Defaults to 70.
                Gets a sliding percentile instead of the whole spectrogram.
                Used to calculate threshold, which determines minimum amplitude required to be a peak.
            """

            self.f_dim1, self.t_dim1=10,10
            """ The window of the spectrogram for finding candidate peak

                At a particular index i,j:
                    we find the location of the max value of S[i:i+t_dim1,j:j+f_dim1]
                    (i,j+f_dim1)---------------------------(i+t_dim1,j+f_dim1)
                        |                                       |
                        |        S[i:i+t_dim1,j:j+f_dim1]       |
                        |                                       |
                      (i,j)-----------------------------------(i+t_dim1,j)

                argmax of this given window is a candidate constellation point
            """

            self.f_dim2, self.t_dim2=5,5
            """ Threshold for local max.
                Typically smaller than f_dim1,t_dim1

                At a particular index i,j:
                    we eliminate these candidates if they are not the argmax of their neighbours
                    (i-t_dim2,j+f_dim2)---------------------------(i+t_dim2,j+f_dim2)
                        |                                                   |
                        |                          (i,j)                    |
                        |                                                   |
                    (i-t_dim2,j-f_dim2)---------------------------(i+t_dim2,j-f_dim2)
            """

            self.base=10
            # base : freq bin lower than this index is not considered as peak

            ## filter_peaks()
            self.high_peak_percentile=65
            self.low_peak_percentile=65
        ############ Shazam Hash Generation Details ############
        if 1:
            self.num_tgt_pts=3
            # Get the top num_tgt_pts points in the target zone as hash

            self.delay_time_secs=1   # Time delayed before seeking the next
            self.delta_time_secs=5   # Time width of Target Zone 
            self.delta_freq_Hz=1500  # Freq width of Target Zone, will be +- half of this from the anchor point
        ############ Shazam Hash Clustering Details ############
        if 1:
            # If threshold_cluster is more than this amount, we classify them as within the same cluster
            self.threshold_short=10 # For comparing short sequences ~10s
            self.threshold_long=300 # For comparing long sequences ~3min
            self.th=self.threshold_short
            self.cluster_verbose=False

    def __str__(self):
        print('\n############ Shazam ############')
        print('\t############ Shazam Constellation Point Details ############')
        print('\t\trunning_percentile_val   :',self.running_percentile_val)
        print('\t\tf_dim1                   :',self.f_dim1)
        print('\t\tt_dim1                   :',self.t_dim1)
        print('\t\tf_dim2                   :',self.f_dim2)
        print('\t\tt_dim2                   :',self.t_dim2)
        print('\t\tbase                     :',self.base)
        print('\t\thigh_peak_percentile     :',self.high_peak_percentile)
        print('\t\tlow_peak_percentile      :',self.low_peak_percentile)
        print('\t############ Shazam Hash Generation Details ############')
        print('\t\tnum_tgt_pts              :',self.num_tgt_pts)
        print('\t\tdelay_time_secs          :',self.delay_time_secs)
        print('\t\tdelta_time_secs          :',self.delta_time_secs)
        print('\t\tdelta_freq_Hz            :',self.delta_freq_Hz)
        print('\t############ Shazam Hash Clustering Details ############')
        print('\t\tth                       :',self.th)
        print('\t\tcluster_verbose          :',self.cluster_verbose)
        return ""


class SR_conf():
    def __init__(self):
        ############ Audio Wave Parameters ############
        if 1:
            self.sr=8000

            self.n_fft_time=0.0464
            self.winlen=0.0464
            self.winstep=0.0232

            self.n_fft=int(self.sr*self.n_fft_time)+1      # int()+1 because want to round up 
            self.win_length=int(self.sr*self.winlen)+1     # int()+1 because want to round up 
            self.hop_length=int(self.sr*self.winstep)+1    # int()+1 because want to round up 
            
            self.num_freq_bins=self.n_fft//2 +1
        ############ Audio Feat Parameters ############
        if 1:
            self.n_mels=26
            self.n_mfcc=20
            self.nfft=self.n_fft+0
            self.nfilt=self.n_mels+0

    def __str__(self):
        print('\n############ SR_conf ############')
        print('\t############ Audio Wave Parameters ############')
        print('\t\tsr               :',self.sr)
        print('\t\tn_fft            :',self.n_fft)
        print('\t\twin_length       :',self.win_length)
        print('\t\thop_length       :',self.hop_length)
        print('\t\twinlen           :',self.winlen)
        print('\t\twinstep          :',self.winstep)
        print('\t\tnum_freq_bins    :',self.num_freq_bins)
        print('\t############ Audio Feat Parameters ############')
        print('\t\tn_mels           :',self.n_mels)
        print('\t\tn_mfcc           :',self.n_mfcc)
        print('\t\tnfft             :',self.nfft)
        print('\t\tnfilt            :',self.nfilt)
        return ""
