//
//  CreateNewWorldViewController.m
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang. All rights reserved.
//

#import "CreateNewWorldViewController.h"
#import <QuartzCore/QuartzCore.h>

static const int GameMode_Creative = 0;
static const int GameMode_Survival = 1;
static const char* getGameModeName(int mode) {
    if (mode == GameMode_Survival) return "survival";
    return "creative";
}

static const int WorldType_Old      = 0;
static const int WorldType_Infinite = 1;

@implementation CreateNewWorldViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        _currentGameModeId = GameMode_Creative;
        _currentWorldType  = WorldType_Old;
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (void) UpdateGameModeDesc
{
    if (_currentGameModeId == GameMode_Creative) {
        [_labelGameModeDesc setText:@"Unlimited resources, flying"];

        UIImage *img = [UIImage imageNamed:@"creative_0_3.png"];
        [_btnGameMode setImage:img forState:UIControlStateNormal];
        UIImage *img2 = [UIImage imageNamed:@"creative_1_3.png"];
        [_btnGameMode setImage:img2 forState:UIControlStateHighlighted];
    }
    if (_currentGameModeId == GameMode_Survival) {
        [_labelGameModeDesc setText:@"Mobs, health and gather resources"];

        UIImage *img = [UIImage imageNamed:@"survival_0_3.png"];
        [_btnGameMode setImage:img forState:UIControlStateNormal];
        UIImage *img2 = [UIImage imageNamed:@"survival_1_3.png"];
        [_btnGameMode setImage:img2 forState:UIControlStateHighlighted];
    }
}

- (void) UpdateWorldTypeDesc
{
    if (_currentWorldType == WorldType_Old) {
        [_btnWorldType setTitle:@"Old" forState:UIControlStateNormal];
        [_labelWorldType setText:@"World Type: Old (256x256)"];
    } else {
        [_btnWorldType setTitle:@"Infinite" forState:UIControlStateNormal];
        [_labelWorldType setText:@"World Type: Infinite"];
    }
}

#pragma mark - View lifecycle

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField setUserInteractionEnabled:YES];
    [self DismissKeyboard];
    return YES;
}

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string
{
    if (textField == _textName) {
        NSUInteger newLength = [textField.text length] + [string length] - range.length;
        if (newLength > 18)
            return NO;
    }

    int length = (int)[string length];
    for (int i = 0; i < length; ++i) {
        unichar ch = [string characterAtIndex:i];
        if (ch >= 128)
            return NO;
    }
    return YES;
}

// Helper: create a Minecraft-styled button with dark background and white border
- (UIButton*)makeStyledButton:(CGRect)frame font:(UIFont*)font
{
    UIButton *btn = [UIButton buttonWithType:UIButtonTypeCustom];
    btn.frame = frame;
    btn.titleLabel.font = font;
    [btn setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    btn.backgroundColor = [UIColor colorWithWhite:0.2 alpha:0.9];
    btn.layer.borderColor = [[UIColor grayColor] CGColor];
    btn.layer.borderWidth = 2.0f;
    return btn;
}

// Helper: create a styled label
- (UILabel*)makeLabel:(CGRect)frame font:(UIFont*)font text:(NSString*)text
{
    UILabel *lbl = [[UILabel alloc] initWithFrame:frame];
    lbl.font = font;
    lbl.textColor = [UIColor whiteColor];
    lbl.backgroundColor = [UIColor clearColor];
    lbl.shadowColor = [UIColor colorWithWhite:0.33 alpha:1.0];
    lbl.shadowOffset = CGSizeMake(1, 1);
    lbl.text = text;
    return lbl;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Dirt-pattern tiled background
    self.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"bg64.png"]];

    // Use a centered container so layout works on any screen size
    _contentContainer = [[UIView alloc] initWithFrame:CGRectZero];
    _contentContainer.backgroundColor = [UIColor clearColor];
    [self.view addSubview:_contentContainer];

    // Tap-to-dismiss keyboard on background
    UIControl *bgControl = [[UIControl alloc] initWithFrame:CGRectMake(0, 0, 2000, 2000)];
    [bgControl addTarget:self action:@selector(DismissKeyboard) forControlEvents:UIControlEventTouchUpInside];
    [_contentContainer insertSubview:bgControl atIndex:0];

    // --- Fonts ---
    UIFont *fontLarge = [UIFont fontWithName:@"minecraft" size:16];
    UIFont *fontSmall = [UIFont fontWithName:@"minecraft" size:14];
    if (!fontLarge) fontLarge = [UIFont systemFontOfSize:16];
    if (!fontSmall) fontSmall = [UIFont systemFontOfSize:14];

    // --- Layout constants ---
    // Wider canvas so buttons don't overlap the title image
    const CGFloat contentW = 568.0f;
    const CGFloat contentH = 320.0f;
    const CGFloat btnW     = 132.0f;
    const CGFloat btnH     = 52.0f;
    const CGFloat gameBtnW = 191.0f;
    const CGFloat gameBtnH = 52.0f;
    const CGFloat fieldW   = 240.0f;
    const CGFloat fieldH   = 32.0f;
    const CGFloat centerX  = contentW / 2.0f;
    const CGFloat pad      = 6.0f;

    // --- Row 0: Header bar ---
    // Cancel (left) | Title Image (fills gap) | Create (right)
    CGFloat headerY = 0;

    // Cancel button (top-left)
    UIButton *btnCancel = [UIButton buttonWithType:UIButtonTypeCustom];
    btnCancel.frame = CGRectMake(0, headerY, btnW, btnH);
    [btnCancel setImage:[UIImage imageNamed:@"cancel_0_3.png"] forState:UIControlStateNormal];
    [btnCancel setImage:[UIImage imageNamed:@"cancel_1_3.png"] forState:UIControlStateHighlighted];
    [btnCancel addTarget:self action:@selector(Cancel) forControlEvents:UIControlEventTouchUpInside];
    [_contentContainer addSubview:btnCancel];

    // Create button (top-right)
    UIButton *btnCreate = [UIButton buttonWithType:UIButtonTypeCustom];
    btnCreate.frame = CGRectMake(contentW - btnW, headerY, btnW, btnH);
    [btnCreate setImage:[UIImage imageNamed:@"create_0_3.png"] forState:UIControlStateNormal];
    [btnCreate setImage:[UIImage imageNamed:@"create_1_3.png"] forState:UIControlStateHighlighted];
    [btnCreate addTarget:self action:@selector(Create) forControlEvents:UIControlEventTouchUpInside];
    [_contentContainer addSubview:btnCreate];

    // Title image (fits in the gap between cancel and create)
    UIImage *titleImg = [UIImage imageNamed:@"worldname_iphone5_3.png"];
    if (!titleImg) titleImg = [UIImage imageNamed:@"worldname_iphone_3.png"];
    if (titleImg) {
        CGFloat gapX = btnW;
        CGFloat gapW = contentW - 2 * btnW;
        UIImageView *titleView = [[UIImageView alloc] initWithFrame:
            CGRectMake(gapX, headerY, gapW, btnH)];
        titleView.image = titleImg;
        titleView.contentMode = UIViewContentModeScaleAspectFit;
        [_contentContainer addSubview:titleView];
    }

    // --- Row 1: World Name text field ---
    CGFloat nameY = btnH + pad + 4;

    _textName = [[UITextField alloc] initWithFrame:
        CGRectMake(centerX - fieldW/2, nameY, fieldW, fieldH)];
    _textName.font = fontLarge;
    _textName.textColor = [UIColor whiteColor];
    _textName.backgroundColor = [UIColor blackColor];
    _textName.layer.borderColor = [[UIColor whiteColor] CGColor];
    _textName.layer.borderWidth = 2.0f;
    _textName.autocorrectionType = UITextAutocorrectionTypeNo;
    _textName.keyboardType = UIKeyboardTypeASCIICapable;
    _textName.returnKeyType = UIReturnKeyDone;
    _textName.delegate = self;
    UIView *namePad = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 6, fieldH)];
    _textName.leftView = namePad;
    _textName.leftViewMode = UITextFieldViewModeAlways;
    [_contentContainer addSubview:_textName];

    // --- Row 2: Game Mode button + description ---
    CGFloat gameModeY = nameY + fieldH + pad + 2;

    _btnGameMode = [UIButton buttonWithType:UIButtonTypeCustom];
    _btnGameMode.frame = CGRectMake(centerX - gameBtnW/2, gameModeY, gameBtnW, gameBtnH);
    [_btnGameMode addTarget:self action:@selector(ToggleGameMode)
        forControlEvents:UIControlEventTouchUpInside];
    [_contentContainer addSubview:_btnGameMode];

    _labelGameModeDesc = [self makeLabel:
        CGRectMake(centerX - fieldW/2, gameModeY + gameBtnH + 2, fieldW, 28)
        font:fontSmall text:@""];
    _labelGameModeDesc.textAlignment = NSTextAlignmentCenter;
    _labelGameModeDesc.numberOfLines = 2;
    [_contentContainer addSubview:_labelGameModeDesc];
    [self UpdateGameModeDesc];

    // --- Row 3: World Type label + button ---
    CGFloat worldTypeY = gameModeY + gameBtnH + 30;

    _labelWorldType = [self makeLabel:
        CGRectMake(centerX - fieldW/2, worldTypeY, fieldW, 18)
        font:fontSmall text:@""];
    _labelWorldType.textAlignment = NSTextAlignmentCenter;
    [_contentContainer addSubview:_labelWorldType];

    _btnWorldType = [self makeStyledButton:
        CGRectMake(centerX - 70, worldTypeY + 18, 140, 28) font:fontSmall];
    [_btnWorldType addTarget:self action:@selector(ToggleWorldType)
        forControlEvents:UIControlEventTouchUpInside];
    [_contentContainer addSubview:_btnWorldType];
    [self UpdateWorldTypeDesc];

    // --- Row 4: Seed section ---
    CGFloat seedLabelY = worldTypeY + 50;

    _labelSeed = [self makeLabel:
        CGRectMake(centerX - fieldW/2, seedLabelY, fieldW, 18)
        font:fontSmall text:@"Seed for the World Generator"];
    _labelSeed.textAlignment = NSTextAlignmentCenter;
    _labelSeed.textColor = [UIColor colorWithWhite:0.67 alpha:1.0];
    [_contentContainer addSubview:_labelSeed];

    _textSeed = [[UITextField alloc] initWithFrame:
        CGRectMake(centerX - fieldW/2, seedLabelY + 20, fieldW, fieldH)];
    _textSeed.font = fontLarge;
    _textSeed.textColor = [UIColor whiteColor];
    _textSeed.backgroundColor = [UIColor blackColor];
    _textSeed.layer.borderColor = [[UIColor whiteColor] CGColor];
    _textSeed.layer.borderWidth = 2.0f;
    _textSeed.autocorrectionType = UITextAutocorrectionTypeNo;
    _textSeed.keyboardType = UIKeyboardTypeASCIICapable;
    _textSeed.returnKeyType = UIReturnKeyDone;
    _textSeed.delegate = self;
    UIView *seedPad = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 6, fieldH)];
    _textSeed.leftView = seedPad;
    _textSeed.leftViewMode = UITextFieldViewModeAlways;
    [_contentContainer addSubview:_textSeed];

    _labelSeedHint = [self makeLabel:
        CGRectMake(centerX - fieldW/2, seedLabelY + 20 + fieldH + 2, fieldW, 18)
        font:fontSmall text:@"Leave blank for random seed"];
    _labelSeedHint.textAlignment = NSTextAlignmentCenter;
    _labelSeedHint.textColor = [UIColor colorWithWhite:0.67 alpha:1.0];
    [_contentContainer addSubview:_labelSeedHint];

    // Set the content container to the design size
    _contentContainer.frame = CGRectMake(0, 0, contentW, contentH);
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];

    if (!_contentContainer) return;

    CGFloat viewW = self.view.bounds.size.width;
    CGFloat viewH = self.view.bounds.size.height;
    CGFloat contentW = _contentContainer.bounds.size.width;
    CGFloat contentH = _contentContainer.bounds.size.height;

    // Scale to fit the screen while maintaining aspect ratio
    CGFloat scaleX = viewW / contentW;
    CGFloat scaleY = viewH / contentH;
    CGFloat scale  = MIN(scaleX, scaleY);

    _contentContainer.transform = CGAffineTransformMakeScale(scale, scale);
    _contentContainer.center = CGPointMake(viewW / 2.0f, viewH / 2.0f);
}

- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return UIInterfaceOrientationIsLandscape(interfaceOrientation);
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

- (IBAction)Create {
    [self addString: [[_textName text] UTF8String]];
    [self addString: [[_textSeed text] UTF8String]];
    [self addString: getGameModeName(_currentGameModeId)];
    [self addString: (_currentWorldType == WorldType_Infinite) ? "infinite" : "old"];
    [self closeOk];
}

- (IBAction)Cancel {
    [self closeCancel];
}

- (IBAction)ToggleGameMode {
    const int NumGameModes = 2;
    if (++_currentGameModeId >= NumGameModes)
        _currentGameModeId = 0;

    [self UpdateGameModeDesc];
}

- (IBAction)ToggleWorldType {
    _currentWorldType = (_currentWorldType == WorldType_Old) ? WorldType_Infinite : WorldType_Old;
    [self UpdateWorldTypeDesc];
}

- (IBAction)DismissKeyboard {
    [_textName resignFirstResponder];
    [_textSeed resignFirstResponder];
}

@end
