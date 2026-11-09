/* ====================== PRECISION ===================== */
// REAL is set by the host via SessionDesc::preprocessorMacros ("float" or "double").
#ifndef REAL
#define REAL float
#endif
typealias Real  = REAL;
typealias Real2 = vector<Real, 2>;

/* ======================== INPUT ======================= */
[[vk::push_constant]]
FractalPushConstants pc;

[[vk::binding(0, 0)]]
RWTexture2D<float4> outputImage;

struct FractalPushConstants {
	Real2 center;     // point in complex plane the view is centered on
    Real zoom;        // scale factor
    int maxIterations;
};

/* ======================== DATA ======================== */

struct Orbit {
	int iter; // iterations to escape
	Real2 z; // z after escape
}

/* ====================== INTERFACE ===================== */

interface IFractal {
	Orbit iterate(Real2 c, int maxIter);
}

interface IFunction {
	float apply(Orbit o);
}

interface IColor {
	float4 shade(float iter);
}

/* ====================== FRACTALS ====================== */
struct Mandelbrot : IFractal {
	Orbit iterate(Real2 c, int maxIter) {
		Real2 z = Real2(0.0, 0.0);
		int i;
		for (i = 0; i < maxIter; i++) {
			Real x2 = z.x*z.x;
			Real y2 = z.y*z.y;
			z = Real2(x2 - y2, 2.0*z.x*z.y) + c; // z = z^2 + c
			if (x2 + y2 > 65536.0) break;
		}
		Orbit orbit;
		orbit.iter = i;
		orbit.z = z;
		return orbit;
	}
};

struct BurningShip : IFractal {
    Orbit iterate(Real2 c, int maxIter) {
        Real2 z = Real2(0.0, 0.0);
        int i;
        for (i = 0; i < maxIter; i++) {
            Real x2 = z.x*z.x;
            Real y2 = z.y*z.y;
            if (x2 + y2 > 65536.0) break;
            z = Real2(x2 - y2, 2.0*abs(z.x*z.y)) + c; // z = (|Re z| + i|Im z|)^2 + c
        }
        Orbit orbit;
		orbit.iter = i;
		orbit.z = z;
		return orbit;
    }
};

/* ====================== FUNCTIONS ===================== */
struct Iterative : IFunction {
	float apply(Orbit o) {
		return (float) o.iter / (float)pc.maxIterations;
	}
};

struct Normalized : IFunction {
	float apply(Orbit o) {
		if (o.iter >= pc.maxIterations) return 1.0;     // converged
		float r2 = float(dot(o.z, o.z));                // |z|^2, log is 32-bit only
		float nu = o.iter + 1.0 - log2(0.5 * log(r2));  // log|z| = 0.5 * log|z|^2
		return nu / pc.maxIterations;                   // same scale as Iterative
	}
}

/* ======================= COLORS ======================= */
struct GrayScale : IColor {
	float4 shade(float f) {
		return float4(f, f, f, 1.0);
	}
};

extern struct Fractal : IFractal;
extern struct Function : IFunction;
extern struct Color : IColor;

/* ======================= COMPUTE ====================== */
[shader("compute")] [numthreads(16, 16, 1)]
void compMain(uint3 id : SV_DispatchThreadID)
{
	uint width, height;
    outputImage.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) return;

	Fractal fractal; Function function; Color color;

	// sub-image coordinates -> (x, y) = ([-aspect, aspect], [-1, 1])
    float2 uv = (float2(id.xy) + 0.5) / float2(width, height) * 2.0 - 1.0;
    uv.x *= float(width) / float(height);
	Real2 c = pc.center + Real2(uv) * pc.zoom;

	Orbit o = fractal.iterate(c, pc.maxIterations);
	float f = function.apply(o);
	outputImage[id.xy] = color.shade(f);
}