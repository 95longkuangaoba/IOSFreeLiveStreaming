//
//  videoCaptureBaseClass.m
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/11/30.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "videoCaptureBaseClass.h"


//#define videoCaptureMethodNotImplemented() \
@throw [NSException exceptionWithName:NSInternalInconsistencyException \
                               reason:[NSString stringWithFormat:@"You must override %@ in a subclass.", NSStringFromSelector(_cmd)] \
                             userInfo:nil]





@implementation videoCaptureBaseClass

-(instancetype)init
{
    if (self = [super init]) {
        
    };
    return self;
}









//初始化摄像头
-(void)cameraCaptureInit:(BOOL)type
{
    
}

//设置摄像头采集
-(void)setupCameraCapture
{
    
}


-(void)swapCamera:(UIView *)view
{
   
}



//开始摄像
-(void)startCamera:(UIView *)view
{
    NSLog(@"I AM  SUPER!!!!");
    
}

//关闭摄像头
-(void)stopCamera
{
 
}





-(void)setCLDelegate:(id)del
{
   
}

@end
