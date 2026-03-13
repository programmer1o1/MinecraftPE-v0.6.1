//
//  minecraftpeViewController.mm
//  minecraftpe
//
//  Copyright 2011 Mojang AB. All rights reserved.
//
//  Uses MGLKView (MetalANGLE's GLKView equivalent) for GLES rendering.
//  MGLKView properly manages the drawable-backed framebuffer and handles
//  presentation to a CAMetalLayer internally.
//

#import <QuartzCore/QuartzCore.h>

#import "minecraftpeViewController.h"

#import <MetalANGLE/MGLKit.h>
#import <MetalANGLE/EGL/egl.h>

#import "../../../src/platform/input/Multitouch.h"
#import "../../../src/platform/input/Mouse.h"
#import "../../../src/client/OptionStrings.h"

#import "../../../src/App.h"
#import "../../../src/AppPlatform_iOS.h"
#import "../../../src/NinecraftApp.h"

#import "dialogs/CreateNewWorldViewController.h"
#import "dialogs/RenameMPWorldViewController.h"
#import "../../lib_projects/InAppSettingsKit/Models/IASKSettingsReader.h"
#import "../../lib_projects/InAppSettingsKit/Views/IASKSwitch.h"
#import "../../lib_projects/InAppSettingsKit/Views/IASKPSToggleSwitchSpecifierViewCell.h"

// ─── Private interface ────────────────────────────────────────────────────────

@interface minecraftpeViewController () <MGLKViewDelegate>
@property (nonatomic, assign) CADisplayLink *displayLink;

// GLES surface (MGLKView manages the framebuffer + Metal drawable)
@property (nonatomic, retain) MGLKView *mglkView;

- (void)_resetAllPointers;
- (void)_fixAlphaToOne;
@end

// Need real GL functions for _fixAlphaToOne (macros redirect to shader/MatrixStack)
#include "../../../src/client/renderer/gles.h"
#if defined(__APPLE__) && !defined(MACOS)
// Un-define the macros we need as real GL calls in this file
#undef glEnable
#undef glDisable
#undef glGetFloatv
#undef glGetIntegerv
#undef glIsEnabled
#undef glDrawArrays
extern "C" void glEnable(GLenum cap);
extern "C" void glDisable(GLenum cap);
extern "C" void glDrawArrays(GLenum mode, GLint first, GLsizei count);
#endif

// ─── Implementation ───────────────────────────────────────────────────────────

@implementation minecraftpeViewController {
    int _pixW, _pixH;
}

@synthesize animating;
@synthesize displayLink;
@synthesize appSettingsViewController;
@synthesize mglkView = _mglkView;

// ─── Settings VC (lazy) ──────────────────────────────────────────────────────

- (IASKAppSettingsViewController *)appSettingsViewController {
    if (!appSettingsViewController) {
        appSettingsViewController = [[IASKAppSettingsViewController alloc]
            initWithNibName:@"IASKAppSettingsView" bundle:nil];
        appSettingsViewController.delegate = self;
        [[NSNotificationCenter defaultCenter]
            addObserver:self selector:@selector(settingToggled:)
            name:kIASKAppSettingChanged object:nil];
    }
    return appSettingsViewController;
}

// ─── View creation ───────────────────────────────────────────────────────────

- (void)loadView
{
    UIView *root = [[UIView alloc] initWithFrame:[UIScreen mainScreen].bounds];
    root.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    root.opaque = YES;
    root.backgroundColor = [UIColor blackColor];
    self.view = root;
    [root release];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Extend content under status bar / notch area for true full-screen
    self.edgesForExtendedLayout = UIRectEdgeAll;
    self.extendedLayoutIncludesOpaqueBars = YES;

    animating              = NO;
    animationFrameInterval = 1;
    self.displayLink       = nil;
    viewScale              = [UIScreen mainScreen].scale;
    _pixW = _pixH          = 0;

    _keyboardView = [[ShowKeyboardView alloc] init];

    _platform           = new AppPlatform_iOS(self);
    _context            = new AppContext();
    _context->platform  = _platform;
    _context->doRender  = false;

    NSArray *docPaths   = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSArray *cachePaths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory,   NSUserDomainMask, YES);

    _app = new NinecraftApp();
    ((Minecraft *)_app)->externalStoragePath      = [[docPaths   objectAtIndex:0] UTF8String];
    ((Minecraft *)_app)->externalCacheStoragePath = [[cachePaths objectAtIndex:0] UTF8String];

    _touchMap = new UITouch *[Multitouch::MAX_POINTERS];
    [self _resetAllPointers];

    _dialog             = nil;
    _dialogResultStatus = -1;

    // ─── GLES surface (MGLKView) ──────────────────────────────────────────────
    // MGLKView is MetalANGLE's equivalent of GLKView.  It creates a proper
    // drawable-backed framebuffer (defaultOpenGLFrameBufferID) and handles
    // presentation via its internal CAMetalLayer.
    MGLContext *ctx = [[MGLContext alloc] initWithAPI:kMGLRenderingAPIOpenGLES2];
    NSAssert(ctx, @"Failed to create MGLContext");

    MGLKView *glView = [[MGLKView alloc] initWithFrame:self.view.bounds context:ctx];
    [ctx release];
    glView.autoresizingMask     = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    glView.drawableColorFormat  = MGLDrawableColorFormatRGBA8888;
    glView.drawableDepthFormat  = MGLDrawableDepthFormat24;
    glView.opaque               = YES;
    glView.multipleTouchEnabled = YES;
    glView.contentScaleFactor   = viewScale;
    glView.delegate             = self;
    glView.enableSetNeedsDisplay = NO;  // we drive rendering from CADisplayLink
    [self.view addSubview:glView];
    self.mglkView = glView;
    [glView release];

    // Make context current for initial resource loading
    [MGLContext setCurrentContext:glView.context];

    NSLog(@"[VC] viewDidLoad: MGLKView defaultFBO=%u drawableSize=%@",
          glView.defaultOpenGLFrameBufferID,
          NSStringFromCGSize(glView.drawableSize));
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];

    if (!_app->isInited()) {
        CGFloat w = self.view.bounds.size.width;
        CGFloat h = self.view.bounds.size.height;
        _pixW = (int)(w * viewScale);
        _pixH = (int)(h * viewScale);

        // Bind the MGLKView's drawable so _app->init() has a valid GL context+FBO
        [MGLContext setCurrentContext:self.mglkView.context];
        [self.mglkView bindDrawable];

        // Initialize GLES2 shader pipeline and software matrix stack
        glInit();

        _app->init(*_context);
        _app->setSize(_pixW, _pixH);
        NSLog(@"[VC] app init %dx%d  defaultFBO=%u", _pixW, _pixH,
              self.mglkView.defaultOpenGLFrameBufferID);
    }

    [self startAnimation];
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];

    // Keep contentScaleFactor in sync with viewScale (native Retina scale)
    if (self.mglkView.contentScaleFactor != viewScale)
        self.mglkView.contentScaleFactor = viewScale;

    if (_app && _app->isInited()) {
        CGFloat w = self.view.bounds.size.width;
        CGFloat h = self.view.bounds.size.height;
        int pixW = (int)(w * viewScale);
        int pixH = (int)(h * viewScale);
        if (pixW != _pixW || pixH != _pixH) {
            _pixW = pixW;
            _pixH = pixH;
            _app->setSize(_pixW, _pixH);
        }
    }
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self stopAnimation];
    [super viewWillDisappear:animated];
}

- (void)dealloc
{
    [self stopAnimation];

    delete _app;      _app      = nullptr;
    delete _platform; _platform = nullptr;
    delete[] _touchMap;

    if ([MGLContext currentContext] == self.mglkView.context)
        [MGLContext setCurrentContext:nil];

    [_keyboardView release];

    self.mglkView = nil;

    [super dealloc];
}

- (void)didReceiveMemoryWarning { [super didReceiveMemoryWarning]; }

// ─── Orientation ─────────────────────────────────────────────────────────────

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}
- (BOOL)shouldAutorotate { return YES; }

// ─── Fullscreen (hide status bar, home indicator) ────────────────────────────

- (BOOL)prefersStatusBarHidden { return YES; }
- (BOOL)prefersHomeIndicatorAutoHidden { return YES; }
- (UIRectEdge)preferredScreenEdgesDeferringSystemGestures { return UIRectEdgeAll; }

// ─── Animation / render loop ─────────────────────────────────────────────────

- (NSInteger)animationFrameInterval { return animationFrameInterval; }

- (void)setAnimationFrameInterval:(NSInteger)frameInterval
{
    if (frameInterval >= 1) {
        animationFrameInterval = frameInterval;
        if (animating) { [self stopAnimation]; [self startAnimation]; }
    }
}

- (void)startAnimation
{
    if (!animating) {
        CADisplayLink *dl = [[UIScreen mainScreen]
            displayLinkWithTarget:self selector:@selector(drawFrame)];
        dl.preferredFramesPerSecond = 60;
        [dl addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
        self.displayLink = dl;
        animating = YES;
    }
}

- (void)stopAnimation
{
    if (animating) {
        [self.displayLink invalidate];
        self.displayLink = nil;
        animating = NO;
    }
}

// ─── Alpha fix ────────────────────────────────────────────────────────────────
//
// The game renders with alpha=0 on many fragments.  MetalANGLE composites
// those transparent pixels against the UIKit background.
//
// Fix: draw a fullscreen quad using glBlendFuncSeparate(ZERO,ONE,ONE,ZERO).
//   New_RGB = 0*src + 1*dst = dst   (framebuffer RGB untouched)
//   New_A   = 1*src + 0*dst = 1.0   (source alpha = 1.0)
//
- (void)_fixAlphaToOne
{
    // GLES2: use a dedicated mini-shader to write alpha=1 without touching RGB.
    // This avoids any dependency on the game's shader state or matrix stack.
    static const char* kAlphaVS =
        "precision highp float;\n"
        "attribute vec2 a_pos;\n"
        "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";
    static const char* kAlphaFS =
        "precision mediump float;\n"
        "void main() { gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0); }\n";

    static GLuint sAlphaProg = 0;
    static GLint  sAlphaPosLoc = -1;

    // One-time compile
    if (sAlphaProg == 0) {
        GLuint vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vs, 1, &kAlphaVS, nullptr);
        glCompileShader(vs);

        GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fs, 1, &kAlphaFS, nullptr);
        glCompileShader(fs);

        sAlphaProg = glCreateProgram();
        glAttachShader(sAlphaProg, vs);
        glAttachShader(sAlphaProg, fs);
        glBindAttribLocation(sAlphaProg, 0, "a_pos");
        glLinkProgram(sAlphaProg);

        sAlphaPosLoc = glGetAttribLocation(sAlphaProg, "a_pos");

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    // Save current program
    GLint prevProg = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProg);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ZERO);

    glUseProgram(sAlphaProg);

    static const GLfloat kQuad[] = { -1,-1,  1,-1,  1,1,  -1,-1,  1,1,  -1,1 };
    glEnableVertexAttribArray(sAlphaPosLoc);
    glVertexAttribPointer(sAlphaPosLoc, 2, GL_FLOAT, GL_FALSE, 0, kQuad);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(sAlphaPosLoc);

    // Restore state
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(prevProg);

    // Re-enable the game's vertex attrib arrays (positions 0,1,2)
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
}

// ─── Render frame ─────────────────────────────────────────────────────────────

- (void)drawFrame
{
    if (!_app || !_app->isInited()) return;
    [self.mglkView display];
}

// ─── MGLKView delegate ────────────────────────────────────────────────────────
//
// Called by [mglkView display].  MGLKView has already:
//   1. Set the GL context current
//   2. Bound its default FBO (backed by the Metal drawable)
// After this method returns, MGLKView presents the FBO content.
//
- (void)mglkView:(MGLKView *)view drawInRect:(CGRect)rect
{
    if (!_app || !_app->isInited()) return;

    _app->update();

    // Rebind MGLKView's FBO — the game may have left a different FBO bound
    [view bindDrawable];

    // Fix alpha: the game renders with alpha=0 on many fragments
    [self _fixAlphaToOne];
}

- (void)enteredBackground
{
#ifdef APPLE_DEMO_PROMOTION
    if (_app) ((Minecraft *)_app)->leaveGame();
#endif
}

- (void)setAudioEnabled:(BOOL)status
{
    if (status) _app->audioEngineOn();
    else        _app->audioEngineOff();
}

// ─── Touch handling ──────────────────────────────────────────────────────────

- (void)_resetAllPointers {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i) _touchMap[i] = 0;
}
- (int)_getIndexForTouch:(UITouch *)touch {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i)
        if (_touchMap[i] == touch) return i;
    return -1;
}
- (int)_startTrackingTouch:(UITouch *)touch {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i)
        if (_touchMap[i] == 0) { _touchMap[i] = touch; return i; }
    return -1;
}
- (int)_stopTrackingTouch:(UITouch *)touch {
    for (int i = 0; i < Multitouch::MAX_POINTERS; ++i)
        if (_touchMap[i] == touch) { _touchMap[i] = 0; return i; }
    return -1;
}
- (CGPoint)_gameCoordForTouch:(UITouch *)touch {
    CGPoint p = [touch locationInView:self.view];
    p.x *= viewScale; p.y *= viewScale;
    return p;
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *t in touches) {
        int pid = [self _startTrackingTouch:t];
        if (pid >= 0) {
            CGPoint p = [self _gameCoordForTouch:t];
            Mouse::feed(1, 1, p.x, p.y);
            Multitouch::feed(1, 1, p.x, p.y, pid);
        }
    }
}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *t in touches) {
        int pid = [self _getIndexForTouch:t];
        if (pid >= 0) {
            CGPoint p = [self _gameCoordForTouch:t];
            Mouse::feed(0, 0, p.x, p.y);
            Multitouch::feed(0, 0, p.x, p.y, pid);
        }
    }
}
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *t in touches) {
        int pid = [self _stopTrackingTouch:t];
        if (pid >= 0) {
            CGPoint p = [self _gameCoordForTouch:t];
            Mouse::feed(1, 0, p.x, p.y);
            Multitouch::feed(1, 0, p.x, p.y, pid);
        }
    }
}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event {
    [self touchesEnded:touches withEvent:event];
}

// ─── Dialogs ─────────────────────────────────────────────────────────────────

- (BaseDialogController *)dialog { return _dialog; }

- (void)initDialog {
    _dialogResultStatus = -1;
    _dialogResultStrings.clear();
    [_dialog setListener:self];
}

- (void)closeDialog {
    if (_dialog) {
        _dialogResultStatus  = [_dialog getUserInputStatus];
        _dialogResultStrings = [_dialog getUserInput];
    }
    [_dialog dismissViewControllerAnimated:YES completion:nil];
    [_dialog release];
    _dialog = nil;
}

- (void)showDialog_CreateWorld {
    if (!_dialog) {
        _dialog = [[CreateNewWorldViewController alloc]
            initWithNibName:nil bundle:nil];
        _dialog.modalPresentationStyle = UIModalPresentationFullScreen;
        [self presentViewController:_dialog animated:YES completion:nil];
        [self initDialog];
    }
}

- (void)showDialog_MainMenuOptions {
    if (!_dialog) {
        UINavigationController *nav = [[UINavigationController alloc]
            initWithRootViewController:self.appSettingsViewController];
        self.appSettingsViewController.showDoneButton = YES;
        [self.appSettingsViewController setShowCreditsFooter:NO];
        [self presentViewController:nav animated:YES completion:nil];
#ifdef APPLE_DEMO_PROMOTION
        [appSettingsViewController setEnabled:NO
            forKey:[NSString stringWithUTF8String:OptionStrings::Multiplayer_ServerVisible]];
        [appSettingsViewController setEnabled:NO
            forKey:[NSString stringWithUTF8String:OptionStrings::Multiplayer_Username]];
#endif
        [self initDialog];
        [nav release];
    }
}

- (void)showDialog_RenameMPWorld {
    if (!_dialog) {
        BOOL   isIpad    = (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad);
        CGRect screen    = [[UIScreen mainScreen] bounds];
        BOOL   isIphone5 = (MAX(screen.size.width, screen.size.height) == 568);
        NSString *xib    = isIpad ? @"RenameMPWorld_ipad"
                         : (isIphone5 ? @"RenameMPWorld_iphone5" : @"RenameMPWorld_iphone");
        _dialog = [[RenameMPWorldViewController alloc]
            initWithNibName:xib bundle:[NSBundle mainBundle]];
        [self presentViewController:_dialog animated:YES completion:nil];
        [self initDialog];
    }
}

- (int)getUserInputStatus { return _dialogResultStatus; }
- (std::vector<std::string>)getUserInput { return _dialogResultStrings; }

// ─── IASK delegate ───────────────────────────────────────────────────────────

static NSString *DefaultUsername = @"Stevie";

- (void)settingsViewControllerDidEnd:(IASKAppSettingsViewController *)sender {
    [self dismissViewControllerAnimated:YES completion:nil];
    NSString *key = [NSString stringWithUTF8String:OptionStrings::Multiplayer_Username];
    if ([[NSUserDefaults standardUserDefaults] objectForKey:key] == nil ||
        [[[NSUserDefaults standardUserDefaults] objectForKey:key] isEqualToString:@""])
        [[NSUserDefaults standardUserDefaults] setObject:DefaultUsername forKey:key];
    _dialogResultStatus = 1;
}

- (void)settingToggled:(NSNotification *)notification {
    NSString *name          = [notification object];
    NSString *keyLowQuality = [NSString stringWithUTF8String:OptionStrings::Graphics_LowQuality];
    NSString *keyFancy      = [NSString stringWithUTF8String:OptionStrings::Graphics_Fancy];
    if ([name isEqualToString:keyLowQuality]) {
        if (![[[notification userInfo] objectForKey:name] boolValue])
            [appSettingsViewController getSwitch:keyFancy].on = NO;
    }
    if ([name isEqualToString:keyFancy]) {
        if (![[[notification userInfo] objectForKey:name] boolValue])
            [appSettingsViewController getSwitch:keyLowQuality].on = NO;
    }
}

// ─── Defaults ────────────────────────────────────────────────────────────────

+ (void)initialize {
    if ([self class] == [minecraftpeViewController class]) {
        NSDictionary *d = [NSDictionary dictionaryWithObjectsAndKeys:
            DefaultUsername,                [NSString stringWithUTF8String:OptionStrings::Multiplayer_Username],
            [NSNumber numberWithBool:YES],  [NSString stringWithUTF8String:OptionStrings::Multiplayer_ServerVisible],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Graphics_Fancy],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Graphics_LowQuality],
            [NSNumber numberWithFloat:0.5f],[NSString stringWithUTF8String:OptionStrings::Controls_Sensitivity],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Controls_InvertMouse],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Controls_IsLefthanded],
            [NSNumber numberWithBool:NO],   [NSString stringWithUTF8String:OptionStrings::Controls_UseTouchJoypad],
            @"2",                           [NSString stringWithUTF8String:OptionStrings::Game_DifficultyLevel],
            nil];
        [[NSUserDefaults standardUserDefaults] registerDefaults:d];
    }
}

// ─── Keyboard ────────────────────────────────────────────────────────────────

- (void)showKeyboard {
    for (UIView *v in self.view.subviews)
        if ([v isKindOfClass:[ShowKeyboardView class]])
            { [(ShowKeyboardView *)v showKeyboard]; return; }
    [self.view insertSubview:_keyboardView atIndex:0];
    [_keyboardView showKeyboard];
}

- (void)hideKeyboard {
    for (UIView *v in self.view.subviews)
        if ([v isKindOfClass:[ShowKeyboardView class]]) {
            [(ShowKeyboardView *)v hideKeyboard];
            [(ShowKeyboardView *)v removeFromSuperview];
        }
}

@end