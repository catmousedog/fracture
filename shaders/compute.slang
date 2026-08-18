struct FractalPushConstants {
    float2 center;   // point in complex plane the view is centered on
    float zoom;      // scale factor
    int maxIterations;
};

[[vk::push_constant]]
FractalPushConstants pc;

[[vk::binding(0, 0)]]
RWTexture2D<float4> outputImage;

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 id : SV_DispatchThreadID)
{
    uint width, height;
    outputImage.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) return;

    float2 uv = (float2(id.xy) / float2(width, height)) * 2.0 - 1.0;
    uv.x *= float(width) / float(height);

    float2 c = pc.center + uv * pc.zoom;

    float2 z = float2(0.0, 0.0);
    int i;
    for (i = 0; i < pc.maxIterations; i++)
    {
        float2 zSquared = float2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y);
        z = zSquared + c;
        if (dot(z, z) > 4.0) break;
    }

    float t = i / (float)pc.maxIterations;
    outputImage[id.xy] = float4(t, t, t, 1.0);
}