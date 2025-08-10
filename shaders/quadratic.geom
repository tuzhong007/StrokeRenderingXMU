#version 450 core
layout(lines) in;
layout(triangle_strip, max_vertices = 9) out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float w;

layout(location = 0) in vec3 color[];
layout(location = 1) in float parameterT[];
layout(location = 2) in vec2 P0[];
layout(location = 3) in vec2 P1[];
layout(location = 4) in vec2 P2[];
layout(location = 5) in int segID[];

out vec2 gTexCoord; // Pass texture coordinates to fragment shader
out vec2 hTexCoord;
out vec3 gColor;    // Pass color to fragment shader
flat out int isConvex;

// Pass intersection flag to fragment shader, indicating if the p1'' is in triangle p0', p1', p2'. 
//If not, the implicit function evaluation (using hTexCoord) need not be done for the triangle p0', p1', p2, hence reducing computation load
flat out int isIntersected; 

// Function to calculate barycentric coordinates
vec3 barycentricCoordinate(vec2 p0, vec2 p1, vec2 p2, vec2 p) {
	float area = ((p1.x - p0.x) * (p2.y - p0.y) - (p2.x - p0.x) * (p1.y - p0.y));
    // if p0, p1, p2 are collinear, or area = 0, return a convenient value
    if (abs(area) < 1e-3) {
        return vec3(0, 0, 0);
    }
	float alpha = ((p1.x - p.x) * (p2.y - p.y) - (p2.x - p.x) * (p1.y - p.y)) / area;
	float beta = -((p0.x - p.x) * (p2.y - p.y) - (p2.x - p.x) * (p0.y - p.y)) / area;
	float gamma = 1.0f - alpha - beta;
	return vec3(alpha, beta, gamma);
}

void generateConvexBoundary(vec2 p0, vec2 p1, vec2 p2)
{
    vec3 color0 = (((segID[0] + segID[1]) % 4) == 1) ? vec3(0,1,1) : vec3(0,0,1);
    //vec3 color0 = vec3(0, 0, 0);
    vec3 color1 = color0, color2 = color0;
    float w_half = w / 2.f;

    vec2 tg0 = p1 - p0, tg2 = p2 - p1;
    // unit normal vectors at p0, p2
    vec2 n0 = normalize(vec2(-tg0.y, tg0.x)), n2 = normalize(vec2(-tg2.y, tg2.x));
    // make sure the normal vector is pointing outside
    if (dot(n0, tg2) > 0) n0 = -n0;
    if (dot(n2, tg0) < 0) n2 = -n2;
    float cosPhi = dot(tg0, tg2) / length(tg0) / length(tg2);
    float cosPhi_2 = sqrt((cosPhi + 1.0) / 2);
    // offset of p1
    vec2 d;
    // if tg0 and tg2 are parallel, the offset is w_half * n0
    bool isDegenerate = (abs(cosPhi) > 1 - 1e-3);
    if (isDegenerate) {
		d = w_half * n0;
	} else {
		// offset of p1
		d = (n0 + n2) / length(n0 + n2) * w_half / cosPhi_2;
	}


    // p0'', p1'', p2''
    vec2 p0_pp = p0 - w_half * n0, p1_pp = p1 - d, p2_pp = p2 - w_half * n2;
    // p0', p1', p2'
    vec2 p0_p = p0 + w_half * n0, p1_p = p1 + d, p2_p = p2 + w_half * n2;
    vec2 tg0_pp = p1_pp - p0_pp, tg2_pp = p2_pp - p1_pp;
    bool isReversed;
    if (dot(tg0, tg0_pp) < 0 || dot(tg2, tg2_pp) < 0){
    		isReversed = true;
	} else {
		isReversed = false;
	}
    // calculate the barycentric coordinates of p0', p1', p2' in the concave triangle p0'', p1'', p2''
    // for the signed distance calculation of the offset curve in the triangle p0'', p1'', p2''
    vec3 baryP0_Prime = barycentricCoordinate(p0_pp, p1_pp, p2_pp, p0_p);
    vec3 baryP1_Prime = barycentricCoordinate(p0_pp, p1_pp, p2_pp, p1_p);
    vec3 baryP2_Prime = barycentricCoordinate(p0_pp, p1_pp, p2_pp, p2_p);

    // calculate the barycentric coordinates of p1'' in the convex triangle p0', p1', p2'
    // for the intersection check of the offset curve O2(t) and the triangle p0', p1', p2'
    vec3 baryP1pp_p0p_p1p_p2p = barycentricCoordinate(p0_p, p1_p, p2_p, p1_pp);
    if (baryP1pp_p0p_p1p_p2p.x < 0 || baryP1pp_p0p_p1p_p2p.y < 0 || baryP1pp_p0p_p1p_p2p.z < 0)
    {
        // p1'' is outside the triangle p0', p1', p2'
        isIntersected = 0;
    }
    else
    {
        isIntersected = 1;
    }
    // curved triangles
    // 1st
    // p0'
    gl_Position = projection * view * model * vec4(p0_p, 0, 1);
    gTexCoord = vec2(0, 0);
    hTexCoord = baryP0_Prime.y * vec2(0.5, 0) + baryP0_Prime.z * vec2(1, 1);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		hTexCoord = vec2(1, 0);
	}
    gColor = color0;
    isConvex = 1;
    EmitVertex();
    // p1'
    gl_Position = projection * view * model * vec4(p1_p, 0, 1);
    gTexCoord = vec2(0.5, 0);
    hTexCoord = baryP1_Prime.y * vec2(0.5, 0) + baryP1_Prime.z * vec2(1, 1);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		hTexCoord = vec2(1, 0);
	}
    gColor = color1;
    isConvex = 1;
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2_p, 0, 1);
    gTexCoord = vec2(1, 1);
    hTexCoord = baryP2_Prime.y * vec2(0.5, 0) + baryP2_Prime.z * vec2(1, 1);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		hTexCoord = vec2(1, 0);
	}
    gColor = color2;
    isConvex = 1;
    EmitVertex();
    EndPrimitive();


    
    
    // linear triangles

    // 1st
    // p0'
    gl_Position = projection * view * model * vec4(p0_p, 0, 1);
    //gTexCoord = vec2(0.5, 0.5);
    gTexCoord = baryP0_Prime.y * vec2(0.5, 0) + baryP0_Prime.z * vec2(1, 1);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		gTexCoord = vec2(1, 0);
	}
    gColor = color0;
    isConvex = 0;
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2_p, 0, 1);
    //gTexCoord = vec2(0.5, 0.5);
    gTexCoord = baryP2_Prime.y * vec2(0.5, 0) + baryP2_Prime.z * vec2(1, 1);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		gTexCoord = vec2(1, 0);
	}
    gColor = color2;
    isConvex = 0;
    EmitVertex();
    // p0''
    gl_Position = projection * view * model * vec4(p0 - w_half * n0, 0, 1);
    //gTexCoord = vec2(0.5, 0.5);
    gTexCoord = vec2(0, 0);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		gTexCoord = vec2(1, 0);
	}
    gColor = color0;
    isConvex = 0;
    EmitVertex();
    EndPrimitive();

    //2nd
    // p0''
    gl_Position = projection * view * model * vec4(p0 - w_half * n0, 0, 1);
    //gTexCoord = vec2(0.5, 0.5);
    gTexCoord = vec2(0, 0);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		gTexCoord = vec2(1, 0);
	}
    gColor = color0;
    isConvex = 0;
    EmitVertex();
    // p2''
    gl_Position = projection * view * model * vec4(p2 - w_half * n2, 0, 1);
    //gTexCoord = vec2(0.5, 0.5);
    gTexCoord = vec2(1, 1);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		gTexCoord = vec2(1, 0);
	}
    gColor = color2;
    isConvex = 0;
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2_p, 0, 1);
    //gTexCoord = vec2(0.5, 0.5);
    gTexCoord = baryP2_Prime.y * vec2(0.5, 0) + baryP2_Prime.z * vec2(1, 1);
    if (isReversed || isDegenerate) {
        // edge case, render full color
		gTexCoord = vec2(1, 0);
	}
    gColor = color2;
    isConvex = 0;
    EmitVertex();
    EndPrimitive();
}

vec2 quadraticBezier(vec2 p0, vec2 p1, vec2 p2, float t) {
	return (1.0 - t) * (1.0 - t) * p0 + 2.0 * (1.0 - t) * t * p1 + t * t * p2;
}

// get the intersection point for two lines: a + alpha*b and c + beta*d, with alpha, beta in R
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
    vec2 p0, p1, p2;
    p0 = quadraticBezier(P0[0], P1[0], P2[0], parameterT[0]);
    p2 = quadraticBezier(P0[1], P1[1], P2[1], parameterT[1]);
    // get tangent vectors at p0 and p2 by derivative
    vec2 p0p1_direction = 2 * (1 - parameterT[0]) * (P1[0] - P0[0]) + 2 * parameterT[0] * (P2[0] - P1[0]);
    vec2 p1p2_direction = 2 * (1 - parameterT[1]) * (P1[1] - P0[1]) + 2 * parameterT[1] * (P2[1] - P1[1]);
    // p1 is the intersection point of the two tangents
    p1 = getIntersection(p0, p0p1_direction, p2, p1p2_direction);
    
    generateConvexBoundary(p0, p1, p2);
}
