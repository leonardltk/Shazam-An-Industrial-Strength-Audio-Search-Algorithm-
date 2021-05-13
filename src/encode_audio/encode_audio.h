#ifndef HELLOMP3ENC_H_
#define HELLOMP3ENC_H_

#define RET_MP3_OK 0
#define RET_MP3_OUT_BUFFER_FULL -1
#define RET_MP3_ENCODE_ERROR -2
#define RET_MP3_GENERIC_ERROR -3

/*
 * inSampleRate: inData sampleRate
 * inChannel   : inData channel
 * bps         : bit rate(default set 48000)
 */
int HelloMp3Encode(int inSampleRate, int inChannel, int bps, unsigned char *inData, int inLength,
                   unsigned char *outData, int outBufferSize, int *errorCode);

#endif /* HELLOMP3ENC_H_ */
