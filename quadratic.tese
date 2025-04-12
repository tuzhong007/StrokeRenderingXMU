#version 450 core

layout(isolines) in;

in vec2 tcsTexCoord[];
in vec3 tcsColor[];
in float Phi[];
in int segments[];

layout(location = 0) out vec3 color;
layout(location = 1) out float parameterT;
layout(location = 2) out vec2 P0;
layout(location = 3) out vec2 P1;
layout(location = 4) out vec2 P2;
layout(location = 5) out int segID;

vec2 quadraticBezier(vec2 p0, vec2 p1, vec2 p2, float t) {
	return (1.0 - t) * (1.0 - t) * p0 + 2.0 * (1.0 - t) * t * p1 + t * t * p2;
}

void main() {
    // Interpolate position
    float u = gl_TessCoord.x;
    segID = int(ceil(u * segments[0]-0.05)); // segment ID
    P0 = gl_in[0].gl_Position.xy;
    P1 = gl_in[1].gl_Position.xy;
    P2 = gl_in[2].gl_Position.xy;

    parameterT = u;
    //. to be modified: solve t by Phi_i

    color = vec3(1, 0, 0);
    gl_Position = vec4(quadraticBezier(P0, P1, P2, parameterT), 0, 1); // Dummy value just to satisfy the pipeline
}
