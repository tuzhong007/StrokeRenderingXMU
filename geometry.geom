#version 450 core
layout(lines) in;
layout(triangle_strip, max_vertices = 12) out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isStencil;

layout(location = 0) in vec3 color[];
layout(location = 1) in float parameterT[];
layout(location = 2) in vec2 P0[];
layout(location = 3) in vec2 P1[];
layout(location = 4) in vec2 P2[];
layout(location = 5) in int segID[];

out vec2 gTexCoord; // Pass texture coordinates to fragment shader
out vec3 gColor;    // Pass color to fragment shader
out vec2 posInLocalSpace; // position of the vertex in local space
out vec4 p0d0;
out vec4 p1d1;
out vec4 p2d2;
out vec2 initParameters; // initial value in Newton iteration


void generateConvexBoundary(vec2 p0, vec2 p1, vec2 p2)
{
    float w = 50.0;
    vec3 color0 = (((segID[0] + segID[1]) % 4) == 1) ? vec3(1,0,0) : vec3(0,1,0);
    vec3 color1 = color0, color2 = color0;


    vec2 tg0 = p1 - p0, tg2 = p2 - p1;
    // unit normal vectors at p0, p2
    vec2 n0 = normalize(vec2(-tg0.y, tg0.x)), n2 = normalize(vec2(-tg2.y, tg2.x));
    // make sure the normal vector is pointing outside
    if (dot(n0, tg2) > 0) n0 = -n0;
    if (dot(n2, tg0) < 0) n2 = -n2;
    float cosPhi = dot(tg0, tg2) / length(tg0) / length(tg2);
    float cosPhi_2 = sqrt((cosPhi + 1.0) / 2);
    // offset of p1
    vec2 d = (n0 + n2) / length(n0 + n2) * w / cosPhi_2;

    p0d0 = vec4(p0, w * n0), p1d1 = vec4(p1, d), p2d2 = vec4(p2, w * n2);

    // curved triangle
    // 1st
    // p0'
    gl_Position = projection * view * model * vec4(p0 + w * n0, 0, 1);
    gTexCoord = vec2(0, 0);
    gColor = color0;
    posInLocalSpace = p0 + w * n0;
    initParameters = vec2(0, 1);
    EmitVertex();
    // p1'
    gl_Position = projection * view * model * vec4(p1 + d, 0, 1);
    gTexCoord = vec2(0.5, 0);
    gColor = color1;
    posInLocalSpace = p1 + d;
    initParameters = vec2(0.5, 1);
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2 + w * n2, 0, 1);
    gTexCoord = vec2(1, 1);
    gColor = color2;
    posInLocalSpace = p2 + w * n2;
    initParameters = vec2(1, 1);
    EmitVertex();
    EndPrimitive();


    // linear triangles
    // 1st
    // p0'
    gl_Position = projection * view * model * vec4(p0 + w * n0, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color0;
    posInLocalSpace = p0 + w * n0;
    initParameters = vec2(0, 1);
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2 + w * n2, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color2;
    posInLocalSpace = p2 + w * n2;
    initParameters = vec2(1, 1);
    EmitVertex();
    // p0''
    gl_Position = projection * view * model * vec4(p0 - w * n0, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color0;
    posInLocalSpace = p0 - w * n0;
    initParameters = vec2(0, 0);
    EmitVertex();
    EndPrimitive();

    //2nd
    // p0''
    gl_Position = projection * view * model * vec4(p0 - w * n0, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color0;
    posInLocalSpace = p0 - w * n0;
    initParameters = vec2(0, 0);
    EmitVertex();
    // p2''
    gl_Position = projection * view * model * vec4(p2 - w * n2, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color2;
    posInLocalSpace = p2 - w * n2;
    initParameters = vec2(1, 0);
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2 + w * n2, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color2;
    posInLocalSpace = p2 + w * n2;
    initParameters = vec2(1, 1);
    EmitVertex();
    EndPrimitive();
}

void generateConcaveBoundary(vec2 p0, vec2 p1, vec2 p2)
{
    float w = 50.0;
    vec3 color0 = (((segID[0] + segID[1]) % 4) == 1) ? vec3(1,0,0) : vec3(0,1,0);
    vec3 color1 = color0, color2 = color0;

    vec2 tg0 = p1 - p0, tg2 = p2 - p1;
    // unit normal vectors at p0, p2
    vec2 n0 = normalize(vec2(-tg0.y, tg0.x)), n2 = normalize(vec2(-tg2.y, tg2.x));
    // make sure the normal vector is pointing outside
    if (dot(n0, tg2) > 0) n0 = -n0;
    if (dot(n2, tg0) < 0) n2 = -n2;
    float cosPhi = dot(tg0, tg2) / length(tg0) / length(tg2);
    float cosPhi_2 = sqrt((cosPhi + 1.0) / 2);
    // offset of p1
    vec2 d = (n0 + n2) / length(n0 + n2) * w / cosPhi_2;

    // curved triangle
    //2nd
    // p0''
    gl_Position = projection * view * model * vec4(p0 - w * n0, 0, 1);
    gTexCoord = vec2(0, 0);
    gColor = color0;
    EmitVertex();
    // p1''
    gl_Position = projection * view * model * vec4(p1 - d, 0, 1);
    gTexCoord = vec2(0.5, 0);
    gColor = color1;
    EmitVertex();
    // p2''
    gl_Position = projection * view * model * vec4(p2 - w * n2, 0, 1);
    gTexCoord = vec2(1, 1);
    gColor = color2;
    EmitVertex();
    EndPrimitive();
}

vec2 quadraticBezier(vec2 p0, vec2 p1, vec2 p2, float t) {
	return (1.0 - t) * (1.0 - t) * p0 + 2.0 * (1.0 - t) * t * p1 + t * t * p2;
}

mat2 inverse(mat2 m) {
    // Calculate the determinant
    float determinant = m[0][0] * m[1][1] - m[0][1] * m[1][0];

    // Check if the determinant is not zero (to avoid division by zero)
    if (determinant == 0.0) {
        // Return identity matrix if determinant is zero
        return mat2(1.0);
    }

    // Calculate the inverse
    return (1.0 / determinant) * mat2(
        m[1][1], -m[0][1],
        -m[1][0], m[0][0]
    );
}

// get the intersection point for two lines: a + alpha*b and c + beta*d, with alpha, beta in R
vec2 getIntersection(vec2 a, vec2 b, vec2 c, vec2 d) {
	vec2 alpha_beta = inverse(mat2(b, -d)) * (c - a);
    return a + alpha_beta.x * b;
}

void debugTriangle(vec2 p0, vec2 p1, vec2 p2) {
    gl_Position = projection * view * model * vec4(p0, 0, 1);
	gTexCoord = vec2(0, 0);
	gColor = vec3(1, 0, 0);
	EmitVertex();
	gl_Position = projection * view * model * vec4(p1, 0, 1);
	gTexCoord = vec2(0.5, 0);
	gColor = vec3(1, 0, 0);
	EmitVertex();
	gl_Position = projection * view * model * vec4(p2, 0, 1);
	gTexCoord = vec2(1, 1);
	gColor = vec3(1, 0, 0);
	EmitVertex();
	EndPrimitive();

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
    if (!isStencil) generateConvexBoundary(p0, p1, p2);
    else generateConcaveBoundary(p0, p1, p2);
}
