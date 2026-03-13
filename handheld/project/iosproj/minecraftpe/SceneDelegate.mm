//
//  SceneDelegate.mm
//  minecraftpe
//
//  UIScene lifecycle support (iOS 13+).
//  Replaces the old AppDelegate window management path.
//

#import "SceneDelegate.h"
#import "minecraftpeAppDelegate.h"
#import "minecraftpeViewController.h"

API_AVAILABLE(ios(13.0))
@implementation SceneDelegate

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session
      options:(UISceneConnectionOptions *)connectionOptions
{
    UIWindowScene *windowScene = (UIWindowScene *)scene;

    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    self.window.backgroundColor = [UIColor blackColor];

    // Reuse the existing view controller + app delegate audio init
    minecraftpeViewController *vc = [[minecraftpeViewController alloc] init];
    self.window.rootViewController = vc;
    [vc release];

    [self.window makeKeyAndVisible];

    // Initialise audio through the app delegate (shared instance)
    minecraftpeAppDelegate *appDelegate = (minecraftpeAppDelegate *)[UIApplication sharedApplication].delegate;
    appDelegate.viewController = vc;

    NSLog(@"[SceneDelegate] scene connected, VC=%p", vc);
}

- (void)sceneDidBecomeActive:(UIScene *)scene
{
    NSLog(@"become-active: %@", [NSThread currentThread]);
    minecraftpeAppDelegate *appDelegate = (minecraftpeAppDelegate *)[UIApplication sharedApplication].delegate;
    [appDelegate.viewController startAnimation];
}

- (void)sceneWillResignActive:(UIScene *)scene
{
    NSLog(@"resign-active: %@", [NSThread currentThread]);
    minecraftpeAppDelegate *appDelegate = (minecraftpeAppDelegate *)[UIApplication sharedApplication].delegate;
    [appDelegate.viewController stopAnimation];
}

- (void)sceneDidEnterBackground:(UIScene *)scene
{
    minecraftpeAppDelegate *appDelegate = (minecraftpeAppDelegate *)[UIApplication sharedApplication].delegate;
    [appDelegate.viewController enteredBackground];
}

@end
