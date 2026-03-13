//
//  EAGLView.h
//  minecraftpe
//
//  Rendering view whose backing layer is an MGLLayer (MetalANGLE's
//  CALayer subclass).  This is the direct equivalent of the original
//  EAGLView/CAEAGLLayer pattern, ported to the MetalANGLE API.
//

#import <UIKit/UIKit.h>
#import <MetalANGLE/MGLKit.h>

@interface EAGLView : UIView

@property(nonatomic, readonly) MGLLayer *glLayer;

@end
