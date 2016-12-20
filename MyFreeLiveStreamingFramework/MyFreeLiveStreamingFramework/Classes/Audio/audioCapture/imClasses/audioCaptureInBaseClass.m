//
//  audioCaptureInBaseClass.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/4.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "audioCaptureInBaseClass.h"


@implementation audioCaptureInBaseClass
{
    dispatch_queue_t audioCaptureQueue;
    AVCaptureSession * audioSession;
    AVCaptureDeviceInput * audioInput;
    AVCaptureAudioDataOutput * audioOutput;
    AVCaptureConnection * audioConnection;
    AVCaptureDevice * audioDevice;
    
    NSFileHandle * audioFileHandle;
    
}






-(void)initAudioCaputre
{
    audioSession = [[AVCaptureSession alloc]init];
    
    audioCaptureQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    
    audioDevice = [[AVCaptureDevice devicesWithMediaType:AVMediaTypeAudio]lastObject];
    

    
    NSLog(@"初始化音频采集器");
    
    audioInput = [[AVCaptureDeviceInput alloc]initWithDevice:audioDevice error:nil];
    if ([audioSession canAddInput:audioInput])
    {
        [audioSession addInput: audioInput];
        
    }
    
    audioOutput = [[AVCaptureAudioDataOutput alloc]init];
    if ([audioSession canAddOutput:audioOutput])
    {
        [audioSession addOutput:audioOutput];
    }
    
    [audioSession beginConfiguration];
    [audioSession commitConfiguration];
    
    audioConnection = [audioOutput connectionWithMediaType:AVMediaTypeAudio];
    
    [audioOutput setSampleBufferDelegate:self queue:audioCaptureQueue];
    
    
    NSLog(@"设置音频采集");

    
}
    
 
    
    //设置音频采集数据
-(void)setupAudioCapture
{
    
  

}
    
    //开始音频采集
-(void)startAudioCapture
{
    
       [audioSession startRunning];
    NSLog(@"音频开始采集");

}
    
    //结束音频采集
-(void)endAudioCapture
{
    
    
    [audioSession stopRunning];
    
    [audioFileHandle closeFile];
    audioFileHandle = NULL;
    
    NSLog(@"音频结束采集");

}


-(void)setAUDelegate:(id)del
{
    self.audioCaptureDelegate = del;
    NSLog(@"设置音频采集代理");
    
}
    
    


- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{
    
    //NSLog(@"音频采集");
    if (  connection == audioConnection) {
        
        
        [self.audioCaptureDelegate captureOutput:captureOutput AUdidOutputSampleBuffer:sampleBuffer fromConnection:connection];
     
        
    }
}

@end
