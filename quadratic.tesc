#version 450 core

layout(vertices = 3) out;  // Triangle patches

in vec2 TexCoord[];
in vec3 color[];

out vec2 tcsTexCoord[];
out vec3 tcsColor[];

void main() {
    // Pass attributes to TES
    tcsTexCoord[gl_InvocationID] = TexCoord[gl_InvocationID];
    tcsColor[gl_InvocationID] = color[gl_InvocationID];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    // Set constant tessellation levels (can be dynamic later)
    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = 0.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
    }
}
