#import "MetalLayer.h"

@interface MetalView:NSView {
    bool click;
}
    -(id)init:(MetalLayerBase *)ml;
    -(bool)isClick;
@end
@implementation MetalView
    +(Class)layerClass { return [CAMetalLayer class]; }
    -(BOOL)wantsUpdateLayer { return YES; }
    -(void)updateLayer { [super updateLayer]; }
    -(id)init:(MetalLayerBase *)ml {
        self = [super initWithFrame:ml->frame()];
        if(self) {
            self->click = false;
            self.layer = ml->layer();
            self.wantsLayer = YES;
        }
        return self;
    }
    -(void)mouseDown:(NSEvent *)e {
        click = true;
        [super mouseDown:e];
    }
    -(void)mouseUp:(NSEvent *)e {
        click = false;
        [super mouseDown:e];
    }
    -(bool)isClick {
        return click;
    }
@end

class Metal {
  
    private:
    
        bool state = false;
    
        id<MTLDevice> device;
        id<MTLTexture> texture[2];
        id<MTLCommandQueue> queue;
        id<MTLComputePipelineState> pipelineState;
        id<MTLBuffer> resolution;
        id<MTLBuffer> coeff;
        id<MTLSamplerState> samplerState;
        id<MTLArgumentEncoder> argumentEncoder;
        id<MTLBuffer> argumentEncoderBuffer;
    

        NSWindow *win;
        MetalView *metalview;
        RGBA32FloatMetalLayer *bypass;
    
        dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    
        bool isLeftMouseDown = false;
        CGSize size;
    
    public:
    
        MTLSamplerDescriptor *samplerDescriptor() {
            MTLSamplerDescriptor *samplerDescriptor = [MTLSamplerDescriptor new];
            samplerDescriptor.sAddressMode = samplerDescriptor.tAddressMode = MTLSamplerAddressModeRepeat;
            samplerDescriptor.minFilter = samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
            samplerDescriptor.normalizedCoordinates = YES;
            return samplerDescriptor;
        }
    
        Metal() {
            
            device = MTLCreateSystemDefaultDevice();
            
            
            id<MTLFunction> function = [[device newDefaultLibrary] newFunctionWithName:@"reactiondiffusion"];
            
            this->pipelineState = [device newComputePipelineStateWithFunction:function error:nil];
            
            this->queue = [device newCommandQueue];
            
            MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:W height:H mipmapped:NO];
            descriptor.usage = MTLTextureUsageShaderWrite|MTLTextureUsageShaderRead;
            
            this->texture[0] = [device newTextureWithDescriptor:descriptor];
            this->texture[1] = [device newTextureWithDescriptor:descriptor];
            
            float *data = new float[W*H*4];
            for(int i=0; i<W*H; i++) {
                data[i*4+0] = data[i*4+1] = data[i*4+2] = 0;
                data[i*4+3] = 1;
            }
            
            [this->texture[this->state] replaceRegion:MTLRegionMake2D(0,0,W,H) mipmapLevel:0 withBytes:data bytesPerRow:W<<4];
            
            this->samplerState = [MTLCreateSystemDefaultDevice() newSamplerStateWithDescriptor:samplerDescriptor()];
            
            this->argumentEncoder = [function newArgumentEncoderWithBufferIndex:0];
            this->argumentEncoderBuffer = [device newBufferWithLength:sizeof(float)*[argumentEncoder encodedLength] options:MTLResourceOptionCPUCacheModeDefault];
            [this->argumentEncoder setArgumentBuffer:argumentEncoderBuffer offset:0];
            
            this->resolution = MetalUtils::create(this->device,this->argumentEncoder,sizeof(float)*2,0,0);
            MetalUtils::setFloatValue(this->resolution,W,H);
            
            [this->argumentEncoder setBuffer:MetalUniform::$()->mouse() offset:0 atIndex:1];
            
            this->coeff = MetalUtils::create(this->device,this->argumentEncoder,sizeof(float)*4,0,2);
            MetalUtils::setFloatValue(this->coeff,1.0,0.5,0.005,0.03);
            
            this->win = [[NSWindow alloc] initWithContentRect:CGRectMake(0,0,W,H) styleMask:1 backing:NSBackingStoreBuffered defer:NO];
            
            this->bypass = new RGBA32FloatMetalLayer(@"default",@"bypass",CGRectMake(0,0,W,H),false);
            this->metalview = [[MetalView alloc] init:this->bypass];
            
            [[this->win contentView] addSubview:this->metalview];
            [this->win center];
            this->size = [[this->win contentView]frame].size;
        }
        
        void render(unsigned int *buffer) {
            
            MetalUniform::$()->update([this->win frame].origin,this->size,[this->metalview isClick]);
            
            int it = 5;
            
            while(it--) {
                id<MTLCommandBuffer> commandBuffer = this->queue.commandBuffer;
                id<MTLComputeCommandEncoder> encoder = commandBuffer.computeCommandEncoder;
                [encoder setComputePipelineState:this->pipelineState];
                [encoder setTexture:texture[this->state] atIndex:0];
                [encoder setTexture:texture[!this->state] atIndex:1];
                
                [encoder useResource:this->resolution usage:MTLResourceUsageRead];
                [encoder useResource:MetalUniform::$()->mouse() usage:MTLResourceUsageRead];
                [encoder useResource:this->coeff usage:MTLResourceUsageRead];
                
                [encoder setBuffer:this->argumentEncoderBuffer offset:0 atIndex:0];
                [encoder setSamplerState:this->samplerState atIndex:0];

                MTLSize threadGroupSize = MTLSizeMake(16,8,1);
                MTLSize threadGroups = MTLSizeMake(texture[this->state].width/threadGroupSize.width,texture[this->state].height/threadGroupSize.height,1);
                
                [encoder dispatchThreadgroups:threadGroups threadsPerThreadgroup:threadGroupSize];
                [encoder endEncoding];
                [commandBuffer commit];
                [commandBuffer waitUntilCompleted];
                
                this->state = !this->state;
            };
            
            this->bypass->texture(texture[!this->state]);
            this->bypass->update(^(id<MTLCommandBuffer> commandBuffer){
                 dispatch_semaphore_signal(this->semaphore);
            });
            
            
            
            dispatch_semaphore_wait(this->semaphore,DISPATCH_TIME_FOREVER);
            this->bypass->cleanup();
            
            dispatch_async(dispatch_get_main_queue(),^{
                this->size = [[this->win contentView]frame].size;
            });
            
            static dispatch_once_t predicate;
            dispatch_once(&predicate,^{
                dispatch_async(dispatch_get_main_queue(),^{
                    [this->win makeKeyAndOrderFront:nil];
                });
            });
        }
    
        ~Metal() {
            [this->metalview removeFromSuperview];
        }
};
