#define prefix reactiondiffusion

#import <metal_stdlib>
#import "ShaderUtils.h"


struct Arguments(prefix) {
    device float2 *resolution[[id(0)]];
    device float4 *mouse[[id(1)]];
    device float4 *coeff[[id(2)]];
};


using namespace metal;

kernel void reactiondiffusion(
    texture2d<float,access::sample> srcTexture[[texture(0)]],
    texture2d<float,access::write> dstTexture[[texture(1)]],
    constant Arguments(prefix) &args[[buffer(0)]],
    sampler samplers[[sampler(0)]],
    uint2 gid[[thread_position_in_grid]]) {
    
    
    if(gid.x<args.resolution[0].x&&gid.y<args.resolution[0].y) {
        
        float dA = args.coeff[0].x;
        float dB = args.coeff[0].y;
        float feed = args.coeff[0].z;
        float kill = args.coeff[0].w;
        
        float2 pos = float2(gid);

        float2 aspect = float2(args.resolution[0].x/args.resolution[0].y,1.0);

        float2 weight = 1.0/args.resolution[0];

        float2 src = srcTexture.sample(samplers,(pos+float2(0.5))*weight).xy;

        float2 N  = srcTexture.sample(samplers,(pos+float2( 0.5,-0.5))*weight).xy;
        float2 S  = srcTexture.sample(samplers,(pos+float2( 0.5, 1.5))*weight).xy;
        float2 E  = srcTexture.sample(samplers,(pos+float2( 1.5, 0.5))*weight).xy;
        float2 W  = srcTexture.sample(samplers,(pos+float2(-0.5, 0.5))*weight).xy;

        float2 NE = srcTexture.sample(samplers,(pos+float2( 1.5, 1.5))*weight).xy;
        float2 NW = srcTexture.sample(samplers,(pos+float2(-0.5, 1.5))*weight).xy;
        float2 SE = srcTexture.sample(samplers,(pos+float2( 1.5,-0.5))*weight).xy;
        float2 SW = srcTexture.sample(samplers,(pos+float2(-0.5,-0.5))*weight).xy;
         
        float diff1 = 0.2;
        float diff2 = 0.05;
         
        float2 lap = vec2(src)*-1.0;
        lap += N * diff1;
        lap += S * diff1;
        lap += E * diff1;
        lap += W * diff1;
        lap += NE * diff2;
        lap += NW * diff2;
        lap += SE * diff2;
        lap += SW * diff2;
         
        src.x += ((dA*lap.x) - (src.x*src.y*src.y) + (feed*(1.0-src.x)));
        src.y += ((dB*lap.y) + (src.x*src.y*src.y) - ((kill+feed)*src.y));
        
        if(args.mouse[0].z==1.0) {
                
            float dist = distance(((args.mouse[0].xy*2.0)-1.0)*aspect,(float2(gid)/(args.resolution[0]*0.5)-1.0)*aspect);
            if(dist<args.mouse[0].w)  {
                src.y += (1.0-dist/args.mouse[0].w)*0.1;
            }
        }
                
        dstTexture.write(float4(clamp(src,0.0,1.0),1.0,1.0),gid);
        
    }
}
