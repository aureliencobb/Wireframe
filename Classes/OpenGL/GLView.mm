#import "GLView.h"
#import <OpenGLES/ES2/gl.h> // <-- for GL_RENDERBUFFER only

#if GL_1_1
const bool ForceES1 = true;
#else
const bool ForceES1 = false;
#endif

@implementation GLView

+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

- (id) initWithFrame: (CGRect) frame
{
    if (self = [super initWithFrame:frame])
    {
        CAEAGLLayer* eaglLayer = (CAEAGLLayer*) self.layer;
        eaglLayer.opaque = YES;

        EAGLRenderingAPI api = kEAGLRenderingAPIOpenGLES2;
        m_context = [[EAGLContext alloc] initWithAPI:api];
        
        if (!m_context || ForceES1) {
            api = kEAGLRenderingAPIOpenGLES1;
            m_context = [[EAGLContext alloc] initWithAPI:api];
        }
        
        if (!m_context || ![EAGLContext setCurrentContext:m_context]) {
            [self release];
            return nil;
        }
        

        if (api == kEAGLRenderingAPIOpenGLES1) {
            NSLog(@"Using OpenGL ES 1.1");
            m_renderingEngine = ES1::CreateRenderingEngine();
        } else {
            NSLog(@"Using OpenGL ES 2.0");
            m_renderingEngine = ES2::CreateRenderingEngine();
        }
        
        m_resourceManager = CreateResourceManager();
        m_applicationEngine = CreateApplicationEngine(m_renderingEngine, m_resourceManager);

        m_scale = [[UIScreen mainScreen] scale];
        
        eaglLayer.contentsScale = m_scale;
        
        [m_context renderbufferStorage:GL_RENDERBUFFER fromDrawable: eaglLayer];
                
        int width = CGRectGetWidth(frame) * m_scale;
        int height = CGRectGetHeight(frame) * m_scale;
        m_applicationEngine->Initialize(width, height);
        
        [self drawView: nil];
        m_timestamp = CACurrentMediaTime();
        
        CADisplayLink* displayLink;
        displayLink = [CADisplayLink displayLinkWithTarget:self
                                     selector:@selector(drawView:)];
        
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop]
                     forMode:NSDefaultRunLoopMode];
    }
    return self;
}

- (void) drawView: (CADisplayLink*) displayLink
{
    if (displayLink != nil) {
        float elapsedSeconds = displayLink.timestamp - m_timestamp;
        m_timestamp = displayLink.timestamp;
        m_applicationEngine->UpdateAnimation(elapsedSeconds);
    }
    
    m_applicationEngine->Render();
    [m_context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void) touchesBegan: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint location  = [touch locationInView: self];
    m_applicationEngine->OnFingerDown(ivec2(location.x * m_scale, location.y * m_scale));
}

- (void) touchesEnded: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint location  = [touch locationInView: self];
    m_applicationEngine->OnFingerUp(ivec2(location.x * m_scale, location.y * m_scale));
}

- (void) touchesMoved: (NSSet*) touches withEvent: (UIEvent*) event
{
    UITouch* touch = [touches anyObject];
    CGPoint previous  = [touch previousLocationInView: self];
    CGPoint current = [touch locationInView: self];
    m_applicationEngine->OnFingerMove(ivec2(previous.x * m_scale, previous.y * m_scale),
                                      ivec2(current.x * m_scale, current.y * m_scale));
}

@end