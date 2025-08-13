#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aColor;
layout (location = 3) in float aPrevArcLengthPiecewiseQuadratic;

out vec2 TexCoord;
out vec3 color;
out float prevArcLengthPiecewiseQuadratic;

void main()
{
    gl_Position = vec4(aPos, 1.0f);
    TexCoord = aTexCoord;
    color = aColor;
    prevArcLengthPiecewiseQuadratic = aPrevArcLengthPiecewiseQuadratic;
}