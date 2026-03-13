//
//  EAGLView.m
//  minecraftpe
//

#import "EAGLView.h"

@implementation EAGLView

// Use MGLLayer as the backing layer — the MetalANGLE equivalent of CAEAGLLayer.
+ (Class)layerClass
{
    return [MGLLayer class];
}

- (instancetype)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self) {
        self.contentScaleFactor = [UIScreen mainScreen].scale;
        self.multipleTouchEnabled = YES;
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        self.contentScaleFactor = [UIScreen mainScreen].scale;
        self.multipleTouchEnabled = YES;
    }
    return self;
}

- (MGLLayer *)glLayer
{
    return (MGLLayer *)self.layer;
}

@end
