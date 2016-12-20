//
//  H264HardEncoder.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/1.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "H264HardEncoder.h"



@implementation H264HardEncoder
{
    NSString * yuvFile;
    VTCompressionSessionRef EncodingSession;
    dispatch_queue_t aQueue;
    CMFormatDescriptionRef * format;
    CMSampleTimingInfo * timingInfo;
    int frameCount;
    NSData * sps;
    NSData * pps;
}




-(void)initWithConfiguration
{
    EncodingSession = nil;
    aQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    frameCount = 0;
    sps = NULL;
    pps = NULL;
}

void didCompressH264(void *outputCallbackRefCon, void *sourceFrameRefCon, OSStatus status, VTEncodeInfoFlags infoFlags,
                     CMSampleBufferRef sampleBuffer )
{
    if (status != 0) {
        return;
    }
    
    if (!CMSampleBufferDataIsReady(sampleBuffer)) {
        NSLog(@"didCompressH264 data is not ready");
        return;
    }
    
    H264HardEncoder * encoder= (__bridge H264HardEncoder *) outputCallbackRefCon;
    
    bool keyframe = !CFDictionaryContainsKey(CFArrayGetValueAtIndex(CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, true), 0), kCMSampleAttachmentKey_NotSync);
    
    
    if (keyframe)
    {
        CMFormatDescriptionRef format = CMSampleBufferGetFormatDescription(sampleBuffer);
        
        size_t spsSize ,spsCount;
        const uint8_t * sps;
        OSStatus statusCode = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 0, &sps, &spsSize, &spsCount, 0);
        if (statusCode == noErr)
        {
            size_t ppsSize , ppsCount;
            const uint8_t * pps;
            OSStatus statusCode = CMVideoFormatDescriptionGetH264ParameterSetAtIndex(format, 1, &pps, &ppsSize, &ppsCount, 0);
            if (statusCode == noErr)
            {
                encoder -> sps = [NSData dataWithBytes:sps length:spsSize];
                encoder ->pps = [NSData dataWithBytes:pps length:ppsSize];
                if (encoder ->_delegate) {
                    [encoder ->_delegate gotSpsPps:encoder -> sps pps:encoder -> pps];
                }
            }
        }
    }
    
    
//    CMBlockBufferRef dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
//    size_t length, totalLength;
//    
//    char * dataPointer ;
//    OSStatus statusCodeRet = CMBlockBufferGetDataPointer(dataBuffer, 0, &length, &totalLength, &dataPointer);
//    if (statusCodeRet == noErr) {
//        size_t bufferOffset = 0;
//        static const int AVCCHeaderLength = 4;
//        while (bufferOffset < totalLength - AVCCHeaderLength)
//        {
//            uint32_t NALUnitLength = 0;
//            memcpy(&NALUnitLength, dataPointer + bufferOffset, AVCCHeaderLength);
//            NSData * data = [[NSData alloc] initWithBytes:(dataPointer + bufferOffset+AVCCHeaderLength) length:NALUnitLength];
//            
//            [encoder -> _delegate gotEncodedData:data iskeyFrame:keyframe];
//            
//            bufferOffset += AVCCHeaderLength + NALUnitLength;
//            
//            
    
    
    CMBlockBufferRef dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
    size_t length, totalLength;
    char *dataPointer;
    OSStatus statusCodeRet = CMBlockBufferGetDataPointer(dataBuffer, 0, &length, &totalLength, &dataPointer);
    if (statusCodeRet == noErr) {
        
        size_t bufferOffset = 0;
        static const int AVCCHeaderLength = 4;
        while (bufferOffset < totalLength - AVCCHeaderLength)
        {
            uint32_t NALUnitLength = 0;
            memcpy(&NALUnitLength, dataPointer + bufferOffset, AVCCHeaderLength);
            NALUnitLength = CFSwapInt32BigToHost(NALUnitLength);
            NSData* data = [[NSData alloc] initWithBytes:(dataPointer + bufferOffset + AVCCHeaderLength) length:NALUnitLength];
            [encoder->_delegate gotEncodedData:data iskeyFrame:keyframe];
            bufferOffset += AVCCHeaderLength + NALUnitLength;
    
    
    
        }
    }
    
    

}


-(void)initEncode:(int)width height:(int)height
{
    dispatch_sync(aQueue, ^{
        OSStatus status = VTCompressionSessionCreate(NULL, width, height, kCMVideoCodecType_H264, NULL, NULL, NULL, didCompressH264, (__bridge void *)(self), &EncodingSession );
        
        if (status != 0) {
            NSLog(@"Error by VTCompressionSessionCreate ");
            return ;
        }
        
        
        
        VTSessionSetProperty(EncodingSession, kVTCompressionPropertyKey_RealTime, kCFBooleanTrue);
        VTSessionSetProperty(EncodingSession, kVTCompressionPropertyKey_ProfileLevel, kVTProfileLevel_H264_Baseline_4_1);
        
        //设置码率上限值 单位bps
        SInt32 bitRate = width * height * 50;
        
        CFNumberRef ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &bitRate);
        VTSessionSetProperty(EncodingSession, kVTCompressionPropertyKey_AverageBitRate, ref);
        
        
        //设置码率均值 单位是byte
        int bitRateLimit = width * height * 10;
        
        CFNumberRef bitRateLimitRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &bitRateLimit);
        
        VTSessionSetProperty(EncodingSession, kVTCompressionPropertyKey_DataRateLimits, bitRateLimitRef);
        
        
        
        int frameInterval = 10; //关键帧间隔 越低效果越屌 帧数据越大
        CFNumberRef  frameIntervalRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &frameInterval);
        VTSessionSetProperty(EncodingSession, kVTCompressionPropertyKey_MaxKeyFrameInterval,frameIntervalRef);
        CFRelease(frameIntervalRef);
        VTCompressionSessionPrepareToEncodeFrames(EncodingSession);
        
        //设置期望帧率
        int fps = 10;
        CFNumberRef fpsRef= CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &fps);
        VTSessionSetProperty(EncodingSession, kVTCompressionPropertyKey_ExpectedFrameRate, fpsRef);
        
        
        
        
    });
    
}


-(void)encode:(CMSampleBufferRef)sampleBuffer
{
    if (EncodingSession == nil || EncodingSession == NULL  ) {
        return;
    }
    dispatch_sync(aQueue, ^{
        frameCount++;
        
        NSLog(@"frameCount = %d",frameCount);
        CVImageBufferRef imageBuffer = (CVImageBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
        CMTime presentationTimeStamp = CMTimeMake(frameCount, 1000);
        VTEncodeInfoFlags flags;
        OSStatus statusCode = VTCompressionSessionEncodeFrame(EncodingSession, imageBuffer, presentationTimeStamp, kCMTimeInvalid, NULL, NULL, &flags);
        if (statusCode != noErr) {
            if (EncodingSession != nil ||EncodingSession != NULL) {
                VTCompressionSessionInvalidate(EncodingSession);//Invalidate 使无效
                CFRelease(EncodingSession);
                EncodingSession = NULL;
                return ;
            }
        }
    });
}




-(void)endEncoder
{
    
  
    
    
    VTCompressionSessionCompleteFrames(EncodingSession, kCMTimeInvalid);
    VTCompressionSessionInvalidate(EncodingSession);
   // CFRelease(EncodingSession);
    EncodingSession = NULL;
    return;
  
}




@end
