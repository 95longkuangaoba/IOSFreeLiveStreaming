//
//  videoCaptureInBaseClass.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/2.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "videoCaptureInBaseClass.h"
@import AVFoundation;
@import VideoToolbox;


@implementation videoCaptureInBaseClass
{
    AVCaptureSession * captureSession;
    
    AVCaptureDeviceInput *inputDevice;
    AVCaptureDevice * cameraDeviceB;
    AVCaptureDevice * cameraDeviceF;
    AVCaptureConnection * connectionVideo;
    BOOL cameraDeviceIsF;
   // id <videoCaptureBaseClassDelegate> _myDelegate;
    AVCaptureVideoPreviewLayer * recordLayer;
    
    
    
    AVCaptureOutput * _captureOutput;
    CMSampleBufferRef _sampleBuffer;
    AVCaptureConnection * _connection;
   
}


-(instancetype)init
{
    if (self = [super init]) {
        cameraDeviceIsF = YES;
//        _myDelegate =self.myDelegate  ;
//        NSLog(@"初始化的时候_mydelegate = %p",self.myDelegate);
       // super._connectionVideo = connectionVideo1;
    }
    return self;
}


-(void)setCLDelegate:(id)del
{
    self.myDelegate = del;
   // _myDelegate = self.myDelegate;
  //  NSLog(@"设置代理这里_myDelegate = %p",_myDelegate);
    
   
    
    
}


//初始化摄像头
-(void)cameraCaptureInit:(BOOL)type
{
    [super cameraCaptureInit:type];
    NSLog(@"-----------I------------");
    captureSession = [[AVCaptureSession alloc]init];
    
    
    NSArray * videoDevices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    
    
    
    
    for (AVCaptureDevice * device in videoDevices) {
        if (device.position == AVCaptureDevicePositionFront) {
            cameraDeviceF = device;
            
        }
        else if(device.position == AVCaptureDevicePositionBack)
        {
            cameraDeviceB = device;
            
        }
        
    }
    
    NSError * deviceError;
    if (type == false) {
        inputDevice = [AVCaptureDeviceInput deviceInputWithDevice:cameraDeviceB error:&deviceError];
    }
    else
    {
        inputDevice = [AVCaptureDeviceInput deviceInputWithDevice:cameraDeviceF error:&deviceError];
    }
 
    AVCaptureVideoDataOutput * outputVideoDevice = [[AVCaptureVideoDataOutput alloc]init];
    
    
    NSString * key = (NSString *)kCVPixelBufferPixelFormatTypeKey;
    NSNumber * val = [NSNumber numberWithUnsignedInt:kCVPixelFormatType_420YpCbCr8BiPlanarFullRange];
    NSDictionary * videoSettings = [NSDictionary dictionaryWithObject:val forKey:key];
    outputVideoDevice.videoSettings = videoSettings;
    
    
    outputVideoDevice= outputVideoDevice;
    [outputVideoDevice setSampleBufferDelegate:self queue:dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0)];
    
    [captureSession addInput:inputDevice];
    [captureSession addOutput:outputVideoDevice];
    
    [captureSession beginConfiguration];
    [captureSession commitConfiguration];
    [ captureSession setSessionPreset:[NSString stringWithString:AVCaptureSessionPreset1280x720]];
    
    connectionVideo = [outputVideoDevice connectionWithMediaType:AVMediaTypeVideo];
    
    recordLayer = [AVCaptureVideoPreviewLayer layerWithSession:captureSession];
    [recordLayer setVideoGravity:AVLayerVideoGravityResizeAspect];
    

    
  
}


//设置摄像头采集
-(void)setupCameraCapture
{
    
    NSLog(@"-----------AM------------");
    
    
}


-(void)swapCamera:(UIView *)view
{
   
    if (captureSession.isRunning == YES)
    {
        cameraDeviceIsF = !cameraDeviceIsF;
        NSLog(@"切换摄像头");
       // [self CLstopCamera];
        [self stopCamera];
        [self cameraCaptureInit:cameraDeviceIsF];
        [self startCamera:view];
        
    }
}



//开始摄像
-(void)startCamera:(UIView *)view
{
    [super startCamera:view];
    NSLog(@"I AM CHILD!!!!");
   //  NSLog(@"开启摄像头处_myDelegate = %p",_myDelegate);
    NSLog(@"-----------A------------");
    recordLayer = [AVCaptureVideoPreviewLayer layerWithSession:captureSession];
    [recordLayer setVideoGravity:AVLayerVideoGravityResizeAspect];
    recordLayer.frame = CGRectMake(0, 120, 160, 300);
    
    [view.layer addSublayer:recordLayer];
    [captureSession startRunning];
    
   

}

//关闭摄像头
-(void)stopCamera
{
   
    NSLog(@"-----------TEST------------");
    [captureSession stopRunning];
    [recordLayer removeFromSuperlayer];

}




- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection
{


    
    if (connection == connectionVideo) {

       [self.myDelegate captureOutput:captureOutput CLdidOutputSampleBuffer:sampleBuffer fromConnection:connection];

        
    }
    
}




@end
