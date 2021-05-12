from __future__ import print_function
if True: ## imports / admin
    import os,sys,pdb # pdb.set_trace() # !import code; code.interact(local=vars())
    from _helper_basics_ import *

## Parameters for hashing and matching
def get_hash_kwargs(conf_sr,conf_shazam,):
    """ Create a parameters for the hashing functions.
        Usage:
        ------

        Inputs : 
        ---------
            conf_sr         : config for audio parameters
            conf_shazam     : config for hashing parameters

        Returns : 
        ---------
            kwargs_in : dict
                kwargs_peaks        : for getting contellation point, and filtering them
                kwargs_hashPeaks    : For hashing the points
    """
    kwargs_peaks={
        'f_dim1':conf_shazam.f_dim1,
        't_dim1':conf_shazam.t_dim1,
        'base':conf_shazam.base,
        'threshold_abs':conf_shazam.threshold_abs,
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
    return kwargs_out

################################################################
## Hash
################################################################
## Constellation Points
def get_raw_constellation_pts(curr_MAG_in, f_dim1=10, t_dim1=10, threshold_abs=1, base=-1,):
    """
    The window of the spectrogram for finding candidate peak
        At a particular index i,j:
            we find the location of the max value of S[i:i+t_dim1,j:j+f_dim1]
            (i,j+f_dim1)---------------------------(i+t_dim1,j+f_dim1)
                |                                       |
                |        S[i:i+t_dim1,j:j+f_dim1]       |
                |                                       |
              (i,j)-----------------------------------(i+t_dim1,j)

        argmax of this given window is a candidate constellation point
    Frequency cut-offs.
        freq bin lower than this index is not considered as peak
        base=20 means frequency lower than 430Hz are cut-off
        Hz   = freqidx_to_Hz(base, conf_sr.num_freq_bins, conf_sr.sr)
        base = Hz_to_freqidx(Hz,   conf_sr.num_freq_bins, conf_sr.sr)
    """
    coordinates1 = peak_local_max(curr_MAG_in, 
        min_distance=max(f_dim1,t_dim1), 
        threshold_abs=threshold_abs,
        indices=True,
        )
    peaks=[]
    for fidx, tidx in coordinates1:
        if (fidx > base): # cut off on lower frequencies, if we deem them not important. Usually disruption noise or distortion is in these region.
            peaks.append( ( tidx, fidx, curr_MAG_in[fidx,tidx] ) )
    return peaks

## Combinatorial Hash
def hashPeaks(AllPeaks,
    num_tgt_pts=100,
    delay_time=214,
    delta_time=5,delta_freq=70):
    # " return None if no peaks at all "
    if AllPeaks is None : return [None, None]
    # " Adding all to hashMatrix "
    index = 0
    hash_DICT = {}
    for idx,anchor_peak in enumerate(AllPeaks):
        adjPts = findTargetZonePts(AllPeaks, anchor_peak,delay_time,delta_time,delta_freq) # global variable AllPeaks
        for jdx, curr_adjPts in enumerate(
            heapq.nlargest(
                num_tgt_pts,
                adjPts,
                key=itemgetter(2))
            ):
            # key = hash(  (anchor_peak[1], curr_adjPts[1], curr_adjPts[0]-anchor_peak[0]) )
            key = (anchor_peak[1], curr_adjPts[1], curr_adjPts[0]-anchor_peak[0])
            if not key in hash_DICT : hash_DICT[key] = set()
            hash_DICT[key].add(anchor_peak[0]) # Time bin index (Frame)
    return hash_DICT
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
    # Searching for adjacent points
    for curr_peak in AllPeaks:
        # if    (  Within_Time_Of_Target_Zone  )         and          (  Within_Freq_Of_Target_Zone  )
        if ((curr_peak[0]>low_x and curr_peak[0]<high_x) and (curr_peak[1]>low_y and curr_peak[1]<high_y)):
            adjPts.append(curr_peak)
    return adjPts

def get_Shazam_hash(x, kwargs_peaks, kwargs_hashPeaks, kwargs_STFT):
    """
        Usage :
            kwargs=Shazam.get_hash_kwargs(conf_sr,conf_hash,)
            x_wav,sr_out=read_audio(wav_path, mode='audiofile', sr=conf_sr.sr, mean_norm=False)
            qry_shash=Shazam.get_Shazam_hash(x_wav, **kwargs)
    """
    ## Speech Features
    x_MAG, _,_,_=wav2LPS(x, **kwargs_STFT)
    ## Load Create Constellation Points
    raw_peaks = get_raw_constellation_pts(x_MAG, **kwargs_peaks)
    ## Construct hash from these peaks
    return [hashPeaks(raw_peaks, **kwargs_hashPeaks), x_MAG.shape[1]]

################################################################
## Matching
################################################################
def findTimePairs(ref_hash, query_hash):
    """     Find the matching pairs between sample audio file and the songs in the database"

        Inputs :
        ---------
            ref_hash        :   [ref_hash[hash] = {set of indices} , number of frames]
            query_hash      :   [query_hash[hash] = {set of indices}, number of frames]

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
    """
    timePairs = []
    db_hash=ref_hash[0]
    qry_hash=query_hash[0]
    for element in db_hash.keys() & qry_hash.keys():
        timePairs.extend([db_t-qry_t for qry_t in qry_hash[element] for db_t in db_hash[element] ])
    return timePairs
def count_number_match(ref_hash, query_hash, bins=100, N=1):
    """ Calculates the number of matches in the 
        representative hash (rep_hash_in) of the current cluster against
        the current hash (query_hash_in).

        Usage:
        ------
            num_matches, hist, bin_edges = count_number_match(ref_hash[0], query_hash[0], bins=ref_hash[1]//100, N=1)

        Input:
        ------
            ref_hash
                Reference hash from the database.

            query_hash
                hash to query with the reference.

            bins
                Traditionally, we use 100 bins,
                but with varying reference length of database audio,
                we should normalise to (total_number_of_reference_frames/100).

            N
                Return top N results of the maximum of the histogram, defaults to 1.

        Returns:
        --------
            num_matches
                Number of hash matches.
    """
    db_timepairs = findTimePairs(ref_hash, query_hash)
    if len(db_timepairs)==0 : return 0, [], []
    hist, bin_edges = np.histogram(db_timepairs, bins=bins)
    return np.max(hist), hist, bin_edges
def get_ref_timing(hist, bin_edges, conf_sr):
    """ Calculates the approximate timing this song appears in the reference song
        Usage:
        ------
            num_matches, hist, bin_edges = count_number_match(ref_hash[0], query_hash[0], bins=ref_hash[1]//100, N=1)
            time_frame, start_sec = get_ref_timing(hist, bin_edges, conf_sr)

        Input:
        ------
            hist, bin_edges
                histogram and the bin_edges extracted by count_number_match().

            conf_sr
                config file.

        Returns:
        --------
            time_frame
                approx time_frame that this query appears in the reference.

            start_sec
                approx second that this query appears in the reference.
    """
    if len(hist):
        idx_argmax = np.argmax(hist)
        time_frame = np.mean(bin_edges[idx_argmax:idx_argmax+2])
        start_sec = frameidx_to_seconds(time_frame, conf_sr.hop_length, conf_sr.n_fft, conf_sr.sr)
    else:
        time_frame = None
        start_sec = None
    return time_frame, start_sec
def make_decision(ref_hash, query_hash, bins=100, threshold=10):
    """ Fast decide if representative hash (ref_hash) match with current hash (query_hash).
    If the number of match cross the threshold, immediately return True to save time.

        Usage:
        ------
            bool = make_decision(ref_hash, query_hash, bins=ref_hash[1]//100, threshold=conf_hash.th)

        Input:
        ------
            ref_hash
                Reference hash from the database.

            query_hash
                hash to query with the reference.

            bins
                Traditionally, we use 100 bins,
                but with varying reference length of database audio,
                we should normalise to (total_number_of_reference_frames/100).

            threshold
                If exceeds this threshold,
                we are positive that they are the same,
                hence we return True.

        Returns:
        --------
            boolean
                True if they match, False if they do not match.
    """
    # db_timepairs = findTimePairs(ref_hash, query_hash)
    # return get_candidate_bins(db_timepairs,
    #                 curr_max=max(   query_hash[1],
    #                                 ref_hash[1]),
    #                 bins=bins, N=N, threshold=threshold)
    num_matches, _, _ = count_number_match(ref_hash, query_hash, bins=bins, N=1)
    return True if num_matches>threshold else False
if False : # Deprecated methods
    def get_candidate_bins(db_timepairs, curr_max, bins=100, N=1, threshold=10):
        ## Get statistics
        curr_min = min(db_timepairs)
        ## Normalising it to [0,50]
        curr_offsets = np.array(db_timepairs)-curr_min
        ##
        hist, bin_edges = np.histogram(curr_offsets, bins=bins, range=(0,curr_max-curr_min))
        return True if np.max(hist)>threshold else False
    # "To work on this"
    def get_candidate_bins_debug(db_timepairs, curr_max, bins=100, N=1, threshold=10):
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
## Hash Look-Up Table (LUT)
################################################################
def addLUT(LUT, hash_in, name_in):
    if hash_in[0] is None:
        print(f"\t{name_in} has no hash")
        return LUT
    for key in hash_in[0]:
        if not key in LUT: LUT[key]=set()
        LUT[key].add(name_in)
    return LUT
def removeLUT(LUT, hash_in, name_in):

    if hash_in[0] is None:
        print(f"\t{name_in} has no hash")
        return LUT

    for key in hash_in[0]:
        if key in LUT: LUT[key].remove(name_in)
        if len(LUT[key]) == 0 :
             LUT.pop(key)

    return LUT
def searchLUT(LUT, hash_in, name_in):
    """
    Return a Counter of db with matching hash, in descending order
    """
    # 
    if False : # Deprecated methods
        cand_list = set()
        if hash_in[0] is None: 
            print(f"\t{name_in} has no hash")
            return cand_list
        for key in hash_in[0]:
            if not key in LUT: continue
            cand_list.update(LUT[key])
        return cand_list
    # 
    cand_DICT = collections.Counter()
    if hash_in[0] is None: 
        print(f"\t{name_in} has no hash")
        return cand_DICT
    for key in hash_in[0]:
        if not key in LUT: continue
        cand_DICT.update(LUT[key])
    return cand_DICT.most_common()

################################################################
## Misc
################################################################
if False :# Deprecated methods
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
    def getLines(peaks_in, num_tgt_pts, delay_time, delta_time, delta_freq):
        ValidMaps=[];
        hashmap = {}
        for idx, anchor_peak in enumerate(peaks_in):
            adjPts = findTargetZonePts(peaks_in, anchor_peak, delay_time, delta_time, delta_freq)
            for jdx, curr_adjPts in enumerate(heapq.nlargest(num_tgt_pts, adjPts, key=itemgetter(2))):
                val = (anchor_peak, curr_adjPts)
                ValidMaps.append(val)
                # key=hash(  ( anchor_peak[1], curr_adjPts[1], curr_adjPts[0]-anchor_peak[0]) )
                key=( anchor_peak[1], curr_adjPts[1], curr_adjPts[0]-anchor_peak[0])
                if not key in hashmap:hashmap[key]=set()
                hashmap[key].add(val)
        return ValidMaps, hashmap
def getLines(peaks_in, num_tgt_pts, delay_time, delta_time, delta_freq):
    hashmap = {}
    for idx, anchor_peak in enumerate(peaks_in):
        adjPts = findTargetZonePts(peaks_in, anchor_peak, delay_time, delta_time, delta_freq)
        for jdx, curr_adjPts in enumerate(heapq.nlargest(num_tgt_pts, adjPts, key=itemgetter(2))):
            val = (anchor_peak, curr_adjPts)
            # ValidMaps.append(val)
            # key=hash(  ( anchor_peak[1], curr_adjPts[1], curr_adjPts[0]-anchor_peak[0]) )
            key=( anchor_peak[1], curr_adjPts[1], curr_adjPts[0]-anchor_peak[0])
            if not key in hashmap:hashmap[key]=set()
            hashmap[key].add(val)
    return hashmap

"""
pdb.set_trace()
!import code; code.interact(local=vars())
"""

""" Credits : 
    https://github.com/bmoquist/Shazam
    https://github.com/miishke/PyDataNYC2015
"""
