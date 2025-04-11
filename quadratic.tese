#version 450 core

layout(triangles, equal_spacing, cw) in;

in vec2 tcsTexCoord[];
in vec3 tcsColor[];

out vec2 TexCoord;
out vec3 color;

void main() {
    // Interpolate position
    gl_Position =
        gl_TessCoord.x * gl_in[0].gl_Position +
        gl_TessCoord.y * gl_in[1].gl_Position +
        gl_TessCoord.z * gl_in[2].gl_Position;

    // Interpolate attributes
    TexCoord =
        gl_TessCoord.x * tcsTexCoord[0] +
        gl_TessCoord.y * tcsTexCoord[1] +
        gl_TessCoord.z * tcsTexCoord[2];

    color =
        gl_TessCoord.x * tcsColor[0] +
        gl_TessCoord.y * tcsColor[1] +
        gl_TessCoord.z * tcsColor[2];
}
