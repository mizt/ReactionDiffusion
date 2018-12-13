#import <metal_stdlib>
#import "ShaderUtils.h"

using namespace metal;

kernel void reactiondiffusion(texture2d<float,access::sample> srcTexture[[texture(0)]],texture2d<float,access::write> dstTexture[[texture(1)]],
    const device float2 &resolution[[buffer(0)]],const device float4 &mouse[[buffer(1)]],const device float4 &coeff[[buffer(2)]],sampler wrap[[sampler(0)]],uint2 gid[[thread_position_in_grid]]) {
    
    
    if(gid.x<resolution.x&&gid.y<resolution.y) {
        
        float dA = coeff[0];
        float dB = coeff[1];
        float feed = coeff[2];
        float kill = coeff[3];
        
        ushort width = srcTexture.get_width();
        ushort height = srcTexture.get_height();
        float2 bounds(width, height);
        float2 pos = float2(gid);

        float2 aspect = float2(resolution.x/resolution.y,1.0);

        float2 weight = 1.0/bounds;

        float2 src = srcTexture.sample(wrap,(pos+float2(0.5))*weight).xy;

        float2 N  = srcTexture.sample(SAMPLAR,(pos+float2( 0.5,-0.5))*weight).xy;
        float2 S  = srcTexture.sample(SAMPLAR,(pos+float2( 0.5, 1.5))*weight).xy;
        float2 E  = srcTexture.sample(SAMPLAR,(pos+float2( 1.5, 0.5))*weight).xy;
        float2 W  = srcTexture.sample(SAMPLAR,(pos+float2(-0.5, 0.5))*weight).xy;

        float2 NE = srcTexture.sample(SAMPLAR,(pos+float2( 1.5, 1.5))*weight).xy;
        float2 NW = srcTexture.sample(SAMPLAR,(pos+float2(-0.5, 1.5))*weight).xy;
        float2 SE = srcTexture.sample(SAMPLAR,(pos+float2( 1.5,-0.5))*weight).xy;
        float2 SW = srcTexture.sample(SAMPLAR,(pos+float2(-0.5,-0.5))*weight).xy;
         
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
        
        if(mouse.z==1.0) {
                
            float dist = distance(((mouse.xy*2.0)-1.0)*aspect,(float2(gid)/(resolution*0.5)-1.0)*aspect);
            if(dist<mouse.w)  {
                src.y += (1.0-dist/mouse.w)*0.1;
            }
        }
                
        dstTexture.write(float4(clamp(src,0.0,1.0),1.0,1.0),gid);
        
    }
}
