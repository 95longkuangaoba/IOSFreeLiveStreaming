//
//  H264HardDecoder.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/6.
//  Copyright © 2016年 陈龙. All rights reserved.
//





#import "H264HardDecoder.h"



#define h264outputWidth 480
#define h264outputHeight 360



//const uint8_t lyStartCode[4] = {0,0,0,1};

@interface H264HardDecoder ()
{
    uint8_t * _sps;
    NSInteger _spsSize;
    uint8_t * _pps;
    NSInteger _ppsSize;
    VTDecompressionSessionRef _decoderSession;
    CMVideoFormatDescriptionRef _decoderFormatDescription;
   // CVPixelBufferRef  pixelBuffer;
}
@end
@implementation H264HardDecoder
//解码回调函数
static void didDecompress( void *decompressionOutputRefCon, void *sourceFrameRefCon, OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef pixelBuffer, CMTime presentationTimeStamp, CMTime presentationDuration )
{
    NSLog(@"333333333");
    CVPixelBufferRef *outputPixelBuffer = (CVPixelBufferRef *)sourceFrameRefCon;
    *outputPixelBuffer = CVPixelBufferRetain(pixelBuffer);
    H264HardDecoder *decoder = (__bridge H264HardDecoder *)decompressionOutputRefCon;
    if (decoder.videoDecoderDelegate!=nil)
    {
        NSLog(@"222222222");
        [decoder.videoDecoderDelegate displayDecodedFrame:pixelBuffer];
    }
}

//-(BOOL)initH264D sps:(uint8_t *)sps pps:(uint8_t *)pps uint8_t spsPPSsize:(uint8_t)spsppsSIZE
//{
//    
//}



-(BOOL)initH264Decoder {
    if(_decoderSession) {
        return YES;
    }
    
    //把sps pps 包装成CMVideoFormatDescription
    const uint8_t* const parameterSetPointers[2] = { _sps, _pps };
    const size_t parameterSetSizes[2] = { _spsSize, _ppsSize };
    NSLog(@"sps[0] = %x , sps[1] = %x  sps[2] = %x  sps[3] = %x",_sps[0],_sps[1],_sps[2],_sps[3]);
    NSLog(@"pps[0] = %x , pps[1] = %x  pps[2] = %x  pps[3] = %x",_pps[0],_pps[1],_pps[2],_pps[3]);
   
    OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                          2, //param count
                                                                          parameterSetPointers,
                                                                          parameterSetSizes,
                                                                          4, //nal start code size
                                                                          &_decoderFormatDescription);
    
    if(status == noErr) {
        NSDictionary* destinationPixelBufferAttributes = @{
                                                           (id)kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithInt:kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange],
                                                           //硬解必须是 kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange
                                                           //                                                           或者是kCVPixelFormatType_420YpCbCr8Planar
                                                           //因为iOS是  nv12  其他是nv21
                                                           (id)kCVPixelBufferWidthKey : [NSNumber numberWithInt:h264outputHeight*2],
                                                           (id)kCVPixelBufferHeightKey : [NSNumber numberWithInt:h264outputWidth*2],
                                                           //这里款高和编码反的
                                                           (id)kCVPixelBufferOpenGLCompatibilityKey : [NSNumber numberWithBool:YES]
                                                           };
        
        
        VTDecompressionOutputCallbackRecord callBackRecord;
        callBackRecord.decompressionOutputCallback = didDecompress;
        callBackRecord.decompressionOutputRefCon = (__bridge void *)self;
       OSStatus Sessionstatus = VTDecompressionSessionCreate(kCFAllocatorDefault,
                                              _decoderFormatDescription,
                                              NULL,
                                              (__bridge CFDictionaryRef)destinationPixelBufferAttributes,
                                              &callBackRecord,
                                              &_decoderSession);
        if (Sessionstatus == noErr) {
            VTSessionSetProperty(_decoderSession, kVTDecompressionPropertyKey_ThreadCount, (__bridge CFTypeRef)[NSNumber numberWithInt:1]);
            VTSessionSetProperty(_decoderSession, kVTDecompressionPropertyKey_RealTime, kCFBooleanTrue);
        }
        else
        {
            NSLog(@"FormatDescriptionCreateFromH264 failed status=%d", Sessionstatus);

            
        }
    
    }
    else
    {
       NSLog(@"IOS8VT: reset decoder session failed Sessionstatus=%d", status);
    }
    
    return YES;
}

//-(CVPixelBufferRef)decode:(uint8_t *)frame withSize:(uint32_t)frameSize
//{
//    CVPixelBufferRef outputPixelBuffer = NULL;
//    
//    
//    //用blockBuffer把NALUnit包装起来
//    CMBlockBufferRef blockBuffer = NULL;
//    CMBlockBufferCreateWithMemoryBlock(<#CFAllocatorRef  _Nullable structureAllocator#>, <#void * _Nullable memoryBlock#>, <#size_t blockLength#>, <#CFAllocatorRef  _Nullable blockAllocator#>, <#const CMBlockBufferCustomBlockSource * _Nullable customBlockSource#>, <#size_t offsetToData#>, <#size_t dataLength#>, <#CMBlockBufferFlags flags#>, <#CMBlockBufferRef  _Nullable * _Nonnull newBBufOut#>)
//    OSStatus status  = CMBlockBufferCreateWithMemoryBlock(NULL,
//                                                          (void *)frame,
//                                                          frameSize,
//                                                          kCFAllocatorNull,
//                                                          NULL,
//                                                          0,
//                                                          frameSize,
//                                                          FALSE,
//                                                          &blockBuffer);
//    if(status == kCMBlockBufferNoErr)
//    {
//        
//        //创建sampleBuffer
//        CMSampleBufferRef sampleBuffer = NULL;
//        const size_t sampleSizeArray[] = {frameSize};
//        
//        
//        status = CMSampleBufferCreateReady(kCFAllocatorDefault,
//                                           blockBuffer,
//                                           _decoderFormatDescription ,
//                                           1, 0, NULL, 1, sampleSizeArray,
//                                           &sampleBuffer);
//        if (status == kCMBlockBufferNoErr && sampleBuffer)
//        {
//            
//            // 默认是同步操作。
//            // 调用didDecompress，返回后再回调
//            //传入CMsampleBuffer
//            VTDecodeFrameFlags flags = 0;
//            VTDecodeInfoFlags flagOut = 0;
//            OSStatus decodeStatus = VTDecompressionSessionDecodeFrame(_decoderSession,
//                                                                      sampleBuffer,
//                                                                      flags,
//                                                                      &outputPixelBuffer,
//                                                                      &flagOut);
//            
//            if(decodeStatus == kVTInvalidSessionErr) {
//                NSLog(@"IOS8VT: Invalid session, reset decoder session");
//            } else if(decodeStatus == kVTVideoDecoderBadDataErr) {
//                NSLog(@"IOS8VT: decode failed status=%d(Bad data)", decodeStatus);
//            } else if(decodeStatus != noErr) {
//                NSLog(@"IOS8VT: decode failed status=%d", decodeStatus);
//            }
//            CFRelease(sampleBuffer);
//        }
//        CFRelease(blockBuffer);
//    }
//    
//    return outputPixelBuffer;
//}

//
//-(void) decodeNalu:(uint8_t *)frame withSize:(uint32_t)frameSize
//{
//    //    NSLog(@">>>>>>>>>>开始解码");
//    
//    
//    //因为编码的时候 定义开始码 是 “0x00 00 01”   根据开始码可以定义NALU  0x1F转成2进制为00011111 0x05转成2进制为0101 0x07转成2进制为0111 0x08转2进制为1000
//    //替换头字节长度
//    int nalu_type = (frame[4] & 0x1F);
//    CVPixelBufferRef pixelBuffer = NULL;
//    uint32_t nalSize = (uint32_t)(frameSize - 4);
//    uint8_t *pNalSize = (uint8_t*)(&nalSize);
//    frame[0] = *(pNalSize + 3);
//    frame[1] = *(pNalSize + 2);
//    frame[2] = *(pNalSize + 1);
//    frame[3] = *(pNalSize);
//    //传输的时候。关键帧不能丢数据 否则绿屏   B/P可以丢  这样会卡顿
//    NSLog(@"nalu_type = %d",nalu_type);
//    switch (nalu_type)
//    {
//        case 0x05:
//            //           NSLog(@"nalu_type:%d Nal type is IDR frame",nalu_type);  //关键帧
//            if([self initH264Decoder])
//            {
//                pixelBuffer = [self decode:frame withSize:frameSize];
//            }
//            break;
//        case 0x07:
//            //           NSLog(@"nalu_type:%d Nal type is SPS",nalu_type);   //sps
//            _spsSize = frameSize - 4;
//            _sps = malloc(_spsSize);
//            memcpy(_sps, &frame[4], _spsSize);
//            NSLog(@"_sps = %s",_sps);
//            break;
//        case 0x08:
//        {
//            //            NSLog(@"nalu_type:%d Nal type is PPS",nalu_type);   //pps
//            _ppsSize = frameSize - 4;
//            _pps = malloc(_ppsSize);
//            memcpy(_pps, &frame[4], _ppsSize);
//            break;
//        }
//        default:
//        {
//            //            NSLog(@"Nal type is B/P frame");//其他帧
//            if([self initH264Decoder])
//            {
//                pixelBuffer = [self decode:frame withSize:frameSize];
//            }
//            break;
//        }
//            
//            
//    }
//}

-(void)decodeH264HeaderSPS:(uint8_t *)sps andwithSpsSize:(NSInteger)sps_size andwithpps:(uint8_t *)pps withppsSize:(NSInteger)pps_size
{
    _sps = malloc(sps_size);
     memcpy(_sps, sps+4, sps_size-4);
    _spsSize = sps_size;
    
    
    _pps = malloc(pps_size);
    memcpy(_pps, pps+4, pps_size-4);
    _ppsSize = pps_size;
    [self initH264Decoder];
    
}


-(CVPixelBufferRef)decode:(uint8_t *)videoData withSize:(uint32_t)dataSize
{
    
    NSLog(@"dataSize = %d",dataSize);
    
    
        CVPixelBufferRef outputPixelBuffer = NULL;
    
    
        //用blockBuffer把NALUnit包装起来
        CMBlockBufferRef blockBuffer = NULL;
//        CMBlockBufferCreateWithMemoryBlock(<#CFAllocatorRef  _Nullable structureAllocator#>, <#void * _Nullable memoryBlock#>, <#size_t blockLength#>, <#CFAllocatorRef  _Nullable blockAllocator#>, <#const CMBlockBufferCustomBlockSource * _Nullable customBlockSource#>, <#size_t offsetToData#>, <#size_t dataLength#>, <#CMBlockBufferFlags flags#>, <#CMBlockBufferRef  _Nullable * _Nonnull newBBufOut#>)
        OSStatus status  = CMBlockBufferCreateWithMemoryBlock(NULL,
                                                              (void *)videoData,
                                                              dataSize,
                                                              kCFAllocatorNull,
                                                              NULL,
                                                              0,
                                                              dataSize,
                                                              FALSE,
                                                              &blockBuffer);
        if(status == kCMBlockBufferNoErr)
        {
    
            //创建sampleBuffer
            CMSampleBufferRef sampleBuffer = NULL;
            const size_t sampleSizeArray[] = {dataSize};
    
    
            status = CMSampleBufferCreateReady(kCFAllocatorDefault,
                                               blockBuffer,
                                               _decoderFormatDescription ,
                                               1, 0, NULL, 1, sampleSizeArray,
                                               &sampleBuffer);
            if (status == kCMBlockBufferNoErr && sampleBuffer)
            {
    
                // 默认是同步操作。
                // 调用didDecompress，返回后再回调
                //传入CMsampleBuffer
                VTDecodeFrameFlags flags = 0;
                VTDecodeInfoFlags flagOut = 0;
                
          //      VTDecompressionSessionDecodeFrame(<#VTDecompressionSessionRef  _Nonnull session#>, <#CMSampleBufferRef  _Nonnull sampleBuffer#>, <#VTDecodeFrameFlags decodeFlags#>, <#void * _Nullable sourceFrameRefCon#>, <#VTDecodeInfoFlags * _Nullable infoFlagsOut#>)
            
                
                OSStatus decodeStatus = VTDecompressionSessionDecodeFrame(_decoderSession,
                                                                          sampleBuffer,
                                                                          flags,
                                                                          &outputPixelBuffer,
                                                                          &flagOut);
    
                if(decodeStatus == kVTInvalidSessionErr) {
                    NSLog(@"IOS8VT: Invalid session, reset decoder session");
                } else if(decodeStatus == kVTVideoDecoderBadDataErr) {
                    NSLog(@"IOS8VT: decode failed status=%d(Bad data)", decodeStatus);
                } else if(decodeStatus != noErr) {
                    NSLog(@"IOS8VT: decode failed status=%d", decodeStatus);
                }
                CFRelease(sampleBuffer);
            }
            CFRelease(blockBuffer);
        }
    
    if (outputPixelBuffer){
        NSLog(@"IOS8VT: outputbuffer QQQQQQQQQQ");
    }
        
        return outputPixelBuffer;
    

}

//-(void)getpixelBuffer:(CVPixelBufferRef )pixBuffer
//{
//    pixelBuffer = NULL;
//    pixelBuffer = pixBuffer;
//    
//}











/*
#import "H264HardDecoder.h"



#define h264outputWidth 800
#define h264outputHeight 600



const uint8_t lyStartCode[4] = {0,0,0,1};
@implementation H264HardDecoder

{
    uint8_t * _sps;
    NSInteger _spsSize;
    uint8_t * _pps;
    NSInteger _ppsSize;
    VTDecompressionSessionRef _deocderSession;
    CMVideoFormatDescriptionRef _decoderFormatDescription;
    CADisplayLink * mDisplayLink;
    dispatch_queue_t decoderQueue;
    myOpenGLView * mOpenGLView;
    
    //输入
    NSInputStream * inputStream;
    uint8_t * packetBuffer;
    long packetSize;
    uint8_t * inputBuffer;
    NSInteger inputSize;
    NSInteger inputMaxSize;
    
}


-(instancetype)initH264Decoder
{
    if (self = [super init])
    {
        mDisplayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(updateFrame)];
        mDisplayLink.frameInterval = 2.0; // 默认是30FPS的帧率录制
        [mDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        [mDisplayLink setPaused:YES];
        

    }
    return self;

}

-(void)startDecode
{
    [self onInputStart];
    [mDisplayLink setPaused:NO];
    
}


-(void)onInputStart
{
    //NSString * audioPath  = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)lastObject]stringByAppendingPathComponent:@"chenlong.aac"];
    
    
    NSString * h264Path =[[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES)lastObject]stringByAppendingPathComponent:@"chenlong.h264"];
    
    if (h264Path == NULL) {
        NSLog(@"没找到视频文件");
    }
    else
    {
    
    inputStream =[[NSInputStream alloc]initWithFileAtPath:h264Path];
    
    [inputStream open];
    inputSize = 0;
    inputMaxSize = 640 * 480 * 3 * 4;
    inputBuffer = malloc(inputMaxSize);
    }
}




-(void)onInputEnd
{
    [inputStream close];
    inputStream = nil;
    if (inputBuffer) {
        free(inputBuffer);
        inputBuffer = NULL;
    }
    
    [mDisplayLink setPaused:YES];
    
}


-(void)readPacket
{
    if (packetSize && packetBuffer) {
        packetSize = 0;
        free(packetBuffer);
        packetBuffer = NULL;
    }
    if (inputSize < inputMaxSize && inputStream.hasBytesAvailable) {
        inputSize += [inputStream read:inputBuffer + inputSize maxLength:inputMaxSize - inputSize];
    }
    if (memcmp(inputBuffer, lyStartCode, 4)== 0) {
        if (inputSize > 4) {
            //除了开始码还有内容
            
            uint8_t * pStart = inputBuffer + 4;
            uint8_t * pEnd = inputBuffer + inputSize;
            while (pStart != pEnd ) {
                //这里使用一种简略的方式来获取这一帧的长度，同过查找下一个0x00000001来确定。
                if (memcmp(pStart - 3, lyStartCode, 4) == 0) {
                    packetSize = pStart - inputBuffer -3;
                    if (packetBuffer) {
                        free(packetBuffer);
                        packetBuffer  = NULL;
                    }
                    packetBuffer = malloc(packetSize);
                    memcpy(packetBuffer, inputBuffer, packetSize);//复制packet内容到新的缓冲区
                    memmove(inputBuffer, inputBuffer + packetSize, inputSize - packetSize);//把缓冲区前移
                    inputSize -= packetSize;
                    break;
                }
                else
                {
                    ++pStart;
                }
            }
            
            
            
        }
    }
}
    


void didDecompress(void *decompressionOutputRefCon, void *sourceFrameRefCon, OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef pixelBuffer, CMTime presentationTimeStamp, CMTime presentationDuration ){
    CVPixelBufferRef *outputPixelBuffer = (CVPixelBufferRef *)sourceFrameRefCon;
    *outputPixelBuffer = CVPixelBufferRetain(pixelBuffer);
}



-(void)updateFrame
{
    if (inputStream) {
        dispatch_sync(decoderQueue, ^{
            [self readPacket];
            if (packetBuffer == NULL || packetSize == 0) {
                [self onInputEnd];
                return ;
            }
            uint32_t nalSize = (uint32_t)(packetSize - 4);
            uint32_t *pNalSize = (uint32_t *)packetBuffer;
            *pNalSize = CFSwapInt32HostToBig(nalSize);
            
            //在buffer 的前面填入代表长度的int
            CVPixelBufferRef pixelBuffer = NULL;
            int nalType = packetBuffer[4] & 0x1F;
            switch (nalType) {
                case 0x05:
                    NSLog(@"Nal type is IDR frame");
                    [self initVideoToolBox];
                    pixelBuffer = [self decode];
                    break;
                case 0x07:
                    NSLog(@"Nal type is SPS");
                    _spsSize = packetSize - 4;
                    _sps = malloc(_spsSize);
                    memcpy(_sps, packetBuffer + 4, _spsSize);
                    break;
                case 0x08:
                    NSLog(@"Nal type is PPS");
                    _ppsSize = packetSize - 4;
                    _pps = malloc(_ppsSize);
                    memcpy(_pps, packetBuffer + 4, _ppsSize);
                    break;
                default:
                    NSLog(@"Nal type is B/P frame");
                    pixelBuffer = [self decode];
                    break;
            }
            
            if(pixelBuffer) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [mOpenGLView displayPixelBuffer:pixelBuffer];
                  
                    
                    CVPixelBufferRelease(pixelBuffer);
                });
            }
            NSLog(@"Read Nalu size %ld", packetSize);
        });
    }
}



-(CVPixelBufferRef)decode {
    
    CVPixelBufferRef outputPixelBuffer = NULL;
    if (_deocderSession) {
        CMBlockBufferRef blockBuffer = NULL;
        OSStatus status  = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault,
                                                              (void*)packetBuffer, packetSize,
                                                              kCFAllocatorNull,
                                                              NULL, 0, packetSize,
                                                              0, &blockBuffer);
        if(status == kCMBlockBufferNoErr) {
            CMSampleBufferRef sampleBuffer = NULL;
            const size_t sampleSizeArray[] = {packetSize};
            status = CMSampleBufferCreateReady(kCFAllocatorDefault,
                                               blockBuffer,
                                               _decoderFormatDescription,
                                               1, 0, NULL, 1, sampleSizeArray,
                                               &sampleBuffer);
            if (status == kCMBlockBufferNoErr && sampleBuffer) {
                VTDecodeFrameFlags flags = 0;
                VTDecodeInfoFlags flagOut = 0;
                // 默认是同步操作。
                // 调用didDecompress，返回后再回调
                OSStatus decodeStatus = VTDecompressionSessionDecodeFrame(_deocderSession,
                                                                          sampleBuffer,
                                                                          flags,
                                                                          &outputPixelBuffer,
                                                                          &flagOut);
                
                if(decodeStatus == kVTInvalidSessionErr) {
                    NSLog(@"IOS8VT: Invalid session, reset decoder session");
                } else if(decodeStatus == kVTVideoDecoderBadDataErr) {
                    NSLog(@"IOS8VT: decode failed status=%d(Bad data)", decodeStatus);
                } else if(decodeStatus != noErr) {
                    NSLog(@"IOS8VT: decode failed status=%d", decodeStatus);
                }
                
                CFRelease(sampleBuffer);
            }
            CFRelease(blockBuffer);
        }
    }
    
    return outputPixelBuffer;
}



- (void)initVideoToolBox {
    if (!_deocderSession) {
        const uint8_t* parameterSetPointers[2] = {_sps, _pps};
        const size_t parameterSetSizes[2] = {_spsSize, _ppsSize};
        OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(kCFAllocatorDefault,
                                                                              2, //param count
                                                                              parameterSetPointers,
                                                                              parameterSetSizes,
                                                                              4, //nal start code size
                                                                              &_decoderFormatDescription);
        if(status == noErr) {
            CFDictionaryRef attrs = NULL;
            const void *keys[] = { kCVPixelBufferPixelFormatTypeKey };
            //      kCVPixelFormatType_420YpCbCr8Planar is YUV420
            //      kCVPixelFormatType_420YpCbCr8BiPlanarFullRange is NV12
            uint32_t v = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
            const void *values[] = { CFNumberCreate(NULL, kCFNumberSInt32Type, &v) };
            attrs = CFDictionaryCreate(NULL, keys, values, 1, NULL, NULL);
            
            VTDecompressionOutputCallbackRecord callBackRecord;
            callBackRecord.decompressionOutputCallback = didDecompress;
            callBackRecord.decompressionOutputRefCon = NULL;
            
            status = VTDecompressionSessionCreate(kCFAllocatorDefault,
                                                  _decoderFormatDescription,
                                                  NULL, attrs,
                                                  &callBackRecord,
                                                  &_deocderSession);
            CFRelease(attrs);
        } else {
            NSLog(@"IOS8VT: reset decoder session failed status=%d", status);
        }
        
        
    }
}

- (void)EndVideoToolBox
{
    if(_deocderSession) {
        VTDecompressionSessionInvalidate(_deocderSession);
        CFRelease(_deocderSession);
        _deocderSession = NULL;
    }
    
    if(_decoderFormatDescription) {
        CFRelease(_decoderFormatDescription);
        _decoderFormatDescription = NULL;
    }
    
    free(_sps);
    free(_pps);
    _spsSize = _ppsSize = 0;
}

*/







@end
