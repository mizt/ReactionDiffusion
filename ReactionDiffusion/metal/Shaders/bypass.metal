#define prefix bypass

#import <metal_stdlib>
#import "ShaderUtils.h"

using namespace metal;

struct Arguments(prefix) {
    device float *time[[id(0)]];
    device float2 *resolution[[id(1)]];
    device float4 *mouse[[id(2)]];
    texture2d<float> texture[[id(3)]];
};

vertex VertInOut VertexShader(prefix)(constant float4 *pos[[buffer(0)]],constant packed_float2 *texcoord[[buffer(1)]],uint vid[[vertex_id]]) {
    VertInOut outVert;
    outVert.pos = pos[vid];
    outVert.texcoord = float2(texcoord[vid][0],texcoord[vid][1]);
    return outVert;
}

fragment float4 FragmentShader(prefix)(VertInOut inFrag[[stage_in]],constant Arguments(prefix) &args[[buffer(0)]]) {
    return args.texture.sample(SAMPLAR,inFrag.texcoord.xy);
}
