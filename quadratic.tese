#version 450 core

layout(isolines) in;

in vec2 tcsTexCoord[];
in vec3 tcsColor[];
in float Phi[];
in float cosPhi[];
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

// A = 2|P1-P0|, B = 2|P2-P1|, C = cos(Phi), D = cos(i * Phi / K), where K is the total amount of segments
// solve t_i from the correspoding angle relation: the angle between the tangent at t_i and the tangent at 0 is u * Phi
vec2 calculate_ti_From_Phii(float A, float B, float C, float D) {
    float sigma1 = -A*A* D*D + A*A + 2*A*B*C*D*D - 2*A*B*C + B*B * C*C - B*B * D*D;
    float sigma2 = B * D * sqrt((C*C - 1) * (D*D - 1));
    float t1 = A * (A - A * D*D - B*C + sigma2 + B*C*D*D)/sigma1;
    float t2 = A * (A - A * D*D - B*C - sigma2 + B*C*D*D)/sigma1;
    return vec2(t1, t2);
}

void main() {
    // Interpolate position
    color = vec3(1, 0, 0);
    float u = gl_TessCoord.x;
    segID = int(ceil(u * segments[0]-0.05)); // segment ID
    P0 = gl_in[0].gl_Position.xy;
    P1 = gl_in[1].gl_Position.xy;
    P2 = gl_in[2].gl_Position.xy;
    vec2 P0P1 = P1 - P0, P1P2 = P2 - P1;
    // check if u is 0 or 1
    if (u < 1e-3 || u > 1 - 1e-3) {
		parameterT = u;
		return;
	}

    float A = 2 * length(P1 - P0), B = 2 * length(P2 - P1), C = cosPhi[0], D = cos(u * Phi[0]);

    vec2 t1t2 = calculate_ti_From_Phii(A, B, C, D);
    float t1 = t1t2.x, t2 = t1t2.y;
    // check if t1 and t2 are in [0, 1]
    if (t1 < 0 || t1 > 1) {
        parameterT = t2;
        return;
    }
	else if (t2 < 0 || t2 > 1) {
		parameterT = t1;
        return;
	}

    // check if the tangent at t1 and the tangent at 1 has the angle (1-u) * Phi
    // if so, t1 is the solution
    // otherwise, t2 is the solution
    vec2 tangent_t1 = 2 * (1 - t1) * P0P1 + 2 * t1 * P1P2;
    float angle_t1 = acos(dot(normalize(tangent_t1), normalize(P2 - P0)));

    if (abs(angle_t1 - (1-u) * Phi[0]) < 1e-3) {
		parameterT = t1;
		return;
	}
	else {
		parameterT = t2;
		return;
	}

}
