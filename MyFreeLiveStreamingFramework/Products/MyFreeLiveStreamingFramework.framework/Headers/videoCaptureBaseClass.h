//
//  videoCaptureBaseClass.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/11/30.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <AVKit/AVKit.h>

@import AVFoundation;
@import VideoToolbox;


@protocol videoCaptureBaseClassDelegate <NSObject,AVCaptureVideoDataOutputSampleBufferDelegate>

@required
- (void)captureOutput:(AVCaptureOutput *)captureOutput CLdidOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer fromConnection:(AVCaptureConnection *)connection;


@end


@interface videoCaptureBaseClass : NSObject


@property (nonatomic, assign) id<videoCaptureBaseClassDelegate> myDelegate;
//@property(nonatomic,weak)AVCaptureConnection * _connectionVideo;


//初始化摄像头
-(void)cameraCaptureInit:(BOOL)type;

//设置摄像头采集
-(void)setupCameraCapture;

//前后切换摄像机
-(void)swapCamera:(UIView *)view;


//开始摄像
-(void)startCamera:(UIView *)view;

//关闭摄像头
-(void)stopCamera;
//设置代理
-(void)setCLDelegate:(id)del;



@end
