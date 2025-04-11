#version 450 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 12) out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool isStencil;

in vec2 TexCoord[];
in vec3 color[];

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
    gTexCoord = TexCoord[0];
    gColor = color[0];
    posInLocalSpace = p0 + w * n0;
    initParameters = vec2(0, 1);
    EmitVertex();
    // p1'
    gl_Position = projection * view * model * vec4(p1 + d, 0, 1);
    gTexCoord = TexCoord[1];
    gColor = color[1];
    posInLocalSpace = p1 + d;
    initParameters = vec2(0.5, 1);
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2 + w * n2, 0, 1);
    gTexCoord = TexCoord[2];
    gColor = color[2];
    posInLocalSpace = p2 + w * n2;
    initParameters = vec2(1, 1);
    EmitVertex();
    EndPrimitive();


    // linear triangles
    // 1st
    // p0'
    gl_Position = projection * view * model * vec4(p0 + w * n0, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color[0];
    posInLocalSpace = p0 + w * n0;
    initParameters = vec2(0, 1);
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2 + w * n2, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color[2];
    posInLocalSpace = p2 + w * n2;
    initParameters = vec2(1, 1);
    EmitVertex();
    // p0''
    gl_Position = projection * view * model * vec4(p0 - w * n0, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color[0];
    posInLocalSpace = p0 - w * n0;
    initParameters = vec2(0, 0);
    EmitVertex();
    EndPrimitive();

    //2nd
    // p0''
    gl_Position = projection * view * model * vec4(p0 - w * n0, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color[0];
    posInLocalSpace = p0 - w * n0;
    initParameters = vec2(0, 0);
    EmitVertex();
    // p2''
    gl_Position = projection * view * model * vec4(p2 - w * n2, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color[2];
    posInLocalSpace = p2 - w * n2;
    initParameters = vec2(1, 0);
    EmitVertex();
    // p2'
    gl_Position = projection * view * model * vec4(p2 + w * n2, 0, 1);
    gTexCoord = vec2(0.5, 0.5);
    gColor = color[2];
    posInLocalSpace = p2 + w * n2;
    initParameters = vec2(1, 1);
    EmitVertex();
    EndPrimitive();
}

void generateConcaveBoundary(vec2 p0, vec2 p1, vec2 p2)
{
    float w = 50.0;
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
    gTexCoord = TexCoord[0];
    gColor = color[0];
    EmitVertex();
    // p1''
    gl_Position = projection * view * model * vec4(p1 - d, 0, 1);
    gTexCoord = TexCoord[1];
    gColor = color[1];
    EmitVertex();
    // p2''
    gl_Position = projection * view * model * vec4(p2 - w * n2, 0, 1);
    gTexCoord = TexCoord[2];
    gColor = color[2];
    EmitVertex();
    EndPrimitive();
}


void main() {
    vec2 p0, p1, p2;
    p0 = gl_in[0].gl_Position.xy, p1 = gl_in[1].gl_Position.xy, p2 = gl_in[2].gl_Position.xy;
    if (!isStencil) generateConvexBoundary(p0, p1, p2);
    else generateConcaveBoundary(p0, p1, p2);
}
