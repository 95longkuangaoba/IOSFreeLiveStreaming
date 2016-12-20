//
//  RecoderIm.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/1.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "RecoderIm.h"
#import "self-reliance-media.h"

#define h264outputWidth 480
#define h264outputHeight 360


@implementation RecoderIm
{
    sr___muxer___t *muxer;
    sr___media_event_callback___t cb;
    sr___event_listener___t *listener;
    videoCaptureInBaseClass * _videoCaptureBC;
    audioCaptureBaseClass * _audioCaptureBC;
//    AVCaptureSession * captureSession;
    BOOL cameraDeviceIsF;
//    AVCaptureDevice * cameraDeviceB;
//    AVCaptureDevice * cameraDeviceF;
//    
    AVCaptureVideoDataOutput * outputVideoDevice;
      H264HardEncoder * _H264Encoder;
    aacEncoder * _myAacEncoder;
    
    dispatch_queue_t EncoderQueue;
    
//    int _spsHeader ;
//    int _ppsHeader  ;
//    int _spsCount ;
//    int _ppsCount ;
    int frameCount ;
    int frameHeader ;
    
    NSFileHandle * audioFileHandle;
    NSFileHandle * videoFileHandle;

    NSDate * start_time;
    int64_t pts;
    NSTimeInterval dat1;
    
    NSDate * audioStart_time;
    int64_t audioPts;
    NSTimeInterval audioDat1;
    
    uint64_t videoTimeSt;
    uint64_t audioTimeSt;
}

void send_event(sr___media_event_callback___t *cb,sr___media_event___t event)
{
    NSLog(@"----------send_event = %d----------",event.i32);
}

-(instancetype)initWithvideoCapture:(videoCaptureInBaseClass *)videoCBS audioCapture:(audioCaptureInBaseClass *)audioCBS
{
    if (self =[super init]) {
 
        NSLog(@"--------Im a Recoder init------------");
        videoCBS = [[videoCaptureInBaseClass alloc]init];
        
         _videoCaptureBC = videoCBS;
        _audioCaptureBC = audioCBS;
        
        
        [_videoCaptureBC setCLDelegate:self];

//         _spsHeader =0 ;
//         _ppsHeader = 0  ;
//         _spsCount =0 ;
//         _ppsCount  =0;
         frameCount =0;
         frameHeader =0;
        
        H264HardEncoder * H264Encoder = [H264HardEncoder alloc];
        [H264Encoder initWithConfiguration];

        [H264Encoder initEncode:h264outputWidth height:h264outputHeight];
        H264Encoder.delegate = self;
        _H264Encoder = H264Encoder;
        
        aacEncoder * myAacEncoder = [[aacEncoder alloc]init];
        
        _myAacEncoder = myAacEncoder;
        _myAacEncoder.myaacEncoderDelegate = self;
        
        
        
        audioCBS = [[audioCaptureInBaseClass alloc]init];
        
        _audioCaptureBC = audioCBS;
        
        [_audioCaptureBC setAUDelegate:self];
        [audioCBS initAudioCaputre];
        
        EncoderQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
      
        
        NSString *file = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"chenlong.h264"];
        [[NSFileManager defaultManager] removeItemAtPath:file error:nil];
        [[NSFileManager defaultManager] createFileAtPath:file contents:nil attributes:nil];
        
        videoFileHandle = [NSFileHandle fileHandleForWritingAtPath:file];
        NSLog(@"-----videoFileHandle = %@------",videoFileHandle);
        
        

        NSString *audioFile = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"chenlong.aac"];
        [[NSFileManager defaultManager] removeItemAtPath:audioFile error:nil];
        [[NSFileManager defaultManager] createFileAtPath:audioFile contents:nil attributes:nil];
        audioFileHandle = [NSFileHandle fileHandleForWritingAtPath:audioFile];
        
   
        cb.reciever =(__bridge void *) self;
        cb.send_event = send_event;
        sr___event_listener___create(&cb, &listener);
   
        
      const  char *url = "rtmp://101.200.90.199:1935/live/livestream";

        int ret = sr___muxer___create(url, true, true, listener, &muxer);
        if (ret != 0){
            NSLog(@"muxer create false");
        }
        
    }
    return self;
}




- (void)captureOutput:(AVCaptureOutput *)captureOutput CLdidOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{

    dispatch_sync(EncoderQueue, ^{
          NSLog(@"视频编码开始");
        
           [_H264Encoder encode:sampleBuffer];
      
       
        
        static BOOL start = false;

        if (!start)
        {
            start = true;
            start_time = [NSDate date];
            dat1 = [start_time timeIntervalSince1970];

            pts = 0;
             videoTimeSt = 0;
        }else{
            NSDate * current_time = [NSDate date];
   
            NSTimeInterval dat2 = [current_time timeIntervalSince1970] ;
            NSTimeInterval cha = dat2  - dat1  ;
            NSLog(@"cha = %f",cha);
            

            pts  = (int64_t)(cha * 1000000);
            videoTimeSt = pts - videoTimeSt;
            videoTimeSt = pts;
            
        //    NSLog(@"dat1 = %f",dat1);
        //    NSLog(@"dat2 = %f",dat2);

        }
        NSLog(@"pts = %llu",pts);
           NSLog(@" videoTimeSt-> %llu",videoTimeSt / 1000);
    });
 
    
    
}


-(void)captureOutput:(AVCaptureOutput *)captureOutput AUdidOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    
    dispatch_sync(EncoderQueue, ^{
        [_myAacEncoder encodeSampleBuffer:sampleBuffer completionBlock:^(NSData *encodedData, NSError *error) {
            
              [audioFileHandle writeData:encodedData];
          
            
            
            static BOOL start = false;

            if (!start)
            {
                start = true;
                audioStart_time = [NSDate date];
                audioDat1 = [start_time timeIntervalSince1970];
                
                audioPts = 0;
                 audioTimeSt = 0;
            }else{

                NSDate * audioCurrent_time = [NSDate date];
       
                
                NSTimeInterval audioDat2 = [audioCurrent_time timeIntervalSince1970] ;
                NSTimeInterval audiocha = audioDat2  - audioDat1  ;
                NSLog(@"cha = %f",audiocha);
                

                audioPts  = (int64_t)(audiocha * 1000000);
                audioTimeSt = audioPts - audioTimeSt;
                audioTimeSt = audioPts;
                NSLog(@"dat1 = %f",audioDat1);
                NSLog(@"dat2 = %f",dat1);

            }
            NSLog(@"audiopts = %llu",audioPts);

            NSLog(@" audioTimeSt-> %llu",audioTimeSt / 1000);
            
            
            
            NSLog(@"音频编码开始");
        }];
    });
    
    
}




-(void)gotSpsPps:(NSData *)sps pps:(NSData *)pps
{
   
    
    const char bytes[] = "\x00\x00\x00\x01";
    size_t length = (sizeof(bytes)) - 1;
    NSData * ByteHeader = [NSData dataWithBytes:bytes length:length];
    
    [videoFileHandle writeData:ByteHeader];
    [videoFileHandle writeData:sps];
    [videoFileHandle writeData:ByteHeader];
    [videoFileHandle writeData:pps];
    
    NSMutableData * frameData = [[NSMutableData alloc]init];
    [frameData appendData:ByteHeader];
    [frameData appendData:sps];
    [frameData appendData:ByteHeader];
    [frameData appendData:pps];
    
    
    
    sr___video_format___t format;
    format.codec_type = SR___Video_Codec_Type___AVC;
    format.width = h264outputWidth;
    format.height = h264outputHeight;
    format.bit_rate = h264outputWidth * h264outputHeight * 50;
    format.frame_per_second =10;
    format.extend_data_size =  frameData.length;
    memcpy(format.extend_data , [frameData bytes], frameData.length);
    
    sr___media_frame___t *frame = sr___video_format___to___media_frame(&format);
    
    frame->slice.type.media = SR___Media_Type___Video;
    frame->slice.type.payload = SR___Payload_Type___Format;
    frame->slice.timestamp = 0;
    sr___muxer___frame(muxer, frame);
    NSLog(@"frame.header =%p",frame);
    /*
    //发sps
    NSMutableData * h264Data = [[NSMutableData alloc]init];
    [h264Data appendData:ByteHeader];
    self->_spsHeader++;
   
    NSLog(@"spsHeader = %d",_spsHeader);
    [h264Data appendData:sps];
    _spsCount ++;
//_spsCount = _spsCount;
    NSLog(@"spsCount =  %d",_spsCount);
    
    //发pps
    [h264Data resetBytesInRange:NSMakeRange(0, [h264Data length])];
    [h264Data setLength:0];
    [h264Data appendData:ByteHeader];
    _ppsHeader ++;
    //_ppsHeader = ppsHeader;
    NSLog(@"ppsHeader = %d",_ppsHeader);
    [h264Data appendData:pps];
    _ppsCount ++;
   // _ppsCount = _ppsCount;
    NSLog(@"ppsCount = %d",_ppsCount);
    
   */
}
-(void)gotEncodedData:(NSData *)data iskeyFrame:(BOOL)isKeyFrame
{
    sr___media_frame___t *frame = NULL;
    sr___media_frame___create(&frame);
    
    frame->size = 4 + data.length;
    frame->data = srmalloc(frame->size);
    frame->data[0] = 0;
    frame->data[1] = 0;
    frame->data[2] = 0;
    frame->data[3] = 1;
    memcpy(frame->data + 4, [data bytes], data.length);
    
    frameCount ++;
 

    frame->slice.timestamp = pts * 1000 ;
    frame->slice.type.media = SR___Media_Type___Video;
    frame->slice.type.payload = SR___Payload_Type___Frame;
    
    if (isKeyFrame){
        frame->slice.type.extend = SR___Video_Extend_Type___KeyFrame;
    }else{
        frame->slice.type.extend = SR___Video_Extend_Type___None;
    }
    
    sr___muxer___frame(muxer, frame);
    
//    const char bytes[] = "\x00\x00\x00\x01";
//    size_t length = (sizeof(bytes)) -1 ;
//    NSData * ByteHeader =[NSData dataWithBytes:bytes length:length];
//    NSMutableData * h264Data = [[NSMutableData alloc]init];
//    [h264Data appendData:ByteHeader];
//    frameHeader++;
//    self->frameHeader = frameHeader;
//    NSLog(@"frameHeader = %d",self->frameHeader);
//    [h264Data appendData:data];
//    frameCount ++;
//    self->frameCount = frameCount;
//    NSLog(@"frameCount = %d, 关键帧：%@",self->frameCount,isKeyFrame?@"YES":@"NO" );
//    [videoFileHandle writeData:ByteHeader];
//    [videoFileHandle writeData:data];
}


-(void)end
{
    [_H264Encoder endEncoder];
    [_myAacEncoder audioEndEncoder];
    
    
    [audioFileHandle closeFile];
    audioFileHandle = NULL;
    
    _audioCaptureBC.audioCaptureDelegate = nil;
    
    
    
    _videoCaptureBC.myDelegate = nil;
    
    _videoCaptureBC = nil;
    _audioCaptureBC = nil;
    
    
//    captureSession = nil;
//    cameraDeviceB = nil;
//    cameraDeviceF = nil;
//    
    outputVideoDevice = nil;

    
  //EncoderQueue = nil;
    
    
    NSLog(@"结束音视频编码");
    
}


#pragma mark -- 音频编码代理函数
//得到音频头
- (NSData*) adtsDataForPacketLength:(NSUInteger)packetLength {
    int adtsLength = 7;
    char *packet = malloc(sizeof(char) * adtsLength);
    // Variables Recycled by addADTStoPacket
    int profile = 2;  //AAC LC AAC编码格式的第二种， 低复杂度的格式
    //39=MediaCodecInfo.CodecProfileLevel.AACObjectELD;
    int freqIdx = 4;  //44.1KHz
    int chanCfg = 1;  //MPEG-4 Audio Channel Configuration. 1 Channel front-center
    NSUInteger fullLength = adtsLength + packetLength;
    // fill in ADTS data
    packet[0] = (char)0xFF; // 11111111     = syncword
    packet[1] = (char)0xF9; // 1111 1 00 1  = syncword MPEG-2 Layer CRC
    packet[2] = (char)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
    packet[3] = (char)(((chanCfg&3)<<6) + (fullLength>>11));
    packet[4] = (char)((fullLength&0x7FF) >> 3);
    packet[5] = (char)(((fullLength&7)<<5) + 0x1F);
    packet[6] = (char)0xFC;  //1111 1100
    NSData *data = [NSData dataWithBytesNoCopy:packet length:adtsLength freeWhenDone:YES];
    
    
    //
    //    //sr___video_format___t format;
    sr___audio_format___t audioFormat;
    //    //format.codec_type = SR___Video_Codec_Type___AVC;
    audioFormat.codec_type = SR___Audio_Codec_Type___AAC;
    //
    //    //audioFormat.bit_rate =
    audioFormat.channels =1;
    //   // //audioFormat.extend_data
    audioFormat.extend_data_size = data.length;
    
    audioFormat.sample_per_frame = 1024;
    audioFormat.bit_rate = 64000;
    audioFormat.sample_rate = 44100;
    memcpy(audioFormat.extend_data , [data bytes], data.length);
    //
    //sr___media_frame___t *frame = sr___video_format___to___media_frame(&audioFormat);
    sr___media_frame___t * frame = sr___audio_format___to___media_frame(&audioFormat);
    
    // frame->slice.type.media = SR___Media_Type___Video;
    frame->slice.type.media = SR___Media_Type___Audio;
    frame->slice.type.payload = SR___Payload_Type___Format;
    frame->slice.timestamp = 0;
    sr___muxer___frame(muxer, frame);

    NSLog(@"Audioframe.header =%p",frame);
    //    
    //    
    //    
    
    
    
    return data;
}

//得到音频帧
-(NSData *)gotAudioEncodedData
{
   // data = nil;
   
        NSData *rawAAC = [NSData dataWithBytes:_myAacEncoder.outAudioBufferList.mBuffers[0].mData length:_myAacEncoder.outAudioBufferList.mBuffers[0].mDataByteSize];
    
    
    sr___media_frame___t *frame = NULL;
    sr___media_frame___create(&frame);
    
    frame->size = rawAAC.length;
    frame->data = srmalloc(frame->size);
    memcpy(frame->data, [rawAAC bytes], rawAAC.length);
   //frame->slice.timestamp = frameCount * (1000 / 30.f) * 1000;
    frame->slice.timestamp = audioPts * 1000 ;
    NSLog(@"frame.slice.timestamp = %llu",frame->slice.timestamp);
    frame->slice.type.media = SR___Media_Type___Audio;
    frame->slice.type.payload = SR___Payload_Type___Frame;
    
  
    sr___muxer___frame(muxer, frame);
    

    
    
    
    return rawAAC;
}





@end
