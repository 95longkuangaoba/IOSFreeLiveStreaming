//
//  ViewController.m
//  IOSFreeLiveStreaming
//
//  Created by 陈龙 on 16/11/30.
//  Copyright © 2016年 陈龙. All rights reserved.
//

#import "ViewController.h"
#import "MyLog.h"
#import "RecoderIm.h"
#import "Player.h"
#import "videoCaptureBaseClass.h"
#import "audioCaptureBaseClass.h"
#import "videoCaptureInBaseClass.h"
#import "videoCaptureBaseClass.h"
#import "audioCaptureInBaseClass.h"


@interface ViewController ()
{
    videoCaptureInBaseClass * _vcbs;
    audioCaptureInBaseClass * _acbs;
    RecoderIm * recoder;
    NSData * sps;
    NSData * pps;
    NSData * frameData;
    bool isKeyFrame;
    id<videoCaptureBaseClassDelegate> _myDelegate;
    
    bool isClick;
    
    AVAudioPlayer * _player;
    Player * myPlayer;
 
}
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    MyLog * mylog = [[MyLog alloc]init];
    [mylog log:@"viewDidLoad"];
    
    NSLog(@"----------------------------");
    
   
    
    
    
    _vcbs = [[videoCaptureInBaseClass alloc]init];
    
    
    
    
   _acbs = [audioCaptureInBaseClass alloc];
   
    
   
   
    
    
    
    [_vcbs cameraCaptureInit:YES];
    [_vcbs setupCameraCapture];
   
   // [_vcbs startCamera:self.view];
   // NSLog(@"开启摄像头后_myDelegate = %p",_myDelegate);
    
    [_acbs initAudioCaputre];
    [_acbs setupAudioCaputre];
    
    isClick = false;
    
    
   
 
    
    
      myPlayer = [[Player alloc]init];
      [myPlayer displayVideo:self.view.layer];

}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}
- (IBAction)startBtn:(id)sender {

    if (isClick== false) {
        RecoderIm * recode = [[RecoderIm alloc]initWithvideoCapture:_vcbs audioCapture:_acbs];
        recoder = recode;
        
        _vcbs.myDelegate = recoder;
        
        _acbs.audioCaptureDelegate = recoder;
        [_acbs startAudioCapture];
        isClick = true;
    }
   
     
     }

- (IBAction)topBtn:(id)sender {
    
    
    if (isClick == true) {
        [recoder end];
        _vcbs.myDelegate = nil;
        [_acbs endAudioCapture];
        isClick =  false;
    }
   

}
- (IBAction)swapBtn:(id)sender {
    [_vcbs swapCamera:self.view];
    
}


- (IBAction)playAudio:(id)sender {
  
    [myPlayer audioPlay];
    
}
- (IBAction)videoPlay:(id)sender {
    
   // [self.view.layer display:myPlayer];
    //[myPlayer display:self.view.layer];
//    [self.view.layer display:myPlayer];
  
    
    
}
- (IBAction)vidoStop:(id)sender {
    [myPlayer stopDisplay];
   // [myPlayer xiaohui];
}



@end
