struct UniformBufferObject { float2 offset; float zoom; };
[[vk::binding(0, 0)]] Sampler2D tex;
[[vk::binding(1, 0)]] ConstantBuffer<UniformBufferObject> ubo;

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
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    return tex.Sample(vertIn.fragTexCoord + ubo.offset);
}