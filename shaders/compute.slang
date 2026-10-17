[[vk::push_constant]]
FractalPushConstants pc;

[[vk::binding(0, 0)]]
RWTexture2D<float4> outputImage;

struct FractalPushConstants {
	float2 center;     // point in complex plane the view is centered on
    float zoom;        // scale factor
    int maxIterations;
	uint2 computeSize; // size of the sub-image to compute
	float aspect;      // aspect of the full image
};

[shader("compute")]
[numthreads(16, 16, 1)]
void compMain(uint3 id : SV_DispatchThreadID)
{
	if (any(id.xy >= pc.computeSize)) return;

	// sub-image coordinates -> (x, y) = ([-aspect, aspect], [-1, 1])
    float2 uv = (float2(id.xy) + 0.5) / float2(pc.computeSize) * 2.0 - 1.0;
    uv.x *= pc.aspect; // aspect of full image and not (rounded) sub-image
	
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