//
//  CreateNewWorldViewController.h
//  minecraftpe
//
//  Created by rhino on 10/20/11.
//  Copyright 2011 Mojang AB. All rights reserved.
//

#import "BaseDialogController.h"

@interface CreateNewWorldViewController : BaseDialogController<UITextFieldDelegate>
{
    UILabel* _labelName;
    UILabel* _labelSeed;
    UILabel* _labelSeedHint;

    UILabel* _labelGameMode;
    UILabel* _labelGameModeDesc;

    UITextField* _textName;
    UITextField* _textSeed;

    UIButton* _btnGameMode;

    int _currentGameModeId;
    int _currentWorldType;

    UILabel*  _labelWorldType;
    UIButton* _btnWorldType;

    UIView*   _contentContainer;
}

- (void) UpdateGameModeDesc;
- (void) UpdateWorldTypeDesc;

- (IBAction)DismissKeyboard;

- (IBAction)Create;
- (IBAction)Cancel;
- (IBAction)ToggleGameMode;
- (IBAction)ToggleWorldType;

@end
