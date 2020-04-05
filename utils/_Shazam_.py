from __future__ import print_function
if 1: ## imports / admin
    import os,sys,pdb # pdb.set_trace() # !import code; code.interact(local=vars())
    from _helper_basics_ import *

################################################################
## Hash
################################################################
def get_hash_kwargs(conf_sr,conf_shazam,):
    """ Create a parameters for the hashing functions.
        
        Usage:
        ------

        Inputs : 
        ---------
            x           : array of waveform

            mode        : str
                "phash"     : 
                "shazam"    : 

        Returns : 
        ---------
            kwargs_in : dict
                kwargs_peaks        : for getting contellation point, and filtering them
                kwargs_hashPeaks    : For hashing the points
    """

    kwargs_peaks={
        'running_percentile_val':conf_shazam.running_percentile_val,
        'f_dim1':conf_shazam.f_dim1,
        't_dim1':conf_shazam.t_dim1,
        'f_dim2':conf_shazam.f_dim2,
        't_dim2':conf_shazam.t_dim2,
        'base':conf_shazam.base,
        'high_peak_percentile':conf_shazam.high_peak_percentile,
        'low_peak_percentile':conf_shazam.low_peak_percentile,
    }
    kwargs_hashPeaks={
        'num_tgt_pts': conf_shazam.num_tgt_pts,
        "delay_time" : seconds_to_frameidx(conf_shazam.delay_time_secs, conf_sr.hop_length, conf_sr.n_fft, conf_sr.sr),
        "delta_time" : seconds_to_frameidx(conf_shazam.delta_time_secs, conf_sr.hop_length, conf_sr.n_fft, conf_sr.sr),
        "delta_freq" : Hz_to_freqidx(conf_shazam.delta_freq_Hz, conf_sr.num_freq_bins, conf_sr.sr),
    }
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

    kwargs_out = {
        'kwargs_peaks':kwargs_peaks,
        'kwargs_hashPeaks':kwargs_hashPeaks,
        'kwargs_STFT':kwargs_STFT,
    }
    # 'conf_sr':conf_sr,

    return kwargs_out

def get_Shazam_hash(x, kwargs_peaks, kwargs_hashPeaks, kwargs_STFT):
    ## Speech Features
    x_MAG, _,_,_=wav2LPS(x, **kwargs_STFT)

    ## Load Create Constellation Points
    # raw_peaks = get_raw_constellation_pts_v2(x_MAG, **kwargs_peaks)
    # filtered_peaks = filter_peaks(raw_peaks,kwargs_STFT["n_fft"], **kwargs_peaks)
    raw_peaks = get_raw_constellation_pts_v3(x_MAG, **kwargs_peaks)

    ## Construct hash from these peaks
    return hashPeaks(raw_peaks, **kwargs_hashPeaks)

## Constellation Points
def get_raw_constellation_pts_v2(curr_MAG_in, 
    running_percentile_val=70, f_dim1=30,t_dim1=80, f_dim2=10,t_dim2=20,  base=70,
    high_peak_percentile=None,low_peak_percentile=None ):
    """ Selects local peaks in a spectrogram and returns a list of tuples (time, freq, amplitude)
        S is spectrogram matrix
        f_dim1,f_dim2,t_dim1,and t_dim2 are (freq x time) dimensions of the sliding window for first and second passes

        Inputs : 
        --------
            curr_MAG_in : shape=(freq_bins,time_bins)
            
            percentile_val : range from [0,100]
                Defaults to 70.
                Gets a sliding percentile instead of the whole spectrogram.
                Used to calculate threshold, which determines minimum amplitude required to be a peak.
            
            f_dim1,t_dim1 : Defaults to 30,80
                The window of the spectrogram for finding candidate peak

                At a particular index i,j
                    we find the location of the max value of S[i:i+t_dim1,j:j+f_dim1]
                    (i,j+f_dim1)---------------------------(i+t_dim1,j+f_dim1)
                        |                                       |
                        |        S[i:i+t_dim1,j:j+f_dim1]       |
                        |                                       |
                      (i,j)-----------------------------------(i+t_dim1,j)

                argmax of this given window is a candidate constellation point

            f_dim2,t_dim2 : Threshold for local max, Defaults to 30,80
                Typically smaller than f_dim1,t_dim1

                Given a current candidate peak (fp,tp),
                specifies the window ( fp-f_dim2:fp+f_dim2 , tp-t_dim2:tp+t_dim2 )
                where we reject this peak if it is not a local max in this section.

                At a particular index i,j:
                    we eliminate these candidates if they are not the argmax of their neighbours
                    (i-t_dim2,j+f_dim2)---------------------------(i+t_dim2,j+f_dim2)
                        |                                                   |
                        |                          (i,j)                    |
                        |                                                   |
                    (i-t_dim2,j-f_dim2)---------------------------(i+t_dim2,j-f_dim2)


            base : freq bin lower than this index is not considered as peak
                Defaults to 70

        Returns : 
        --------
            peaks : shape=(Number_Of_Peaks,3)
                peaks[:,0] - time bin index (frame)
                peaks[:,1] - freq bin index
                peaks[:,2] - Amplitude value of magnitude S(t,f)

                if no peaks exists, we return None

        Update : 
        --------
            Return None if no constellation points exists
    """
    S = curr_MAG_in.T
    a = len(S) # num of time bins
    b = len(S[1]) # num of frequency bins

    peaks = []
    t_coords = []
    f_coords = []

    "First pass : Determine the time x frequency window to analyze"
    for i in range(0,a,t_dim1):
        t_e = min(i + t_dim1,a)
        # minimum amplitude required to be a peak
        threshold = np.percentile(S[i:t_e,:], running_percentile_val)
        for j in range(base,b,f_dim1):
            f_e = min(j+f_dim1,b)

            window = S[i:t_e,j:f_e]

            "Check if the largest value in the window is greater than the threshold"
            if np.amax(window) >= threshold:
                row, col = np.unravel_index(np.argmax(window), window.shape) # pulls coordinates of max value from window
                t_coords.append(i+row)
                f_coords.append(j+col) 

    "Second pass : Iterates through coordinates selected above to make sure that each of those points is in fact a local peak"    
    for idx in range(0,len(f_coords)):
        fmin = max(f_coords[idx] - f_dim2, base)
        fmax = min(f_coords[idx] + f_dim2, b)
        tmin = max(t_coords[idx] - t_dim2, 0)
        tmax = min(t_coords[idx] + t_dim2, a)
        window = S[tmin:tmax,fmin:fmax] #window centered around current coordinate pair

        "Break when the window is empty"
        if not window.size: continue

        "Eliminates coordinates that are not local peaks by setting their coordinates to -1"
        if S[t_coords[idx],f_coords[idx]] < np.amax(window):
            t_coords[idx] = -1
            f_coords[idx] = -1

    "Removes all -1 coordinate pairs"
    f_coords[:] = (value for value in f_coords if value != -1)
    t_coords[:] = (value for value in t_coords if value != -1)

    "Return None if no peaks after filtering"
    for x in range(0, len(f_coords)):
        peaks.append((t_coords[x], f_coords[x], S[t_coords[x], f_coords[x]]))
    if len(peaks):
        return peaks
    else:
        return None
def filter_peaks(peaks, n_fft,
    high_peak_percentile=75,low_peak_percentile=60,
    running_percentile_val=None, f_dim1=None,t_dim1=None, f_dim2=None,t_dim2=None,  base=None):
    """ Reduce the number of peaks by separating into high and low frequency regions and thresholding.
        By default we remove the 
            high_peak_percentile : bottom 75% of higher freq peaks
            low_peak_percentile  : bottom 60% of higher freq peaks
    """
    if peaks is None : return None
    ## Separate regions ensure better spread of peaks, into respective lists
    high_peaks = []
    low_peaks  = []
    for curr_peak in peaks:
        if(curr_peak[1]>(n_fft/4)): # Split higher freq into a list
            high_peaks.append(curr_peak)
        else:                       # Split lower freq into a list
            low_peaks.append(curr_peak)
    ## Eliminate peaks based on respective thresholds in the low and high frequency regions.  
    if len(high_peaks)  : high_peak_threshold = np.percentile(high_peaks,high_peak_percentile,axis=0)[2]
    if len(low_peaks)   : low_peak_threshold  = np.percentile(low_peaks,low_peak_percentile,axis=0)[2]
    reduced_peaks = []
    for curr_peak in peaks:
        if (curr_peak[1]>(n_fft/4)) :   # Filtering higher freq by {amplitude < high_peak_threshold}
            if(curr_peak[2]>high_peak_threshold):   reduced_peaks.append(curr_peak)
            else:                                   continue
        else:                           # Filtering lower freq by {amplitude < low_peak_threshold}
            if(curr_peak[2]>low_peak_threshold):    reduced_peaks.append(curr_peak)
            else:                                   continue
    " return None if no peaks after filtering "
    if len(reduced_peaks) :
        return reduced_peaks
    else:
        return None
def get_raw_constellation_pts_v3(curr_MAG_in, 
    running_percentile_val=70, f_dim1=30,t_dim1=80, f_dim2=10,t_dim2=20,  base=70,
    high_peak_percentile=None,low_peak_percentile=None):
    from skimage.feature import peak_local_max
    
    coordinates1 = peak_local_max(curr_MAG_in, 
        min_distance=max(f_dim1,t_dim1), 
        threshold_abs=1,
        indices=True,
        )
    peaks=[]
    for tf_val in coordinates1: 
        tidx,fidx = tf_val
        peaks.append( ( fidx, tidx, curr_MAG_in[tidx,fidx] ) )
    return peaks

## Combinatorial Hash
def hashPeaks(AllPeaks,
    num_tgt_pts=100,
    delay_time=214,
    delta_time=5,delta_freq=70):
    # " return None if no peaks at all "
    if AllPeaks is None : return [None, None]
    # " Adding all to hashMatrix "
    hashMatrix = np.zeros((len(AllPeaks)*num_tgt_pts,2)).astype(np.int32)  # Assume size limitation
    index = 0
    numPeaks = len(AllPeaks)
    for idx,anchor_peak in enumerate(AllPeaks):
        adjPts = findTargetZonePts(AllPeaks, anchor_peak,delay_time,delta_time,delta_freq) # global variable AllPeaks
        adjPts.sort(key=itemgetter(2), reverse=True) # sorts in place
        # for curr_pt in adjPts : curr_pt
        adjPts=adjPts[:num_tgt_pts]
        for jdx,curr_adjPts in enumerate(adjPts):
                hashMatrix[index][0] = hash(  ( anchor_peak[1] ,
                                                curr_adjPts[1] ,
                                                curr_adjPts[0]-anchor_peak[0]) 
                                            )
                hashMatrix[index][1] = anchor_peak[0] # Time bin index (Frame)
                index=index+1
    ## Remove the zeros
    hashMatrix = hashMatrix[~np.all(hashMatrix==0,axis=1)]
    # " Dealing with no hashMatrix.shape=(0,2) "
    if hashMatrix.shape[0]:
        ## Store them as a dictionary
        hash_DICT = {}
        for curr_h in hashMatrix:
            element,index = curr_h
            if element not in hash_DICT:
                hash_DICT[element] = {index}
            else: # add on if duplicates exists
                hash_DICT[element].add(index)
                # print('element',hash_DICT[element])
        ## Save as a sorted dictionary
        return [hash_DICT , np.max(hashMatrix[:,-1])]
    else:
        return [None, None]
def findTargetZonePts(AllPeaks, anchor_peak, delay_time,delta_time,delta_freq):
    """
    Find all points within the Target Zone of current anchor point.
    Inputs : 
        anchor_peak : anchor_peak == AllPeaks[index]
        delay_time  : Time delayed before seeking the next
        delta_time  : Time width of Target Zone 
        delta_freq  : Freq width of Target Zone, will be +- half of this from the anchor point
    """
    adjPts = []
    ## Target Zone Boundaries
    # Time range
    low_x = anchor_peak[0]+delay_time
    high_x = low_x+delta_time
    # Freq range
    low_y = anchor_peak[1]-delta_freq//2
    high_y = anchor_peak[1]+delta_freq//2

    for curr_peak in AllPeaks:
        # if    (  Within_Time_Of_Target_Zone  )         and          (  Within_Freq_Of_Target_Zone  )
        if ((curr_peak[0]>low_x and curr_peak[0]<high_x) and (curr_peak[1]>low_y and curr_peak[1]<high_y)):
            adjPts.append(curr_peak)
            
    return adjPts

################################################################
## Matching
################################################################
def count_number_match(rep_hash_in, query_hash_in, bins=100, N=1, verbose=0):
    """ Calculates the number of matches in the 
        representative hash (rep_hash_in) of the current cluster against
        the current hash (query_hash_in).

        Usage:
        ------
            num_matches = count_number_match(rep_hash_in, query_hash_in, bins=100, N=1)

        Input:
        ------
            rep_hash
                hash representing the current cluster.

            query_hash
                hash to compare with current cluster.

            bins
                Split current time matches into histogram,
                with specified number of bins.
                !!! Might need to change to be adaptively to increase recall.

            N
                Only give results for top N bins.
                !!! Maybe return top 3 instead?
                !!! See how it can improve accuracy.

        Returns:
        --------
            num_matches
                Number of hash matches.
    """
    db_timepairs = findTimePairs(rep_hash_in, query_hash_in)
    ## Get statistics
    curr_max=max(   query_hash_in[1], rep_hash_in[1]),
    curr_min = min(db_timepairs)

    curr_offsets = np.array(db_timepairs)-curr_min
    try :
        hist, bin_edges = np.histogram(curr_offsets, bins=100, range=(0,curr_max-curr_min))
    except IndexError:
        hist, bin_edges = np.histogram(curr_offsets, bins=100)
    return np.max(hist)
def make_decision(rep_hash_in, query_hash_in, bins=100, N=1, threshold=10, verbose=0):
    """ Fast decide if representative hash (rep_hash_in) match with current hash (query_hash_in).
    If the number of match cross the threshold, immediately return True to save time.

        Usage:
        ------
            bool = make_decision(rep_hash_in, query_hash_in, bins=100, N=1, threshold=conf_hash.threshold_cluster, verbose=0)

        Input:
        ------
            rep_hash
                hash representing the current cluster.

            query_hash
                hash to compare with current cluster.

            bins
                Split current time matches into histogram, with specified number of bins.
                !!! Might need to change to be adaptively to increase recall.

            N
                Only give results for top N bins.
                !!! Maybe return top 3 instead?
                !!! See how it can improve accuracy.

        Returns:
        --------
            num_matches
                Number of hash matches.
    """
    db_timepairs = findTimePairs(rep_hash_in, query_hash_in)
    return get_candidate_bins(db_timepairs, 
                    curr_max=max(   query_hash_in[1],
                                    rep_hash_in[1]), 
                    bins=bins, N=N, threshold=threshold)
def findTimePairs(hash_database_in,query_hash,   deltaTime=4,deltaFreq=2):
    """     Find the matching pairs between sample audio file and the songs in the database"
        Inputs :
        ---------
            hash_database   :   [db_hash_DICT_sorted[hash] = {set of indices}   , _]
            query_hash      :   [qry_hash_DICT_sorted[hash] = {set of indices}  , _]
        Outputs :
        ---------
            timePairs       :   array of shape (K,3)
                                [K,:] : index K - the number of matched hashes
                                [:,0] : index 0 - the time_idx of the matched song
                                [:,1] : index 1 - the time_idx of the query
                                        This is so we can calculate the offset

                                             t_song, t_query
                                        array([[ 26,  48],
                                               [ 52, 131],
                                               [304, 197]])
    Next step, include the following adaptive threshold : deltaTime=4,deltaFreq=2"
       # |Query_f1 - Database_f1| < deltaFreq
       # |Query_f2 - Database_f2| < deltaFreq
       # |Query_𝛿t - Database_𝛿t| < deltaTime
       # (Database__time_anchor,Query__time_anchor,songID)
    """
    timePairs = []
    db_hash=hash_database_in[0]
    qry_hash=query_hash[0]
    for element in db_hash.keys() & qry_hash.keys():
        timePairs.extend([db_t-qry_t for qry_t in qry_hash[element] for db_t in db_hash[element] ])
    return timePairs
def get_candidate_bins_v2(db_timepairs, curr_max, bins=100, N=1, threshold=10):
    ## Get statistics
    curr_min = min(db_timepairs)
    ## Normalising it to [0,50]
    curr_offsets = np.array(db_timepairs)-curr_min
    ## 
    hist, bin_edges = np.histogram(curr_offsets, bins=bins, range=(0,curr_max-curr_min))
    return True if np.max(hist)>threshold else False
get_candidate_bins=get_candidate_bins_v2

# "To work on this"
def get_candidate_bins_v1(db_timepairs, curr_max, bins=100, N=1, threshold=10):
    if len(db_timepairs) < threshold: return None
    ## Get statistics
    curr_min = min(db_timepairs)
    # curr_max = max(db_timepairs)-curr_min
    curr_scle = (bins-1)/(curr_max-1); 
    ## try first run
    for elem in set(db_timepairs): 
        if db_timepairs.count(elem) > threshold:
            return elem
    ## Normalising it to [0,50]
    curr_offsets = list(map(lambda x : (x - curr_min) * curr_scle, db_timepairs ))
    for elem in set(curr_offsets):
        if curr_offsets.count(elem) > threshold:
            return elem
    ## 
    if False:
        curr_offsets=(db_timepairs - curr_min) * curr_scle
        curr_offsets = curr_offsets.astype(np.int16);               # print(' curr_offsets :',curr_offsets)
        cnt = collections.Counter(curr_offsets);                    # print(' cnt :',cnt)
        candidate_bins_LIST=cnt.most_common()
    if False:
        curr_offsets=(db_timepairs - curr_min) * curr_scle
        curr_offsets = curr_offsets.astype(np.int16);               # print(' curr_offsets :',curr_offsets)
        candidate_bins_LIST=[( mode_1[count_1.argmax()], count_1.max() )]
        bincts = np.bincount(curr_offsets)
        mode_1 = np.argmax(bincts)
        count_1 = bincts[mode_1]
        candidate_bins_LIST=[(mode_1,count_1)]
    return None

################################################################
## Hash table Lookup
################################################################
def addLUT(LUT, db_hash_in, name_in):
    if db_hash_in[0] is None: 
        print(f"\t{name_in} has no hash")
        return LUT
    for key in db_hash_in[0]:
        if not key in LUT: LUT[key]=set()
        LUT[key].add(name_in)
    return LUT
def searchLUT_v1(LUT, qry_hash_in, name_in):
    """
    Return a set of db with matching hash.
    """
    cand_list = set()
    if qry_hash_in[0] is None: 
        print(f"\t{name_in} has no hash")
        return cand_list
    for key in qry_hash_in[0]:
        if not key in LUT: continue
        cand_list.update(LUT[key])
    return cand_list
def searchLUT_v2(LUT, qry_hash_in, name_in):
    """
    Return a Counter of db with matching hash, in descending order
    """
    cand_DICT = collections.Counter()
    if qry_hash_in[0] is None: 
        print(f"\t{name_in} has no hash")
        return cand_DICT
    for key in qry_hash_in[0]:
        if not key in LUT: continue
        cand_DICT.update(LUT[key])
    return cand_DICT.most_common()
searchLUT=searchLUT_v2
################################################################
##        Misc         ##
################################################################
def getLines(AllPeaks, 
    num_tgt_pts, delay_time, delta_time, delta_freq):
    ValidQuads_list=[];
    for idx,anchor_peak in enumerate(AllPeaks):
        adjPts = findTargetZonePts(AllPeaks, anchor_peak,delay_time,delta_time,delta_freq) # global variable AllPeaks
        adjPts.sort(key=itemgetter(2), reverse=True) # sorts in place
        # for curr_pt in adjPts : curr_pt
        adjPts=adjPts[:num_tgt_pts]
        for jdx,curr_adjPts in enumerate(adjPts):
            ValidQuads_list.append((anchor_peak,curr_adjPts))
    return ValidQuads_list

"""
pdb.set_trace()
!import code; code.interact(local=vars())
"""

""" Credits : 
    https://github.com/bmoquist/Shazam
    https://github.com/miishke/PyDataNYC2015
"""
