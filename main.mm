#import <Cocoa/Cocoa.h>

#define FrameRate 60
#define W 1280
#define H  720

#import "Metal.h"

class App {
    
    private:
    
        dispatch_source_t timer;
    
        Metal *metal = nullptr;
    
        unsigned int *buffer = nullptr;
    
        void initialize() {
            
            this->buffer = new unsigned int[W*H];
            
            this->metal = new Metal();
            
            this->timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_queue_create("setInterval",0));
            dispatch_source_set_timer(this->timer,dispatch_time(0,0),(1.0/FrameRate)*1000000000,0);
            dispatch_source_set_event_handler(this->timer,^(){
                
                this->metal->render(this->buffer);
                
            });
            if(this->timer) dispatch_resume(this->timer);
        }
    
    public:
    
        App() {
            this->initialize();
        }
    
        ~App() {
            
            if(this->timer) {
                dispatch_source_cancel(this->timer);
                this->timer = nullptr;
            }
            
            usleep(100000);
            
            if(this->metal) delete this->metal;
            if(this->buffer) delete[] this->buffer;
        }
};

@interface AppDelegate:NSObject<NSApplicationDelegate> {
    App *m;
}
@end

@implementation AppDelegate
-(void)applicationDidFinishLaunching:(NSNotification*)aNotification {
    m = new App();
}
-(void)applicationWillTerminate:(NSNotification *)aNotification {
    if(m) delete m;
}
@end

int main (int argc, const char * argv[]) {
    @autoreleasepool {
        srand(CFAbsoluteTimeGetCurrent());
        srandom(CFAbsoluteTimeGetCurrent());
        id app = [NSApplication sharedApplication];
        id delegat = [AppDelegate alloc];
        [app setDelegate:delegat];
        
        id menu = [[NSMenu alloc] init];
        id rootMenuItem = [[NSMenuItem alloc] init];
        [menu addItem:rootMenuItem];
        id appMenu = [[NSMenu alloc] init];
        id quitMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quit" action:@selector(terminate:) keyEquivalent:@"q"];
        [appMenu addItem:quitMenuItem];
        [rootMenuItem setSubmenu:appMenu];
        [NSApp setMainMenu:menu];
       
        [app run];
        return 0;
    }
}
