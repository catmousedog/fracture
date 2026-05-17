#version 450

layout(location = 0) in  vec2 uv;
layout(location = 0) out vec4 outColor;

void main()
{
    // Map uv [0,1] to Mandelbrot space
    vec2 c = (uv - vec2(0.5, 0.5)) * 3.5 - vec2(0.7, 0.0);

    vec2  z         = vec2(0.0);
    int   maxIter   = 128;
    int   i         = 0;

    for (; i < maxIter; i++)
    {
        if (dot(z, z) > 4.0) break;
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
    }

    float t = float(i) / float(maxIter);
    // Simple smooth coloring
    vec3 col = 0.5 + 0.5 * cos(3.14159 * t * 3.0 + vec3(0.0, 0.6, 1.0));

    outColor = vec4(col, 1.0);
}
