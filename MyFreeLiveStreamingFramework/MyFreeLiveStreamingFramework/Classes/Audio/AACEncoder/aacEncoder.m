//
//  aacEncoder.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/3.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "aacEncoder.h"
#import "self-reliance-media.h"




@implementation aacEncoder
{
    AudioConverterRef audioConverter;//音频转换器， 编码器
    uint8_t * aacBuffer;

    NSUInteger aacBufferSize;
    char * pcmBuffer;

    size_t pcmBufferSize;
    dispatch_queue_t encoderQueue;
    dispatch_queue_t callbackQueue;

    int audioFrameCount;
 
    
    sr___muxer___t *amuxer;
    sr___media_event_callback___t acb;
    sr___event_listener___t *alistener;
    
    
    AudioStreamBasicDescription myoutAudioStreamBasicDescription;

}


void mysend_event(sr___media_event_callback___t *acb,sr___media_event___t event)
{
    NSLog(@"----------send_event = %d----------",event.i32);
}




-(id)init
{
    if (self = [super init]) {
        aacBuffer = nil;
        encoderQueue = dispatch_queue_create("AAC Encoder Queue", DISPATCH_QUEUE_SERIAL);
        callbackQueue = dispatch_queue_create("AAC Encoder Callback Queue", DISPATCH_QUEUE_SERIAL);
        audioConverter = NULL;
        pcmBufferSize = 0;
        pcmBuffer = NULL;
        aacBufferSize = 1024;
        aacBuffer = malloc(aacBufferSize * sizeof(uint8_t));
        audioFrameCount = 0;
        memset(aacBuffer, 0, aacBufferSize);
        
        

    
    }
    return self;
}


//设置编码参数


-(void)setupEncoderFromSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    
   AudioStreamBasicDescription inAudioStreamBasicDescription = *CMAudioFormatDescriptionGetStreamBasicDescription((CMAudioFormatDescriptionRef)CMSampleBufferGetFormatDescription(sampleBuffer));
    
  AudioStreamBasicDescription outAudioStreamBasicDescription = {0}; // 初始化输出流的结构体描述为0. 很重要。
    outAudioStreamBasicDescription.mSampleRate = inAudioStreamBasicDescription.mSampleRate; // 音频流，在正常播放情况下的帧率。如果是压缩的格式，这个属性表示解压缩后的帧率。帧率不能为0。
    
    
    outAudioStreamBasicDescription.mFormatID = kAudioFormatMPEG4AAC; // 设置编码格式
    NSLog(@"音频编码格式为AAC");
    
    outAudioStreamBasicDescription.mFormatFlags = kMPEG4Object_AAC_LC; // 无损编码 ，0表示没有

    outAudioStreamBasicDescription.mBytesPerPacket = 0; // 每一个packet的音频数据大小。如果的动态大小，设置为0。动态大小的格式，需要用AudioStreamPacketDescription 来确定每个packet的大小。
    outAudioStreamBasicDescription.mFramesPerPacket = 1024; // 每个packet的帧数。如果是未压缩的音频数据，值是1。动态帧率格式，这个值是一个较大的固定数字，比如说AAC的1024。如果是动态大小帧数（比如Ogg格式）设置为0。
    outAudioStreamBasicDescription.mBytesPerFrame = 0; //  每帧的大小。每一帧的起始点到下一帧的起始点。如果是压缩格式，设置为0 。
    outAudioStreamBasicDescription.mChannelsPerFrame = 1; // 声道数
    outAudioStreamBasicDescription.mBitsPerChannel = 0; // 压缩格式设置为0
    outAudioStreamBasicDescription.mReserved = 0; // 8字节对齐，填0.
    myoutAudioStreamBasicDescription = outAudioStreamBasicDescription;
    
    UInt32 ulBitRate = 64000;
    UInt32 ulSize = sizeof(ulBitRate);
    AudioConverterSetProperty(audioConverter, kAudioConverterEncodeBitRate, ulSize, &ulBitRate);
//
//    if (bitRateStatus != 0) {
//        NSLog(@"bitRateStatus create faulse = %d",(int)bitRateStatus);
//        return;
//    }
//
    
    
    AudioClassDescription *description = [self
                                          getAudioClassDescriptionWithType:kAudioFormatMPEG4AAC
                                          fromManufacturer:kAppleSoftwareAudioCodecManufacturer]; //软编
    
    OSStatus status = AudioConverterNewSpecific(&inAudioStreamBasicDescription, &outAudioStreamBasicDescription, 1, description, &audioConverter); // 创建转换器
    if (status != 0) {
        NSLog(@"setup converter: %d", (int)status);
    }
}

/**
 *  获取编解码器
 *
 *  @param type         编码格式
 *  @param manufacturer 软/硬编
 *
 编解码器（codec）指的是一个能够对一个信号或者一个数据流进行变换的设备或者程序。这里指的变换既包括将 信号或者数据流进行编码（通常是为了传输、存储或者加密）或者提取得到一个编码流的操作，也包括为了观察或者处理从这个编码流中恢复适合观察或操作的形式的操作。编解码器经常用在视频会议和流媒体等应用中。
 *  @return 指定编码器
 */
- (AudioClassDescription *)getAudioClassDescriptionWithType:(UInt32)type
                                           fromManufacturer:(UInt32)manufacturer
{
    static AudioClassDescription desc;
    
    UInt32 encoderSpecifier = type;
    OSStatus st;
    
    UInt32 size;
    st = AudioFormatGetPropertyInfo(kAudioFormatProperty_Encoders,
                                    sizeof(encoderSpecifier),
                                    &encoderSpecifier,
                                    &size);
    
    NSLog(@"音频format size = %d",size);
    if (st) {
        NSLog(@"error getting audio format propery info: %d", (int)(st));
        return nil;
    }
    
    unsigned int count = size / sizeof(AudioClassDescription);
    AudioClassDescription descriptions[count];
    st = AudioFormatGetProperty(kAudioFormatProperty_Encoders,
                                sizeof(encoderSpecifier),
                                &encoderSpecifier,
                                &size,
                                descriptions);
    if (st) {
        NSLog(@"error getting audio format propery: %d", (int)(st));
        return nil;
    }
    NSLog(@"音频format count = %d",count);
    
    
    for (unsigned int i = 0; i < count; i++) {
        if ((type == descriptions[i].mSubType) &&
            (manufacturer == descriptions[i].mManufacturer)) {
            memcpy(&desc, &(descriptions[i]), sizeof(desc));
            return &desc;
        }
    }
   
    return nil;
}


/**
 *  A callback function that supplies audio data to convert. This callback is invoked repeatedly as the converter is ready for new input data.
    一个回调函数，提供音频数据转换。这个回调函数被重复调用，因为转换器已经准备好了新的输入数据。
 */
OSStatus inInputDataProc(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
    aacEncoder *encoder = (__bridge aacEncoder *)(inUserData);
    UInt32 requestedPackets = *ioNumberDataPackets;
    
    size_t copiedSamples = [encoder copyPCMSamplesIntoBuffer:ioData];
    if (copiedSamples < requestedPackets) {
        //PCM 缓冲区还没满
        *ioNumberDataPackets = 0;
        return -1;
    }
    *ioNumberDataPackets = 1;
    
    return noErr;
}

/**
 *  填充PCM到缓冲区
 */
- (size_t) copyPCMSamplesIntoBuffer:(AudioBufferList*)ioData {
    size_t originalBufferSize = pcmBufferSize;
    if (!originalBufferSize) {
        return 0;
    }
    ioData->mBuffers[0].mData = pcmBuffer;
    ioData->mBuffers[0].mDataByteSize = (int)
    pcmBufferSize;
    pcmBuffer = NULL;
    pcmBufferSize = 0;
    return originalBufferSize;
}


- (void) encodeSampleBuffer:(CMSampleBufferRef)sampleBuffer completionBlock:(void (^)(NSData * encodedData, NSError* error))completionBlock {
    CFRetain(sampleBuffer);
    dispatch_async(encoderQueue, ^{
        audioFrameCount ++;
        NSLog(@"音频帧个数：audioFrameCount = %d",audioFrameCount);
        
        if (!audioConverter) {
            [self setupEncoderFromSampleBuffer:sampleBuffer];
        }
        CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
        CFRetain(blockBuffer);
        OSStatus status = CMBlockBufferGetDataPointer(blockBuffer, 0, NULL, &pcmBufferSize, &pcmBuffer);
        NSError *error = nil;
        if (status != kCMBlockBufferNoErr) {
            error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil];
        }
        memset(aacBuffer, 0, aacBufferSize);
        
        AudioBufferList outAudioBufferList = {0};
        
        outAudioBufferList.mNumberBuffers = 1;
        outAudioBufferList.mBuffers[0].mNumberChannels = 1;
        outAudioBufferList.mBuffers[0].mDataByteSize = (int)aacBufferSize;
        
      
       // NSLog(@"outAudio帧数据长度为 %lu",(unsigned long)aacBufferSize);
        
        outAudioBufferList.mBuffers[0].mData = aacBuffer;
        
        _outAudioBufferList = outAudioBufferList;
        
        
        AudioStreamPacketDescription *outPacketDescription = NULL;
        UInt32 ioOutputDataPacketSize = 1;
        // Converts data supplied by an input callback function, supporting non-interleaved and packetized formats.
        // Produces a buffer list of output data from an AudioConverter. The supplied input callback function is called whenever necessary.
        status = AudioConverterFillComplexBuffer(audioConverter, inInputDataProc, (__bridge void *)(self), &ioOutputDataPacketSize, &outAudioBufferList, outPacketDescription);
        NSData *data = nil;
        if (status == 0) {
//            NSData *rawAAC = [NSData dataWithBytes:outAudioBufferList.mBuffers[0].mData length:outAudioBufferList.mBuffers[0].mDataByteSize];
            NSData * rawAAC = [self.myaacEncoderDelegate gotAudioEncodedData];
            NSData *adtsHeader = [self.myaacEncoderDelegate adtsDataForPacketLength:rawAAC.length];
//
//            NSLog(@"音频头长度为: adtsHeader = %lu",adtsHeader.length);
//            NSLog(@"rawAAC 长度为：%lu",rawAAC.length);
//            
//            sr___media_frame___t *frame = NULL;
//            sr___media_frame___create(&frame);
//            
//            frame->size = 4 + rawAAC.length;
//            frame->data = srmalloc(frame->size);
//            frame->data[0] = 0;
//            frame->data[1] = 0;
//            frame->data[2] = 0;
//            frame->data[3] = 1;
//            memcpy(frame->data + 4, [rawAAC bytes], rawAAC.length);
//   
//            CMTime h264TimeStamp = CMTimeMake(audioFrameCount, 1000);
//            NSLog(@"frameCount = %d",audioFrameCount);
//            
//            
//            frame->slice.timestamp = CMTimeGetSeconds(h264TimeStamp) ;
//            
//            //frame->slice.timestamp = 0;
//            frame->slice.type.media = SR___Media_Type___Audio;
//            frame->slice.type.payload = SR___Payload_Type___Frame;
//            
//            
//            sr___muxer___frame(amuxer, frame);
//            
//            
//            
            NSMutableData *fullData = [NSMutableData dataWithData:adtsHeader];
            [fullData appendData:rawAAC];

            
            NSLog(@"头加帧的长度： fullData = %lu",fullData.length);
            
            data = fullData;
        }
        else
        {
            error = [NSError errorWithDomain:NSOSStatusErrorDomain code:status userInfo:nil];
        }
        if (completionBlock) {
            dispatch_async(callbackQueue, ^{
                
              
                
                completionBlock(data, error);
            });
        }
        CFRelease(sampleBuffer);
        CFRelease(blockBuffer);
    });
}



/**
 *  Add ADTS header at the beginning of each and every AAC packet.
 *  This is needed as MediaCodec encoder generates a packet of raw
 *  AAC data.
 *
 *  Note the packetLen must count in the ADTS header itself.
 *  See: http://wiki.multimedia.cx/index.php?title=ADTS
 *  Also: http://wiki.multimedia.cx/index.php?title=MPEG-4_Audio#Channel_Configurations
 **/
//- (NSData*) adtsDataForPacketLength:(NSUInteger)packetLength {
//    
//    
//    int adtsLength = 7;
//    char *packet = malloc(sizeof(char) * adtsLength);
//    // Variables Recycled by addADTStoPacket
//    int profile = 2;  //AAC LC AAC编码格式的第二种， 低复杂度的格式
//    //39=MediaCodecInfo.CodecProfileLevel.AACObjectELD;
//    int freqIdx = 4;  //44.1KHz
//    int chanCfg = 1;  //MPEG-4 Audio Channel Configuration. 1 Channel front-center
//    NSUInteger fullLength = adtsLength + packetLength;
//    // fill in ADTS data
//    packet[0] = (char)0xFF; // 11111111     = syncword
//    packet[1] = (char)0xF9; // 1111 1 00 1  = syncword MPEG-2 Layer CRC
//    packet[2] = (char)(((profile-1)<<6) + (freqIdx<<2) +(chanCfg>>2));
//    packet[3] = (char)(((chanCfg&3)<<6) + (fullLength>>11));
//    packet[4] = (char)((fullLength&0x7FF) >> 3);
//    packet[5] = (char)(((fullLength&7)<<5) + 0x1F);
//    packet[6] = (char)0xFC;  //1111 1100
//    NSData *data = [NSData dataWithBytesNoCopy:packet length:adtsLength freeWhenDone:YES];
//    
//    
////    
////    //sr___video_format___t format;
//    sr___audio_format___t audioFormat;
////    //format.codec_type = SR___Video_Codec_Type___AVC;
//    audioFormat.codec_type = SR___Audio_Codec_Type___AAC;
////    
////    //audioFormat.bit_rate =
//    audioFormat.channels =1;
////   // //audioFormat.extend_data
//    audioFormat.extend_data_size = data.length;
// 
//    audioFormat.sample_per_frame = 1024;
//    audioFormat.bit_rate = 64000;
//    audioFormat.sample_rate = myoutAudioStreamBasicDescription.mSampleRate;
//////    format.width = h264outputWidth;
//////    format.height = h264outputHeight;
//////    format.bit_rate = h264outputWidth * h264outputHeight * 50;
//////    format.frame_per_second =10;
//////    format.extend_data_size =  frameData.length;
//    memcpy(audioFormat.extend_data , [data bytes], data.length);
////    
//    //sr___media_frame___t *frame = sr___video_format___to___media_frame(&audioFormat);
//    sr___media_frame___t * frame = sr___audio_format___to___media_frame(&audioFormat);
//    
//   // frame->slice.type.media = SR___Media_Type___Video;
//    frame->slice.type.media = SR___Media_Type___Audio;
//    frame->slice.type.payload = SR___Payload_Type___Format;
//    sr___muxer___frame(amuxer, frame);
//    CMTime audioTimeStamp = CMTimeMake(audioFrameCount, 1000);
//    
//    frame->slice.timestamp =CMTimeGetSeconds(audioTimeStamp);
////    
////    
//    NSLog(@"Audioframe.header =%p",frame);
////    
////    
////    
//    
//    
//    
//    return data;
//}




-(void)audioEndEncoder
{
    AudioConverterDispose(audioConverter);
    free(aacBuffer);
}







@end
