//
//  audioCaptureProcotol.h
//  MyFreeLiveStreamingFramework
//
//  Created by 陈龙 on 16/12/1.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol audioCaptureProtocol <NSObject>
@required

//初始化音频采集
-(void)initAudioCaputre;

//设置音频采集数据
-(void)setupAudioCaputre;

//开始音频采集
-(void)startAudioCapture;

//结束音频采集
-(void)endAudioCapture;

@end
