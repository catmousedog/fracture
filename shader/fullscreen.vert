#version 450

// Fullscreen triangle trick — no vertex buffer needed.
// gl_VertexIndex 0,1,2 maps to positions that cover the entire screen.
// The GPU clips the triangle to the viewport automatically.

layout(location = 0) out vec2 uv; // [0,1] UV passed to the fragment shader

void main()
{
    // Generates a triangle that covers the screen:
    //   index 0 → bottom-left  (-1, -1)
    //   index 1 → bottom-right ( 3, -1)
    //   index 2 → top-left     (-1,  3)
    uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}
