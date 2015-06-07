#ifndef shadertoy_test_NSGLView_h
#define shadertoy_test_NSGLView_h

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CVDisplayLink.h>

@interface GLView : NSOpenGLView {
	CVDisplayLinkRef displayLink;

}

@end

#endif
