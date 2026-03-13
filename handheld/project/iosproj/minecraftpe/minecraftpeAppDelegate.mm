//
//  minecraftpeAppDelegate.mm
//  minecraftpe
//
//  Created by rhino on 10/17/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import "minecraftpeAppDelegate.h"
#import "minecraftpeViewController.h"


@implementation minecraftpeAppDelegate

@synthesize window = _window;
@synthesize viewController = _viewController;


- (void) registerDefaultsFromFile:(NSString*)filename {
    NSString* pathToUserDefaultsValues = [[NSBundle mainBundle]
                                      pathForResource:filename
                                      ofType:@"plist"];
    NSDictionary* userDefaultsValues = [NSDictionary dictionaryWithContentsOfFile:pathToUserDefaultsValues];
    [[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValues];
}

NSError* audioSessionError = nil;

- (void) initAudio {
    audioSession = [AVAudioSession sharedInstance];
    [audioSession setActive:YES error:&audioSessionError];

    if (audioSessionError)
        NSLog(@"Warning; Couldn't set audio active\n");

    // Register for interruption notifications (AVAudioSessionDelegate was removed in iOS 8)
    [[NSNotificationCenter defaultCenter]
        addObserver:self
        selector:@selector(handleAudioInterruption:)
        name:AVAudioSessionInterruptionNotification
        object:audioSession];

    audioSessionSoundCategory = AVAudioSessionCategoryPlayback;
    audioSessionError = nil;

    [audioSession setCategory:audioSessionSoundCategory error:&audioSessionError];

    if (audioSessionError)
        NSLog(@"Warning; Couldn't init audio\n");
}

- (void)handleAudioInterruption:(NSNotification *)notification {
    NSUInteger type = [notification.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
    if (type == AVAudioSessionInterruptionTypeBegan)
        [self setAudioEnabled:NO];
    else
        [self setAudioEnabled:YES];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [self initAudio];

    // When using UIScene lifecycle (iOS 13+), the SceneDelegate creates the
    // window and view controller.  The code below is the pre-scene fallback
    // that keeps the app working on older iOS versions (or if the scene
    // manifest is absent).
    if (@available(iOS 13.0, *)) {
        // Window creation handled by SceneDelegate
    } else {
        self.window = [[UIWindow alloc] initWithFrame:[UIScreen mainScreen].bounds];
        minecraftpeViewController *vc = [[minecraftpeViewController alloc] init];
        self.viewController = vc;
        self.window.rootViewController = vc;
        [vc release];
        [self.window makeKeyAndVisible];
    }

    NSLog(@"ViewController: %p", self.viewController);
    return YES;
}

// ─── UIScene configuration (iOS 13+) ─────────────────────────────────────────

- (UISceneConfiguration *)application:(UIApplication *)application
    configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession
                                  options:(UISceneConnectionOptions *)options
    API_AVAILABLE(ios(13.0))
{
    UISceneConfiguration *config =
        [[UISceneConfiguration alloc] initWithName:@"Default Configuration"
                                       sessionRole:connectingSceneSession.role];
    config.delegateClass = NSClassFromString(@"SceneDelegate");
    return [config autorelease];
}

// ─── Pre-scene lifecycle callbacks (still used on < iOS 13) ──────────────────

- (void)applicationWillResignActive:(UIApplication *)application
{
    NSLog(@"resign-active: %@", [NSThread currentThread]);
    [self.viewController stopAnimation];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    [self.viewController enteredBackground];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    NSLog(@"become-active: %@", [NSThread currentThread]);
    [self.viewController startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    [self.viewController stopAnimation];
}

- (void)dealloc
{
    [_window release];
    [_viewController release];
    [super dealloc];
}

+ (void) initialize {
    if ([self class] == [minecraftpeAppDelegate class]) {
    }
}

//
// Audio session management
//
- (void)setAudioEnabled:(BOOL)status {
    if(status) {
        NSLog(@"INFO - SoundManager: OpenAL Active");
		[audioSession setCategory:audioSessionSoundCategory error:&audioSessionError];
        if(audioSessionError) {
            NSLog(@"ERROR - SoundManager: Unable to set the audio session category with error: %@", audioSessionError);
            return;
        }
		[audioSession setActive:YES error:&audioSessionError];
		if (audioSessionError) {
            NSLog(@"ERROR - SoundManager: Unable to set the audio session state to YES with error: %@", audioSessionError);
            return;
        }
    } else {
        NSLog(@"INFO - SoundManager: OpenAL Inactive");
    }

    [_viewController setAudioEnabled:status];
}


@end
