#import <MetalKit/MetalKit.h>

namespace Plane {
    
    static const float vertexData[6][4] = {
        { -1.f,-1.f, 0.f, 1.f },
        {  1.f,-1.f, 0.f, 1.f },
        { -1.f, 1.f, 0.f, 1.f },
        {  1.f,-1.f, 0.f, 1.f },
        { -1.f, 1.f, 0.f, 1.f },
        {  1.f, 1.f, 0.f, 1.f }
    };
    
    static const float textureCoordinateData[6][2] = {
        { 0.f, 1.f },
        { 1.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 1.f },
        { 0.f, 0.f },
        { 1.f, 0.f }
    };
}

#import <Foundation/Foundation.h>

namespace MetalUtils {
    
    id<MTLBuffer> create(id<MTLDevice> device,id<MTLArgumentEncoder> encoder,int size,int offset,int index) {
        id<MTLBuffer> buff = [device newBufferWithLength:size options:MTLResourceOptionCPUCacheModeDefault];
        [encoder setBuffer:buff offset:offset atIndex:index];
        return buff;
    }
    
    id<MTLBuffer> create(id<MTLDevice> device,int size) {
        id<MTLBuffer> buff = [device newBufferWithLength:size options:MTLResourceOptionCPUCacheModeDefault];
        return buff;
    }
    
    void setFloatValue(id<MTLBuffer> buff,float a) {
        float *tmp = (float *)[buff contents];
        tmp[0] = a;
    }
    
    void setFloatValue(id<MTLBuffer> buff,float a,float aa) {
        float *tmp = (float *)[buff contents];
        tmp[0] = a;
        tmp[1] = aa;
    }
    
    void setFloatValue(id<MTLBuffer> buff,float a,float aa,float aaa,float aaaa) {
        float *tmp = (float *)[buff contents];
        tmp[0] = a;
        tmp[1] = aa;
        tmp[2] = aaa;
        tmp[3] = aaaa;
    }
    
};

class MetalUniform {
    
    private:
    
        id<MTLBuffer> timeBuffer;
        id<MTLBuffer> mouseBuffer;

        double starttime;
    
        MetalUniform() {
            
            this->starttime = CFAbsoluteTimeGetCurrent();
            
            id<MTLDevice> device = MTLCreateSystemDefaultDevice();
            
            this->timeBuffer  = MetalUtils::create(device,1);
            this->mouseBuffer = MetalUtils::create(device,4);
        }
    
        MetalUniform(const MetalUniform &me) {}
        virtual ~MetalUniform() {}
    
    public:
    
        static MetalUniform *$() {
            static MetalUniform *instance=nullptr;
            if(instance==nullptr) {
                instance = new MetalUniform();
            }
            return instance;
        }
    
        id<MTLBuffer> mouse() { return this->mouseBuffer; }
        id<MTLBuffer> time() { return this->timeBuffer; }
    
        void update(CGPoint origin,CGSize size,bool isClick) {
            
            
            MetalUtils::setFloatValue(this->timeBuffer,CFAbsoluteTimeGetCurrent()-this->starttime);
            
            double x = origin.x;
            double y = origin.y;
            double w = size.width;
            double h = size.height;
            
            NSPoint mouseLoc = [NSEvent mouseLocation];
            
            MetalUtils::setFloatValue(this->mouseBuffer,
                (mouseLoc.x-x)/w,
                1.0-(mouseLoc.y-y)/h,
                (isClick)?1:0,
                0.15
            );
            
            
           
        }
};


class MetalLayerBase {
    
    protected:
    
        NSMutableString *filename;
    
        CAMetalLayer *metalLayer;
        MTLRenderPassDescriptor *renderPassDescriptor;
    
        id<MTLDevice> device;
        id<MTLCommandQueue> commandQueue;
        id<MTLLibrary> library;
        id<MTLRenderPipelineState> renderPipelineState;
        id<CAMetalDrawable> metalDrawable;
    
        id<MTLTexture> drawabletexture;
        id<MTLTexture> textureBuffer;
        id<MTLBuffer> resolutionBuffer;
        id<MTLBuffer> vertexBuffer;
        id<MTLBuffer> texcoordBuffer;
    
        id<MTLArgumentEncoder> argumentEncoder;
        id<MTLBuffer> argumentEncoderBuffer;
        MTLRenderPipelineDescriptor *renderPipelineDescriptor;
    
        CGRect rect;

        bool framebufferOnly;
    
        NSString *prefix;
    
        bool setupShader() {
            
            id<MTLFunction> vertexFunction  = [this->library newFunctionWithName:(this->prefix&&this->prefix.length>0)?[NSString stringWithFormat:@"%@%@",this->prefix,@"VertexShader"]:@"vertexShader"];
            if(!vertexFunction) return nil;
            
            id<MTLFunction> fragmentFunction = [this->library newFunctionWithName:(this->prefix&&this->prefix.length>0)?[NSString stringWithFormat:@"%@%@",this->prefix,@"FragmentShader"]:@"fragmentShader"];
            if(!fragmentFunction) return nil;
            
            if(this->renderPipelineDescriptor==nil) {
                this->renderPipelineDescriptor = [MTLRenderPipelineDescriptor new];
                if(!this->renderPipelineDescriptor) return nil;
                
                this->argumentEncoder = [fragmentFunction newArgumentEncoderWithBufferIndex:0];
            }
            
            this->renderPipelineDescriptor.depthAttachmentPixelFormat      = MTLPixelFormatInvalid;
            this->renderPipelineDescriptor.stencilAttachmentPixelFormat    = MTLPixelFormatInvalid;
            this->renderPipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
            
            if(this->prefix&&this->prefix.length>0) {
                
                MTLRenderPipelineColorAttachmentDescriptor *colorAttachment = this->renderPipelineDescriptor.colorAttachments[0];
                colorAttachment.blendingEnabled = NO;
                
            }
            else {
                
                MTLRenderPipelineColorAttachmentDescriptor *colorAttachment = this->renderPipelineDescriptor.colorAttachments[0];
                colorAttachment.blendingEnabled = YES;
                colorAttachment.rgbBlendOperation = MTLBlendOperationAdd;
                colorAttachment.alphaBlendOperation = MTLBlendOperationAdd;
                colorAttachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                colorAttachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
                colorAttachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                colorAttachment.destinationAlphaBlendFactor = MTLBlendFactorOne;
            }
            
            this->renderPipelineDescriptor.sampleCount = 1;
            
            this->renderPipelineDescriptor.vertexFunction = vertexFunction;
            this->renderPipelineDescriptor.fragmentFunction = fragmentFunction;
            
            NSError *error = nil;
            this->renderPipelineState = [this->device newRenderPipelineStateWithDescriptor:this->renderPipelineDescriptor error:&error];
            if(error||!this->renderPipelineState) return true;
            
            return false;
        }
    
        bool setupMetal() {
            
            this->metalLayer =  [CAMetalLayer layer];
            this->device = MTLCreateSystemDefaultDevice();
            
            this->metalLayer.device = this->device;
            this->metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            this->metalLayer.frame = this->rect;
            this->metalLayer.colorspace = [[NSScreen mainScreen] colorSpace].CGColorSpace;
            
            this->metalLayer.opaque = NO;
            this->metalLayer.framebufferOnly = (this->framebufferOnly)?YES:NO;
            this->metalLayer.displaySyncEnabled = YES;
            
            this->commandQueue = [this->device newCommandQueue];
            if(!this->commandQueue) return false;
            
            NSError *error = nil;
            this->library = [this->device newLibraryWithFile:this->filename error:&error];
            if(error||!this->library) return false;
            
            if(this->setupShader()) return false;
            
            this->vertexBuffer = [this->device newBufferWithBytes:Plane::vertexData length:6*sizeof(float)*4 options:MTLResourceOptionCPUCacheModeDefault];
            if(!this->vertexBuffer) return false;
            
            this->texcoordBuffer = [this->device newBufferWithBytes:Plane::textureCoordinateData length:6*sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault];
            if(!this->texcoordBuffer) return false;
            
            this->argumentEncoderBuffer = [this->device newBufferWithLength:sizeof(float)*[this->argumentEncoder encodedLength] options:MTLResourceOptionCPUCacheModeDefault];
            [this->argumentEncoder setArgumentBuffer:this->argumentEncoderBuffer offset:0];
            
            this->setup();
           
            return true;
        }
    
        id<MTLCommandBuffer> setupCommandBuffer() {
            
            if(!this->metalDrawable) {
                this->metalDrawable = [this->metalLayer nextDrawable];
            }
            
            if(!this->metalDrawable) {
                this->renderPassDescriptor = nil;
            }
            else {
                if(this->renderPassDescriptor == nil) {
                    this->renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
                }
            }
            
            if(this->metalDrawable&&this->renderPassDescriptor) {
                
                id<MTLCommandBuffer> commandBuffer = [this->commandQueue commandBuffer];
                
                MTLRenderPassColorAttachmentDescriptor *colorAttachment = this->renderPassDescriptor.colorAttachments[0];
                colorAttachment.texture = this->metalDrawable.texture;
                colorAttachment.loadAction  = MTLLoadActionClear;
                colorAttachment.clearColor  = MTLClearColorMake(0.0f,0.0f,0.0f,0.0f);
                colorAttachment.storeAction = MTLStoreActionStore;
                
                id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:this->renderPassDescriptor];
                
                [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
                [renderEncoder setRenderPipelineState:this->renderPipelineState];
                
                [renderEncoder setVertexBuffer:this->vertexBuffer offset:0 atIndex:0];
                [renderEncoder setVertexBuffer:this->texcoordBuffer offset:0 atIndex:1];
                
                this->set(renderEncoder);
                
                [renderEncoder setFragmentBuffer:this->argumentEncoderBuffer offset:0 atIndex:0];
                
                [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6 instanceCount:1];
                [renderEncoder endEncoding];
                [commandBuffer presentDrawable:this->metalDrawable];
                
                this->drawabletexture = this->metalDrawable.texture;
                
                return commandBuffer;
            }
            
            return nil;
        }
    
        virtual void setup() = 0;
        virtual void set(id<MTLRenderCommandEncoder> renderEncoder) = 0;
    
    public:
    
        CGRect frame() { return this->rect; }
    
        CAMetalLayer *layer() { return this->metalLayer; };
    
        id<MTLTexture> texture() { return this->textureBuffer; }

        void texture(id<MTLTexture> texture) {
            [this->argumentEncoder setTexture:texture atIndex:3];
        }
        
        id<MTLTexture> drawableTexture() { return this->drawabletexture; }
        
        void cleanup() { this->metalDrawable = nil; }
    
        void update(void (^onComplete)(id<MTLCommandBuffer>)=nil) {
             
            if(this->renderPipelineState) {
                id<MTLCommandBuffer> commandBuffer = this->setupCommandBuffer();
                if(commandBuffer) {
                    if(onComplete) [commandBuffer addCompletedHandler:onComplete];
                    [commandBuffer commit];
                }
            }
        }
    
        MetalLayerBase() {
            
        }
    
        ~MetalLayerBase() {
            
        }
                 
};

class RGBA32FloatMetalLayer : public MetalLayerBase {
    
    private:
    
        void setup() {
 
            MTLTextureDescriptor *texDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float width:this->rect.size.width height:this->rect.size.height mipmapped:NO];
            this->textureBuffer = [this->device newTextureWithDescriptor:texDesc];
            
            this->resolutionBuffer = [this->device newBufferWithLength:sizeof(float)*2 options:MTLResourceOptionCPUCacheModeDefault];
            
            float *r = (float *)[this->resolutionBuffer contents];
            r[0] = this->rect.size.width;
            r[1] = this->rect.size.height;
            
            [this->argumentEncoder setBuffer:MetalUniform::$()->time() offset:0 atIndex:0];
            [this->argumentEncoder setBuffer:this->resolutionBuffer offset:0 atIndex:1];
            [this->argumentEncoder setBuffer:MetalUniform::$()->mouse() offset:0 atIndex:2];
            [this->argumentEncoder setTexture:this->textureBuffer atIndex:3];
        }
    
        void set(id<MTLRenderCommandEncoder> renderEncoder) {
            [renderEncoder useResource:MetalUniform::$()->time() usage:MTLResourceUsageRead];
            [renderEncoder useResource:MetalUniform::$()->mouse() usage:MTLResourceUsageRead];
            [renderEncoder useResource:this->resolutionBuffer usage:MTLResourceUsageRead];
            [renderEncoder useResource:this->textureBuffer usage:MTLResourceUsageSample];
        }
    
    public:
    
        RGBA32FloatMetalLayer(NSString *fileName, NSString *prefix, CGRect frame, bool framebuffer) {
            
            this->filename = [NSMutableString stringWithString:[[NSBundle mainBundle] pathForResource:fileName ofType:@"metallib"]];
            this->rect = frame;
            this->framebufferOnly = framebuffer;
            this->prefix = prefix;
            this->setupMetal();
        }
    
    
};
