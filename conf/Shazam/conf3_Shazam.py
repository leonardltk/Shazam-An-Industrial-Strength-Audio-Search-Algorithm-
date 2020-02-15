from __future__ import print_function
import os

verbose_conf=True
# verbose_conf=False

############ Shazam Constellation Point Details ############
if 1:
    ## get_raw_constellation_pts()
    running_percentile_val=50
    """ percentile_val : range from [0,100]
        Defaults to 70.
        Gets a sliding percentile instead of the whole spectrogram.
        Used to calculate threshold, which determines minimum amplitude required to be a peak.
    """

    f_dim1,t_dim1=10,10
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

    f_dim2,t_dim2=5,5
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

    base=10
    # base : freq bin lower than this index is not considered as peak


    ## filter_peaks()
    high_peak_percentile=65
    low_peak_percentile=65

    if verbose_conf:
        print('\n    ############ Shazam Constellation Point Details ############')
        print('    running_percentile_val   :',running_percentile_val)
        print('    f_dim1                   :',f_dim1)
        print('    t_dim1                   :',t_dim1)
        print('    f_dim2                   :',f_dim2)
        print('    t_dim2                   :',t_dim2)
        print('    base                     :',base)

        print('    high_peak_percentile     :',high_peak_percentile)
        print('    low_peak_percentile      :',low_peak_percentile)


############ Shazam Hash Generation Details ############
if 1:
    num_tgt_pts=3
    # Get the top num_tgt_pts points in the target zone as hash

    delay_time_secs=1
    # delay_time  : Time delayed before seeking the next
    delta_time_secs=5
    delta_freq_Hz=1500
    # delta_time  : Time width of Target Zone 
    # delta_freq  : Freq width of Target Zone, will be +- half of this from the anchor point
    if verbose_conf:
        print('\n    ############ Shazam Hash Generation Details ############')
        print('    num_tgt_pts      :',num_tgt_pts)
        print('    delay_time_secs  :',delay_time_secs)
        print('    delta_time_secs  :',delta_time_secs)
        print('    delta_freq_Hz    :',delta_freq_Hz)

############ Shazam Hash Clustering Details ############
if 1:
    # If threshold_cluster is more than this amount, 
    #   we classify them as within the same cluster
    threshold_cluster=10 # Number of hash match to be considered a match
    cluster_verbose=False
    # cluster_verbose=True

    if verbose_conf:
        print('\n    ############ Shazam Hash Clustering Details ############')
        print('    threshold_cluster    :',threshold_cluster)
        print('    cluster_verbose      :',cluster_verbose)
