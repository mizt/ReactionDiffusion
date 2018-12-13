using namespace metal;

#define vec2 float2
#define vec3 float3
#define vec4 float4
#define ivec2 int2
#define ivec3 int3
#define ivec4 int4
#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4

struct VertInOut {
	float4 pos[[position]];
	float2 texcoord[[user(texturecoord)]];
};

struct FragmentShaderArguments {
	device float *time[[id(0)]];
	device float2 *resolution[[id(1)]];
	device float4 *mouse[[id(2)]];
	texture2d<float> texture[[id(3)]];
};

#define strconcat(a,aa) a##aa
#define VertexShader(a) strconcat(a,VertexShader)
#define FragmentShader(a) strconcat(a,FragmentShader)

constexpr sampler SAMPLAR(
    coord::normalized,
    address::repeat,
    filter::linear
);
