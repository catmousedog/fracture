struct PushConstants {
    float offsetX, offsetY;
    float zoom;
    int maxIter;
};

[[vk::push_constant]]
PushConstants pc;

struct VSInput {
    float2 inPos;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float2 coord;
};

[shader("vertex")]
VSOutput vertMain(VSInput input) {
    VSOutput output;
    output.pos = float4(input.inPos, 0.0, 1.0);
	output.coord = input.inPos;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {

    float cx = vertIn.coord.x * pc.zoom + pc.offsetX;
    float cy = vertIn.coord.y * pc.zoom + pc.offsetY;

    float x = 0;
    float y = 0;

    int i;
    for (i = 0; i < pc.maxIter; i++) {
        float tx = x;

        x = x * x - y * y + cx;
        y = 2 * tx * y + cy;

        if (x * x + y * y > 4) {
            break;
        }
    }

    float color = float(i) / pc.maxIter;
    return float4(color, color, color, 1.0);
}