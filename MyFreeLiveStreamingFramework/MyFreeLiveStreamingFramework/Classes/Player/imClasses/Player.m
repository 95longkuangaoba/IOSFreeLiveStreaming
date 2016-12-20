//
//  Player.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/6.
//  Copyright © 2016年 陈龙. All rights reserved.
//


/*
 int sr___demux___create(const char *url,
 sr___demux_callback___t *callback,
 sr___event_listener___t *listener,
 sr___demux___t **pp_demux)
 
 */



#import "Player.h"
//#import "myOpenGLView.h"
#import "AAPLEAGLLayer.h"
#import <UIKit/UIKit.h>
#import "self-reliance-media.h"
#import "myAACDecoder.h"
@import AVFoundation;
@import VideoToolbox;


#define  AUDIO_BUFFER_SIZE  3 //列缓冲个数
#define AUDIO_MIN_SIZE_PER_FRAME 4096

#define h264outputWidth 480
#define h264outputHeight 360

//typedef struct myDemux
//{
//     sr___demux___t *demux;
//    sr___media_event_callback___t cb;
//    sr___event_listener___t *listener;
//   
//    sr___demux_callback___t demuxcb;
//    void * demuxAudio;
//    void * demuxVideo;
//
//    
//}myDemux;





@implementation Player



{
    
    sr___demux___t *demux;
    sr___media_event_callback___t listenercb;
    sr___event_listener___t *listener;
    sr___demux_callback___t demuxcb;
    
    

    //CADisplayLink * cDisplayLink;
    //aacDecoder * aacDeco;
   // myOpenGLView * openGLView;
    AAPLEAGLLayer * playLayer;
    
    H264HardDecoder * h264Decoder;
    myAACDecoder * aacDecoder;
    
    dispatch_queue_t mDecodeQueue;

    
    uint8_t * _sps;
    NSInteger _spsSize;
    uint8_t * _pps;
    NSInteger _ppsSize;
    
    NeAACDecFrameInfo * info ;
    //char *  sampleBuffer;
    //unsigned long sampleBuffer_size;
    
    AudioQueueRef  audioQueue;
    AudioStreamBasicDescription  audioDesc;
    AudioQueueBufferRef  inAudioBuffers[AUDIO_BUFFER_SIZE]; //音频缓存
    AudioQueueBuffer outAudioBuffer;
    short * samplebuffer ;
    unsigned long samplebuffer_size;

    char * zhBuffer;
    long audioDataCurrent;
    long audioDataLength;
    
}

-(int)getAACDecoderandFrame:(sr___media_frame___t *)frame
{
    NSLog(@"得到音频头======audio");
    if (frame->slice.type.media == SR___Media_Type___Audio) {
        if (frame->slice.type.payload == SR___Payload_Type___Format) {
            NSLog(@"得到音频头");
            sr___audio_format___t * format = sr___media_frame___to___audio_format(frame);
            
           // [aacDecoder initAACDecoderbuffer:frame->data buffer_size:frame->size samplerate:format->sample_rate channels:format->channels];
            [aacDecoder initAACDecoderbuffer:frame->data buffer_size:frame->size samplerate:format->sample_rate channels:&(format->channels)];
//            int32_t codec_type;
//            int32_t bit_rate;
//            int32_t channels;
//            int32_t sample_rate;
//            int32_t sample_size;
//            int32_t sample_type;
//            int32_t sample_per_frame;
//            int32_t reserved;
//            uint8_t extend_data_size;
//            uint8_t extend_data[255];
            info =   [aacDecoder getAACFrameInfosamples:1024 channels:format->channels samplerate:format->sample_rate bitrate:format->bit_rate sampleperframe:1024];
            
            NSLog(@"ssmple per frame %d", format->sample_per_frame);
            
            NSLog(@"sample rate %d", format->sample_rate);
            NSLog(@"sample size %d", format->sample_size);
            
            audioDesc.mSampleRate = (Float64)format->sample_rate;
            NSLog(@"mSampleRate %lf", audioDesc.mSampleRate);
            audioDesc.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger |kAudioFormatFlagIsPacked;
            audioDesc.mBitsPerChannel = 16;
            audioDesc.mFormatID = kAudioFormatLinearPCM;
            audioDesc.mChannelsPerFrame =(UInt32) format->channels;
            NSLog(@"mChannelsPerFrame %u", audioDesc.mChannelsPerFrame);
            audioDesc.mFramesPerPacket = 1;
            audioDesc.mBytesPerFrame = (audioDesc.mBitsPerChannel / 8) * 2;
            audioDesc.mBytesPerPacket = audioDesc.mBytesPerFrame;
            
            samplebuffer_size = (format->channels) * (1024) * 2;

            //[self audioQueueConfig];
            
            srfree(format);
           
        }
        else
        {
            NSLog(@"得到音频数据");
       samplebuffer = [aacDecoder aacDecoderDatahInfo:info buffer:frame->data buffer_size:frame->size];
            if (samplebuffer != NULL) {
                
                NSThread * audioPlayThread = [[NSThread alloc]initWithBlock:^{
                    
                    //[self play:samplebuffer length:info->samples*info->channels];
                   // [self delegateFillPCMBuffer:self];
                    [self delegateFillcallback:PCMBuffer daili:self];
                    
                    NSLog(@"sample_size");
                }];
                [audioPlayThread start];
               
            }
            else
            {
                NSLog(@"音频解码失败");
            }
      NSLog(@"解码了音频数据");
            
        }
    }

    return YES;
}


static int demuxAudio(sr___demux_callback___t *callback, sr___media_frame___t * frame)
{
    
    Player * player = (__bridge Player *)callback->user;
 
    [player getAACDecoderandFrame:frame];
    
//    sr___media_frame___release(&frame);
    
    return 0;
    
}

-(int)getDecoderandFrame:(sr___media_frame___t * )frame
{
    
    if (frame->slice.type.media == SR___Media_Type___Video) {
        if (frame->slice.type.payload == SR___Payload_Type___Format)
        {
            NSLog(@"得到视频头");
            sr___video_format___t *fmt = sr___media_frame___to___video_format(frame);
            
            int i = 4;
            int sps_size = 0;
            int pps_size = 0;
            uint8_t *p_sps = NULL;
            uint8_t *p_pps = NULL;
            while(fmt->extend_data[i] != 0
                  || fmt->extend_data[i + 1] != 0
                  || fmt->extend_data[i + 2] != 0
                  || fmt->extend_data[i + 3] != 1){
                ++i;
            }
            
            p_sps = fmt->extend_data;
            sps_size = i;
            p_pps = &(fmt->extend_data[sps_size]);
            pps_size = fmt->extend_data_size - sps_size;
            
            
            //            log_debug("sps %x %x %x %x\n", p_sps[0], p_sps[1], p_sps[2], p_sps[3]);
            //            log_debug("pps %x %x %x %x\n", p_pps[0], p_pps[1], p_pps[2], p_pps[3]);
            //
            
            
            [h264Decoder decodeH264HeaderSPS:p_sps andwithSpsSize:sps_size andwithpps:p_pps withppsSize:pps_size];
            NSLog(@"解码视频头了");
            
            srfree(fmt);
        }
        else
        {
            NSLog(@"得到视频数据");
            
            //    frame->data;
            //    frame->size;
            
            NSLog(@"frame[0] =%x  frame[1] =%x frame[2] =%x frame[3] =%x",frame->data[0],frame->data[1],frame->data[2],frame->data[3]);
            
            // uint32_t b = (uint32_t)frame->size;
            NSLog(@"frame->size = %zu",frame->size);
//            uint8_t videoData[frame->size - 4];
//            memcpy(videoData, frame->data+4 , frame->size -4 );
            
            
//            CVPixelBufferRef pixBuffer =NULL;
            
            uint32_t size = (uint32_t)frame->size - 4;
            
            frame->data[0] = size >> 24;
            frame->data[1] = (size >> 16) & 0xFF;
            frame->data[2] = (size >> 8) & 0xFF;
            frame->data[3] = size & 0xFF;
            
           [h264Decoder decode:frame->data withSize:(uint32_t)frame->size];
            
//            playLayer.pixelBuffer = pixBuffer;
            
            //    [h264D decode:frame->data withSize:(uint32_t)frame->size];
            
            //[h264D getpixelBuffer:pixB];
            
            // [h264D decode:(uint8_t *)frame withSize:frameSize];
            // [h264D decode:(uint8_t *)frame withSize:frameSize];
            
            
        }
    }
  
    
    return 0;
}


 static int demuxVideo(sr___demux_callback___t *callback,sr___media_frame___t * frame)
{
    
    Player * player = (__bridge Player *)callback->user;
    
    [player getDecoderandFrame:frame];
    NSLog(@"得到视频头======video");
    
    
   // H264HardDecoder * h264D = [[H264HardDecoder alloc]init];
  
    //[h264D initH264Decoder];
    
//        sr___media_frame___release(&frame);
    
    return 0;

}

static void player_send_event(sr___media_event_callback___t *listenercb,sr___media_event___t event)
{
    //NSLog(@"----------send_event = %d----------",event.i32);
    NSLog(@"发送事件信息");
}


-(instancetype)init
{
    if (self = [super init])
    {
       
        
        h264Decoder = [[H264HardDecoder alloc]init];
        h264Decoder.videoDecoderDelegate = self;
 
        aacDecoder = [[myAACDecoder alloc]init];
        
        self.delegate = self;
        info  = malloc(sizeof(info));

        
        playLayer = [[AAPLEAGLLayer alloc]initWithFrame:CGRectMake(180, 120, 160, 300)];
        NSLog(@"playLayer' height = %f    width = %f",playLayer.frame.size.height,playLayer.frame.size.width);
        playLayer.backgroundColor = [UIColor blackColor].CGColor;
        
        
        listenercb.reciever = (__bridge void *)self;
        listenercb.send_event = player_send_event;
        
        sr___event_listener___create(&listenercb, &listener);
        
        const  char *url = "rtmp://101.200.90.199:1935/live/livestream";
        
        demuxcb.demux_audio = demuxAudio;
        demuxcb.demux_video = demuxVideo;
        demuxcb.user = (__bridge void *)self;
        
        int ret = sr___demux___create(url, &demuxcb, listener, &(demux));
        if (ret != 0)
        {
            NSLog(@"demux create false");
        }
        
   
    }
    return self;
}

#pragma mark -- H264解码回调
-(void)displayDecodedFrame:(CVImageBufferRef)imageBuffer
{
    NSLog(@"11111111");
    if (imageBuffer)
    {
        
        NSLog(@"00000000");
        
        playLayer.pixelBuffer = imageBuffer;
      
        CVPixelBufferRelease(imageBuffer);
    }
}


-(void)audioPlay
{
   

}


-(void)displayVideo:(CALayer *)layer
{

    [layer addSublayer: playLayer];

}

-(void)stopDisplay
{
   // [openGLView removeFromSuperview];
    [playLayer removeFromSuperlayer];
    
}



#pragma mark -- AUDIOQUEUE


static void BufferCallback(void * inUserData,AudioQueueRef inAQ,AudioQueueBufferRef buffer)
{
    NSLog(@"处理了音频数据大小为：%u",(unsigned int)buffer->mAudioDataByteSize);
    Player * myplayer = (__bridge Player *)inUserData;
    [myplayer.delegate fillPCMBuffer:myplayer->samplebuffer bufferSize:myplayer->samplebuffer_size];
    //[myplayer.de]
    
}


/*
-(void)fillBuffer:(AudioQueueRef)audioQueue queueBuffer:(AudioQueueBufferRef)buffer
{
    if (audioDataCurrent + AUDIO_MIN_SIZE_PER_FRAME <audioDataLength) {
        if (audioDataCurrent + AUDIO_MIN_SIZE_PER_FRAME  >4096) {
            memcpy(buffer->mAudioData, zhBuffer+(audioDataCurrent % AUDIO_MIN_SIZE_PER_FRAME), (AUDIO_MIN_SIZE_PER_FRAME - audioDataCurrent));
            audioDataCurrent = 0;
            buffer->mAudioDataByteSize = (AUDIO_MIN_SIZE_PER_FRAME - audioDataCurrent);
        }
        memcpy(buffer->mAudioData, zhBuffer + (audioDataCurrent % 5000), AUDIO_MIN_SIZE_PER_FRAME);
        AudioQueueEnqueueBuffer(audioQueue, buffer, 0, NULL);
    }
    
    
}
 
 */


-(int)delegateFillcallback:PCMBuffer daili:(id<PlayerDelegate>)callback
{
    callback = _delegate;
    
    OSStatus status = AudioQueueNewOutput(&audioDesc, BufferCallback, (__bridge void *)self, NULL, NULL, 0, &audioQueue);
    if (status != noErr) {
        return -1;
    }
    for (int i = 0; i<AUDIO_BUFFER_SIZE; i++) {
        status = AudioQueueAllocateBuffer(audioQueue, AUDIO_MIN_SIZE_PER_FRAME, &inAudioBuffers[i]);
//        if (status == noErr) {
//            status = AudioQueueEnqueueBuffer(audioQueue, inAudioBuffers[i], 0, NULL);
//        }
    }
    return 0;
}



//-(void)createAudioQueue
//{
//    [self audioCleanup];
//    
//    AudioQueueNewOutput(&(audioDesc), BufferCallback, (__bridge void *)self, nil, nil, 0, &audioQueue);
//    if (audioQueue) {
//        //添加buffer区
//        for (int i = 0; i<AUDIO_MIN_SIZE_PER_FRAME; i++) {
//            int result = AudioQueueAllocateBuffer(audioQueue, AUDIO_MIN_SIZE_PER_FRAME, &inAudioBuffers[i]);
//            NSLog(@"audioQueueAllocateBuffer  i = %d result = %d",i,result);
//        }
//    }
//    
//}

-(void)audioCleanup
{
    if (audioQueue) {
        [self stopAudio];
        for (int i =0; i< AUDIO_BUFFER_SIZE; i++) {
            AudioQueueFreeBuffer(audioQueue,inAudioBuffers[i]);
            inAudioBuffers[i] = nil;
        }
        audioQueue = nil;
    }
}

-(void)stopAudio
{
    AudioQueueFlush(audioQueue);
    AudioQueueReset(audioQueue);
    AudioQueueStop(audioQueue, true);
}



-(int)PCMBuffer:(uint8_t *)buffer bufferSize:(uint32_t)size
{
    AudioQueueBufferRef  myaudioBuffer =NULL;
    myaudioBuffer->mAudioDataByteSize = size;
    //(Byte *)myaudioBuffer->mAudioData = (Byte *)buffer;
    Byte * audioData = (Byte *)myaudioBuffer->mAudioData;
    for (int i = 0; i<size; i++) {
        audioData[i] = buffer[i];
    }
    AudioQueueEnqueueBuffer(audioQueue, myaudioBuffer, 0, NULL);
    
    return 0;
}
















-(void)dealloc
{
    free(info);
    if (audioQueue != nil) {
        AudioQueueStop(audioQueue, true);
    }
    audioQueue = nil;
   
    //sysLock = nil;
}





@end

