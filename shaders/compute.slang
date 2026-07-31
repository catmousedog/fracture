[[vk::binding(0, 0)]]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    outputImage.GetDimensions(width, height);

    if (id.x >= width || id.y >= height)
        return;

    float2 uv = float2(id.xy) / float2(width, height);
    outputImage[id.xy] = float4(uv.x, uv.y, 0.5, 1.0);
}