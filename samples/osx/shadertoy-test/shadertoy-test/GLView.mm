#import "GLView.h"

#include <sys/time.h>
#include "ShadertoyTestHelper.h"

@interface GLView () {
    ShadertoyTestInfo *test_info;
    ShadertoyTest *test;

    ShadertoyInputs inputs;
    ShadertoyView view;
    ShadertoyOutputs outputs;
    ShadertoyConfig config;
    ShadertoyState state;
    
    NSRect resolution;
}

- (void) drawFrame;
- (NSPoint)getMousePosition:(NSEvent *)event;

@end

// Helper functions
static double
GetTimeNow() {
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

void StopAudio(void *sound) {
}

void* InitAudio(int channels, int sample_rate, int bits_per_sample, int buffer_size) {
    return NULL;
}

@implementation GLView

- (CVReturn) getFrameForTime:(const CVTimeStamp*)outputTime
{
	[self drawFrame];
	
	return kCVReturnSuccess;
}

// This is the renderer output callback function
static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    CVReturn result = [(__bridge GLView*)displayLinkContext getFrameForTime:outputTime];
    return result;
}

- (void) awakeFromNib
{   
    NSOpenGLPixelFormatAttribute attrs[] =
	{
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersionLegacy,
		0
	};
	
	NSOpenGLPixelFormat *pf = [[NSOpenGLPixelFormat alloc] initWithAttributes:attrs];
	
	if (!pf)
	{
		NSLog(@"No OpenGL pixel format");
	}
    
    NSOpenGLContext* context = [[NSOpenGLContext alloc] initWithFormat:pf shareContext:nil];
    
    [self setPixelFormat:pf];
    
    [self setOpenGLContext:context];
}

- (void) prepareOpenGL
{
	[super prepareOpenGL];
	
	GLint swapInt = 1;
	[[self openGLContext] makeCurrentContext];
	[[self openGLContext] setValues:&swapInt forParameter:NSOpenGLCPSwapInterval];
	
	CVDisplayLinkCreateWithActiveCGDisplays(&displayLink);
	CVDisplayLinkSetOutputCallback(displayLink, &MyDisplayLinkCallback, (__bridge void *)(self));
	
	CGLContextObj cglContext = [[self openGLContext] CGLContextObj];
	CGLPixelFormatObj cglPixelFormat = [[self pixelFormat] CGLPixelFormatObj];
	CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, cglContext, cglPixelFormat);
	
	CVDisplayLinkStart(displayLink);
    
    memset(&inputs, 0, sizeof(inputs));
    memset(&view, 0, sizeof(view));
    memset(&outputs, 0, sizeof(outputs));
    memset(&config, 0, sizeof(config));
    memset(&state, 0, sizeof(state));
    
    test_info = TestInit(GetTimeNow, StopAudio, InitAudio, (__bridge void*)[self window]);
    test = NULL;
}

- (void) viewWillMoveToWindow:(NSWindow *)newWindow {
    // Setup a new tracking area when the view is added to the window.
    NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways | NSTrackingInVisibleRect | NSTrackingMouseMoved;
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options:options owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    // TODO(jose): Unfortunately it seems OSX doesn't offer us a easy way on this.
    // If someone knows any solution that is not a lookup table ping me, otherwise I will eventually do a lookup table.
    unsigned short keycode = [event keyCode];
    if (keycode < 256) {
        inputs.keyboard.toggle[keycode] = ~inputs.keyboard.toggle[keycode];
        inputs.keyboard.state[keycode] = 0xff;
    }
}

- (void)keyUp:(NSEvent *)event {
    unsigned short keycode = [event keyCode];
    if (keycode < 256) {
        inputs.keyboard.state[keycode] = 0;
    }
}

- (NSPoint)getMousePosition:(NSEvent *)event {
    NSRect rect = [[[event window] contentView] frame];
    NSPoint point = [event locationInWindow];
    point.y = rect.size.height - point.y;
    return point;
}

- (void)mouseDown:(NSEvent *)event {
    ImGuiEventMonitor(event);
    if ([event type] == NSLeftMouseDown) {
        NSPoint point = [self getMousePosition:event];
        inputs.mouse.z = (float)point.x;
        inputs.mouse.w = (float)(inputs.resolution.y - point.y);
    }
}

- (void)mouseUp:(NSEvent *)event {
    ImGuiEventMonitor(event);
    if ([event type] == NSLeftMouseUp) {
        inputs.mouse.z = (float)-fabs(inputs.mouse.z);
        inputs.mouse.w = (float)-fabs(inputs.mouse.w);
    }
}

- (void)mouseMoved:(NSEvent *)event {
    ImGuiEventMonitor(event);
    if ([NSEvent pressedMouseButtons] & 1) {
        NSPoint point = [self getMousePosition:event];
        inputs.mouse.x = (float)point.x;
        inputs.mouse.y = (float)(inputs.resolution.y - point.y);
    }
}

- (void)mouseDragged:(NSEvent *)event {
    [self mouseMoved:event];
}

- (void)scrollWheel:(NSEvent *)event {
    ImGuiEventMonitor(event);
}

- (void) reshape
{	
	[super reshape];
	
	CGLLockContext([[self openGLContext] CGLContextObj]);
	
	resolution = [self bounds];
    
	CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void)drawRect:(NSRect)rect
{
    [self drawFrame];
}

- (void) drawFrame
{	 
	[[self openGLContext] makeCurrentContext];

	CGLLockContext([[self openGLContext] CGLContextObj]);
    
    TestBegin();
    
    if (test != NULL) {
        // Global time input
        inputs.global_time = (GetTimeNow() - test->start_time) / 1000;

        // Date input
        time_t timestamp = time(NULL);
        struct tm  date;
        date = *localtime(&timestamp);
        inputs.date.x = (float)date.tm_year + 1900;
        inputs.date.y = (float)date.tm_mon;
        inputs.date.z = (float)date.tm_mday;
        inputs.date.w = (float)date.tm_sec + date.tm_min * 60 + date.tm_hour * 60 * 60;

        // Resolution
        view.max.x = (int)resolution.size.width;
        view.max.y = (int)resolution.size.height;
        inputs.resolution.x = resolution.size.width;
        inputs.resolution.y = resolution.size.height;

        // Mouse and keyboard inputs are processed within GLView methods

        // Render
        ShadertoyRender(&state, &inputs, &view, &outputs);

        // Sound output
//        if (outputs.sound_enabled) {
//            DSoundUnit *unit = (DSoundUnit*)outputs.sound_data_param;
//            DSoundPlay(unit, outputs.sound_buffer_size, (BYTE*)outputs.sound_next_buffer);
//            inputs.sound_played_samples = DSoundGetPlayedSamples(unit);
//        }
//        if (outputs.sound_should_stop) {
//            DSoundStop((DSoundUnit*)outputs.sound_data_param);
//        }
//
//        // Audio outputs
//        for (int i = 0; i < SHADERTOY_MAX_CHANNELS; ++i) {
//            if (outputs.music_enabled_channel[i]) {
//                DSoundUnit *audio = (DSoundUnit*)outputs.music_data_param[i];
//                if (!DSoundIsPlaying(audio)) {
//                    int size;
//                    BYTE *data;
//                    GetAudioInfo(i, &size, (void**)&data);
//                    DSoundPlay(audio, size, data);
//                }
//                if (state.music_enabled_channel[i]) {
//                    inputs.audio_played_samples[i] = DSoundGetPlayedSamples(audio);
//                }
//            }
//        }

        GUITestOverlay(true, test, &state, &inputs);
    }

    if (GUITestSelection(true, test_info, &test)) {
        LoadTest(test, &config, &state, &inputs, &outputs);
    }

    TestEnd();
    
	CGLFlushDrawable([[self openGLContext] CGLContextObj]);
	CGLUnlockContext([[self openGLContext] CGLContextObj]);
}

- (void) dealloc
{	
    TestShutdown();
	CVDisplayLinkStop(displayLink);
	CVDisplayLinkRelease(displayLink);
}
@end