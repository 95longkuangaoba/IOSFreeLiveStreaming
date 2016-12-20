//
//  myAACDecoder.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/18.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "myAACDecoder.h"
#import "neaacdec.h"
@import AudioToolbox;

#define  AUDIO_BUFFER_SIZE  3 //列缓冲个数
#define AUDIO_MIN_SIZE_PER_FRAME 2048
@implementation myAACDecoder
{
    //short * sampleBuffer;
    NeAACDecHandle * aacDecoder;
    NeAACDecFrameInfo *hInfo;
    int decoderCount;
   
}

-(instancetype)initAACDecoderbuffer:(unsigned char *)buffer buffer_size:(unsigned long)buffer_size samplerate:(unsigned long)samplerate channels:(unsigned char *)channels
{
    if (self = [super init]) {
        
        aacDecoder = NeAACDecOpen();
        hInfo = malloc(sizeof(hInfo));
        
        //samplebuffer_size =2* (hInfo->channels * hInfo->samples);
        long status = NeAACDecInit(aacDecoder, buffer, buffer_size, &samplerate, channels);
        if (status == -1) {
            NSLog(@"AACDecoder init faulse");
        }
    }
        decoderCount = 0;
    
    NSLog(@"open aacDecoder sucesses");
    return self;
}



-(NeAACDecFrameInfo *)getAACFrameInfosamples:(unsigned long)samples channels:(unsigned long)channels samplerate:(unsigned long)samplerate bitrate:(int32_t)bitrate sampleperframe:(int32_t)sample_per_frame
{
    
    hInfo->samples = samples;
    hInfo->channels = channels;
    hInfo->samplerate = samplerate;
    hInfo->header_type = RAW;
 
    
    
    return hInfo;
}


-(void *)aacDecoderDatahInfo:(NeAACDecFrameInfo *)hInfo buffer:(unsigned char *)buffer buffer_size:(unsigned long)buffer_size
{
//    void* NEAACDECAPI NeAACDecDecode(NeAACDecHandle hDecoder,
//                                     NeAACDecFrameInfo *hInfo,
//                                     unsigned char *buffer,
//                                     unsigned long buffer_size);
    short * decodedbuffer = nil;
    if ((decodedbuffer =NeAACDecDecode(aacDecoder, hInfo, buffer, buffer_size)) == NULL )
    {
        NSLog(@"decode AAC faulse");
        
    }
    
    decoderCount++;
    NSLog(@"解码音频帧个数 %d",decoderCount);
    return decodedbuffer;
}

-(void)closeAACDeoder
{
//    
//    if (audioQueue != nil) {
//        AudioQueueStop(audioQueue, true);
//        audioQueue = nil;
//    }
    NeAACDecClose(aacDecoder);
    free(hInfo);
   // audioDesc = nil;
   // AudioQueueFreeBuffer(audioQueue, inAudioBuffers);
    NSLog(@"音频解码器关闭");
}


@end
