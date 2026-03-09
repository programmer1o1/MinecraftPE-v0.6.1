//
//  EAGLView.h
//  OpenGLES_iPhone
//
//  Created by mmalc Crawford on 11/18/10.
//  Copyright 2010 Apple Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <MetalANGLE/MGLKit.h>

// This class wraps MGLLayer (MetalANGLE) into a convenient UIView subclass.
// The view content is basically a GLES surface rendered via Metal under the hood.
@interface EAGLView : UIView {
    // The pixel dimensions of the MGLLayer.
    GLint framebufferWidth;
    GLint framebufferHeight;

    // The OpenGL ES names for the framebuffer and renderbuffers.
    GLuint defaultFramebuffer;
    GLuint colorRenderbuffer, _depthRenderBuffer;

    @public
    GLfloat viewScale;
}

@property (nonatomic, retain) MGLContext *context;

- (void)setFramebuffer;
- (BOOL)presentFramebuffer;

@end
