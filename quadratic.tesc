#version 450 core

layout(vertices = 3) out;  // Triangle patches

in vec2 TexCoord[];
in vec3 color[];

out vec2 tcsTexCoord[];
out vec3 tcsColor[];
out float Phi[];
out float cosPhi[];
out int segments[]; // Number of segments for each edge

void main() {
    // Pass attributes to TES
    
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    // Set constant tessellation levels (can be dynamic later)
    if (gl_InvocationID == 0) {
        vec2 P0P1 = gl_in[1].gl_Position.xy - gl_in[0].gl_Position.xy;
        vec2 P1P2 = gl_in[2].gl_Position.xy - gl_in[1].gl_Position.xy;

        tcsTexCoord[gl_InvocationID] = TexCoord[gl_InvocationID];
        tcsColor[gl_InvocationID] = color[gl_InvocationID];
        cosPhi[gl_InvocationID] = dot(normalize(P0P1), normalize(P1P2));
        Phi[gl_InvocationID] = acos(cosPhi[gl_InvocationID]);
        int seg = int(ceil(Phi[gl_InvocationID] / 0.5));
        segments[gl_InvocationID] = seg; // Number of segments for each edge
        
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = seg; //. to be modified
        
    }
}
