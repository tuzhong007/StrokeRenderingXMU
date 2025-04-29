#version 450 core
#define MAX_SEG 7

layout(isolines) in;

uniform float l1; // the length of each solid line
uniform float l2; // the gap length of two solid lines

in vec2 tcsTexCoord[];
in vec3 tcsColor[];
//in float Phi[];
//in float cosPhi[];
//in int segments[];
patch in float t_i[MAX_SEG+1];
patch in float s_ti[MAX_SEG+1];
// Number of solid lines in i-th segment
patch in int solidLines_Num_i[MAX_SEG];
//patch in vec2 p0[MAX_SEG + 1];
//patch in vec2 p1[MAX_SEG + 1];
//patch in vec2 p2[MAX_SEG + 1];

layout(location = 0) out vec3 color;
layout(location = 1) out float parameterT;
layout(location = 2) out vec2 P0sub;
layout(location = 3) out vec2 P1sub;
layout(location = 4) out vec2 P2sub;
layout(location = 5) out int segID;
layout(location = 6) flat out int renderFlag;
layout(location = 7) out float s_ti_sub;
//layout(location = 8) out float currentStartLength0;


vec2 quadraticBezier(vec2 p0, vec2 p1, vec2 p2, float t) {
	return (1.0 - t) * (1.0 - t) * p0 + 2.0 * (1.0 - t) * t * p1 + t * t * p2;
}

// get the analytic arclength of the quadratic curve from t=0 to t=t
// https://stackoverflow.com/questions/11854907/calculate-the-length-of-a-segment-of-a-quadratic-bezier
float getArcLength_t(vec2 p0, vec2 p1, vec2 p2, float t) {
    float x0 = p0.x, y0 = p0.y, x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y;
    float ax,ay,bx,by,A,B,C,b,c,u,k,L;
    // check if the quadratic curve is degenerate, or p0p1 and p1p2 are parallel
    if (abs((x1-x0)*(y2-y1)-(x2-x1)*(y1-y0)) < 1e-3) {
		return length(quadraticBezier(p0, p1, p2, t) - p0);
	}
    ax=x0-x1-x1+x2;
    ay=y0-y1-y1+y2;
    bx=x1+x1-x0-x0;
    by=y1+y1-y0-y0;
    A=4.0*((ax*ax)+(ay*ay));
    B=4.0*((ax*bx)+(ay*by));
    C=     (bx*bx)+(by*by);
    b=B/(2.0*A);
    c=C/A;
    u=t+b;
    k=c-(b*b);
    L=0.5*sqrt(A)*
        (
         (u*sqrt((u*u)+k))
        -(b*sqrt((b*b)+k))
        +(k*log(abs((u+sqrt((u*u)+k))/(b+sqrt((b*b)+k)))))
        );
    return L;
}

// get the t value for a given arc length
float solve_t_Given_S(vec2 p0, vec2 p1, vec2 p2, float S, float totalLength){
    float t = 0.5;
    float R = 2 * dot(p1-p0, p2-p0) / dot(p2-p0, p2-p0) - 1;
    float m4 = R * length(p2-p0);
    //float m4 = R * totalLength;
    float A = -m4, B = m4 + totalLength, C = -S;
    float Delta = B*B - 4 * A * C;
    float t1 = (-B + sqrt(Delta)) / (2 * A);
    float t2 = (-B - sqrt(Delta)) / (2 * A);

    if (t1 >= 0 && t1 <= 1) t = t1;
    else if (t2 >= 0 && t2 <= 1) t = t2;

    // Newton iteration
    float error = S - getArcLength_t(p0, p1, p2, t);
    float derivative = length(2 * (1 - t) * (p1 - p0) + 2 * t * (p2 - p1));
    while (abs(error) > 1e-2)
    {
        t = t + error / derivative;
		error = S - getArcLength_t(p0, p1, p2, t);
		derivative = length(2 * (1 - t) * (p1 - p0) + 2 * t * (p2 - p1));
    }
//    for (int i = 0; i < 2; ++i) {
//        t = t + error / derivative;
//		error = S - getArcLength_t(p0, p1, p2, t);
//		derivative = length(2 * (1 - t) * (p1 - p0) + 2 * t * (p2 - p1));
//    }
    return t;
}

vec2 getIntersection(vec2 a, vec2 b, vec2 c, vec2 d) {
    // check if b and d are parallel
    float det = b.x * d.y - b.y * d.x;
    if (abs(det) < 1e-3) {
		// lines are parallel, return a
		return (a + c) / 2;
	}
	vec2 alpha_beta = inverse(mat2(b, -d)) * (c - a);
    return a + alpha_beta.x * b;
}

void main() {
    // Interpolate position
    color = vec3(1, 0, 0);
    vec2 P0 = gl_in[0].gl_Position.xy;
    vec2 P1 = gl_in[1].gl_Position.xy;
    vec2 P2 = gl_in[2].gl_Position.xy;
    vec2 P0P1 = P1 - P0, P1P2 = P2 - P1;

    float u = gl_TessCoord.x, v = gl_TessCoord.y;
    segID = int(ceil(v * gl_TessLevelOuter[0] - 0.5)); // segment ID: which segment (tessellated by curvature) the point belongs to
    int solidLineID = int(ceil(u * gl_TessLevelOuter[1] - 0.05)) / 2; // solid line ID: which solid line the point belongs to
    if (solidLineID > solidLines_Num_i[segID])
    {
        renderFlag = 0; // already out of range, need not render
        return;
    }
    bool isStartPoint = ((int(ceil(u * gl_TessLevelOuter[1] - 0.05)) % 2) == 0); // whether the point is the start point of a solid line
    // segID and solidLineID completely determines the arc length S, hence the parameter t
    float segLength = s_ti[segID + 1] - s_ti[segID];
    
    // calculate the start position of the first solid line
    float modulus = mod(s_ti[segID], (l1 + l2));
    float firstStartLength = (modulus <= l1 ? 0 : (l1 + l2) - modulus), currentStartLength;

    float firstSolidLineLength, currentSolidLineLength; 
    // calculate the length of the first solid line
    if (firstStartLength < 1e-3) {
        firstSolidLineLength = min(l1 - modulus, segLength - firstStartLength);
    }
    else {
        firstSolidLineLength = min(l1, segLength - firstStartLength);
    }
    
    // calculate the start position of the current solid line
    if (solidLineID == 0)
    {
        currentStartLength = firstStartLength;
    }
    else
    {
        // careful, the first solid line might not have length l1
        currentStartLength = firstStartLength + firstSolidLineLength + l2 + (solidLineID - 1) * (l1 + l2);
    }

    if (currentStartLength > segLength) {
		// already out of range, need not render
        renderFlag = 0;
		return;
	}

    // calculate the length of the current solid line
    if (currentStartLength < 1e-3) {
        currentSolidLineLength = min(l1 - modulus, segLength - currentStartLength);
    }
    else {
        currentSolidLineLength = min(l1, segLength - currentStartLength);
    }
    // pass the length of the current point (w.r.t. the original quadratic curve) to the geometry shader for texture mapping
    //currentStartLength0 = gl_TessLevelOuter[1];
    

    // the control points of the current segment
    vec2 p0 = quadraticBezier(P0, P1, P2, t_i[segID]), p1, p2 = quadraticBezier(P0, P1, P2, t_i[segID + 1]);
    vec2 p0p1_direction = 2 * (1 - t_i[segID]) * (P0P1) + 2 * t_i[segID] * (P1P2);
    vec2 p1p2_direction = 2 * (1 - t_i[segID + 1]) * (P0P1) + 2 * t_i[segID + 1] * (P1P2);
    // p1 is the intersection point of the two tangents
    p1 = getIntersection(p0, p0p1_direction, p2, p1p2_direction);
    P0sub = p0; 
    P1sub = p1;
    P2sub = p2;

    // calculate the parameter t for the current point by its arc length
    // if the point is a start point, just use the current start length to calculate t
    if (isStartPoint) {
        parameterT = solve_t_Given_S(p0, p1, p2, currentStartLength, segLength);
        // mark it's the start point
        renderFlag = 1;
        s_ti_sub = s_ti[segID] + currentStartLength;
	}
	else {
		// if the point is an end point, we need to add the length of the solid line
		
        parameterT = solve_t_Given_S(p0, p1, p2, currentStartLength + currentSolidLineLength, segLength);
        // mark it's the end point
        renderFlag = 2;
        s_ti_sub = s_ti[segID] + currentStartLength + currentSolidLineLength;
	}
}
