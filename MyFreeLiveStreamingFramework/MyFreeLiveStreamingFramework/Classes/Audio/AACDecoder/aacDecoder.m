//
//  aacDecoder.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/7.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "aacDecoder.h"
#import <AudioToolbox/AudioToolbox.h>


const uint32_t CONST_BUFFER_COUNT = 3;
const uint32_t CONST_BUFFER_SIZE = 0x10000;
@implementation aacDecoder
{
    
    AudioFileID audioFileID; // An opaque data type that represents an audio file object. 表示一个音频文件对象的不透明数据类型。
    AudioStreamBasicDescription audioStreamBasicDescrpition; // An audio data format specification for a stream of audio 一种音频流的音频数据格式规范
    AudioStreamPacketDescription *audioStreamPacketDescrption; // Describes one packet in a buffer of audio data where the sizes of the packets differ or where there is non-audio data between audio packets.描述在一个音频数据的缓冲区中的一个数据包的大小的数据包的不同，或在那里有音频数据包之间的非音频数据。
    
    AudioQueueRef audioQueue; // Defines an opaque data type that represents an audio queue. 定义一个表示音频队列的不透明数据类型。
    AudioQueueBufferRef audioBuffers[CONST_BUFFER_COUNT];
    
    SInt64 readedPacket; //参数类型
    u_int32_t packetNums;

    
    
  
}



-(instancetype)init
{
    if (self = [super init]) {
        
        
        
        
    }
    return self;
}

- (void)customAudioConfig

{
   
    
    //NSURL *url = [[NSBundle mainBundle] URLForResource:@"abc" withExtension:@"aac"];
    
    // NSString *audioFile = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"chenlong.aac"];
    NSString * audioPath  = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)lastObject]stringByAppendingPathComponent:@"chenlong.aac"];
    
    
    
    NSURL * url = [NSURL fileURLWithPath:audioPath];
    
 
    
   // OSStatus audioStatus = AudioFileOpenWithCallbacks(<#void * _Nonnull inClientData#>, <#AudioFile_ReadProc  _Nonnull inReadFunc#>, <#AudioFile_WriteProc  _Nullable inWriteFunc#>, <#AudioFile_GetSizeProc  _Nonnull inGetSizeFunc#>, <#AudioFile_SetSizeProc  _Nullable inSetSizeFunc#>, <#AudioFileTypeID inFileTypeHint#>, <#AudioFileID  _Nullable * _Nonnull outAudioFile#>)
   // AudioFileOpenURL(<#CFURLRef  _Nonnull inFileRef#>, <#AudioFilePermissions inPermissions#>, <#AudioFileTypeID inFileTypeHint#>, <#AudioFileID  _Nullable * _Nonnull outAudioFile#>)
    OSStatus status = AudioFileOpenURL((__bridge CFURLRef)url, kAudioFileReadPermission, 0, &audioFileID); //Open an existing audio file specified by a URL. 打开一个由网址指定的现有音频文件。
    if (status != noErr) {
        NSLog(@"打开文件失败 %@", url);
        return ;
    }
    uint32_t size = sizeof(audioStreamBasicDescrpition);
    status = AudioFileGetProperty(audioFileID, kAudioFilePropertyDataFormat, &size, &audioStreamBasicDescrpition); // Gets the value of an audio file property.获取音频文件属性的值。
    NSAssert(status == noErr, @"error");
    
    status = AudioQueueNewOutput(&audioStreamBasicDescrpition, bufferReady, (__bridge void * _Nullable)(self), NULL, NULL, 0, &audioQueue); // Creates a new playback audio queue object. 创建一个新的播放音频队列对象。
    NSAssert(status == noErr, @"error");
    
    if (audioStreamBasicDescrpition.mBytesPerPacket == 0 || audioStreamBasicDescrpition.mFramesPerPacket == 0) {
        uint32_t maxSize;
        size = sizeof(maxSize);
        AudioFileGetProperty(audioFileID, kAudioFilePropertyPacketSizeUpperBound, &size, &maxSize); // The theoretical maximum packet size in the file.文件中的理论最大数据包大小
        if (maxSize > CONST_BUFFER_SIZE) {
            maxSize = CONST_BUFFER_SIZE;
        }
        packetNums = CONST_BUFFER_SIZE / maxSize;
        audioStreamPacketDescrption = malloc(sizeof(AudioStreamPacketDescription) * packetNums);
    }
    else {
        packetNums = CONST_BUFFER_SIZE / audioStreamBasicDescrpition.mBytesPerPacket;
        audioStreamPacketDescrption = nil;
    }
    
    char cookies[100];
    memset(cookies, 0, sizeof(cookies));
    // 这里的100 有问题
    AudioFileGetProperty(audioFileID, kAudioFilePropertyMagicCookieData, &size, cookies); // Some file types require that a magic cookie be provided before packets can be written to an audio file. 一些文件类型要求在数据包被写入音频文件之前提供一个神奇的cookie。
    if (size > 0) {
        AudioQueueSetProperty(audioQueue, kAudioQueueProperty_MagicCookie, cookies, size); // Sets an audio queue property value.
    }
    
    readedPacket = 0;
    for (int i = 0; i < CONST_BUFFER_COUNT; ++i) {
        AudioQueueAllocateBuffer(audioQueue, CONST_BUFFER_SIZE, &audioBuffers[i]); // Asks an audio queue object to allocate an audio queue buffer.请求一个音频队列对象来分配一个音频队列缓冲区
        if ([self fillBuffer:audioBuffers[i]]) {
            // full
            break;
        }
        NSLog(@"buffer%d full", i);
    }
}

void bufferReady(void *inUserData,AudioQueueRef inAQ,
                 AudioQueueBufferRef buffer){
    NSLog(@"refresh buffer");
    aacDecoder* myaacDecoder = (__bridge aacDecoder *)inUserData;
    if (!myaacDecoder) {
        NSLog(@"player nil");
        return ;
    }
    if ([myaacDecoder fillBuffer:buffer]) {
        NSLog(@"播放完成");
    }
    
}


- (void)play {
    AudioQueueSetParameter(audioQueue, kAudioQueueParam_Volume, 1.0); // Sets a playback audio queue parameter value. 设置一个播放音频队列参数值
    AudioQueueStart(audioQueue, NULL); // Begins playing or recording audio. 开始播放或录制音频
}


- (bool)fillBuffer:(AudioQueueBufferRef)buffer {
    bool full = NO;
    uint32_t bytes = 0, packets = (uint32_t)packetNums;
    OSStatus status = AudioFileReadPackets(audioFileID, NO, &bytes, audioStreamPacketDescrption, readedPacket, &packets, buffer->mAudioData); // Reads packets of audio data from an audio file 从音频文件读取数据包的音频数据。.
    
    NSAssert(status == noErr, ([NSString stringWithFormat:@"error status %d", status]) );
    if (packets > 0) {
        buffer->mAudioDataByteSize = bytes;
        AudioQueueEnqueueBuffer(audioQueue, buffer, packets, audioStreamPacketDescrption);
        readedPacket += packets;
    }
    else {
        AudioQueueStop(audioQueue, NO);
        full = YES;
    }
    
    return full;
}



- (double)getCurrentTime {
    Float64 timeInterval = 0.0;
    if (audioQueue) {
        AudioQueueTimelineRef timeLine;
        AudioTimeStamp timeStamp;
        OSStatus status = AudioQueueCreateTimeline(audioQueue, &timeLine); // Creates a timeline object for an audio queue  为音频队列创建一个时间轴对象.
        if(status == noErr)
        {
            AudioQueueGetCurrentTime(audioQueue, timeLine, &timeStamp, NULL); // Gets the current audio queue time 得到当前音频队列时间.
            timeInterval = timeStamp.mSampleTime * 1000000 / audioStreamBasicDescrpition.mSampleRate; // The number of sample frames per second of the data in the stream 数据流中每秒钟的样本帧的数量.
        }
    }
    NSLog(@"timeInterval  = %.1fs",timeInterval);
    return timeInterval;
}







@end
