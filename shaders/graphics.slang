[[vk::binding(0, 0)]] Sampler2D tex;
[[vk::binding(1, 0)]] ConstantBuffer<UniformBufferObject> ubo;

struct UniformBufferObject { 
	float2 offset, prevOffset;
	float zoom, prevZoom;
	uint2 computeSize;
	float aspect;
};

struct VSInput {
    float2 inPosition;
    float2 inTexCoord;
};

struct VSOutput {
    float4 pos : SV_Position;
    float2 fragTexCoord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.inPosition, 0.0, 1.0);
    output.fragTexCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {          // texel coords [0, 1]
	if (any(ubo.computeSize == 0)) return float4(0, 0, 0, 1);   // nothing computed yet
    float2 ar = float2(ubo.aspect, 1.0);
	float2 p  = (vertIn.fragTexCoord * 2.0 - 1.0) * ar; // -> centered screen (current view)
	float2 c  = ubo.offset + p * ubo.zoom;              // -> complex plane (current view)
	float2 q  = (c - ubo.prevOffset) / ubo.prevZoom;    // -> centered screen (previous view)
	float2 uv = (q / ar + 1.0) * 0.5;                   // -> texel coords [0, 1] (previous view)
	if (any(uv < 0.0) || any(uv > 1.0)) return float4(0, 0, 0, 1);
	float2 imageSize;  tex.GetDimensions(imageSize.x, imageSize.y);
	float2 px = clamp(uv * ubo.computeSize, 0.5, ubo.computeSize - 0.5);   // stay half a texel inside the corner
	return tex.Sample(px / imageSize);
}