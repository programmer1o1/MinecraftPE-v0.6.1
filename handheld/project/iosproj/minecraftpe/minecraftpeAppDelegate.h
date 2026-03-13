//
//  minecraftpeAppDelegate.h
//  minecraftpe
//
//  Created by rhino on 10/17/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@class minecraftpeViewController;

@interface minecraftpeAppDelegate : UIResponder <UIApplicationDelegate> {
    AVAudioSession* audioSession;
    NSString*       audioSessionSoundCategory;
}

@property (nonatomic, retain) UIWindow *window;

@property (nonatomic, retain) minecraftpeViewController *viewController;

- (void)initAudio;

+ (void) initialize;

@end
