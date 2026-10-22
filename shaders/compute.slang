/* ======================== INPUT ======================= */
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

/* ====================== INTERFACE ===================== */
interface IFractal {
	float iterate(float2 c, int maxIter);
}

interface IColor {
	float4 shade(float iter);
}

struct Mandelbrot : IFractal {
	float iterate(float2 c, int maxIter) {
		float2 z = float2(0.0, 0.0);
		int i;
		for (i = 0; i < maxIter; i++) {
			float x2 = z.x*z.x;
			float y2 = z.y*z.y;
			z = float2(x2 - y2, 2.0*z.x*z.y) + c; // z = z^2 + c
			if (x2 + y2 > 4.0) break;
		}
		return i / (float)maxIter;
	}
};

struct BurningShip : IFractal {
    float iterate(float2 c, int maxIter) {
        float2 z = float2(0.0, 0.0);
        int i;
        for (i = 0; i < maxIter; i++) {
            float x2 = z.x*z.x;
            float y2 = z.y*z.y;
            if (x2 + y2 > 4.0) break;
            z = float2(x2 - y2, 2.0*abs(z.x*z.y)) + c; // z = (|Re z| + i|Im z|)^2 + c
        }
        return i / (float)maxIter;
    }
};

struct GrayScale : IColor {
	float4 shade(float f) {
		return float4(f, f, f, 1.0);
	}
};

extern struct Fractal : IFractal;
extern struct Color : IColor;

/* ======================= COMPUTE ====================== */
[shader("compute")] [numthreads(16, 16, 1)]
void compMain(uint3 id : SV_DispatchThreadID)
{
	if (any(id.xy >= pc.computeSize)) return;

	Fractal fractal; Color color;

	// sub-image coordinates -> (x, y) = ([-aspect, aspect], [-1, 1])
    float2 uv = (float2(id.xy) + 0.5) / float2(pc.computeSize) * 2.0 - 1.0;
    uv.x *= pc.aspect; // aspect of full image and not (rounded) sub-image
	float2 c = pc.center + uv * pc.zoom;

	float f = fractal.iterate(c, pc.maxIterations);
	outputImage[id.xy] = color.shade(f);
}