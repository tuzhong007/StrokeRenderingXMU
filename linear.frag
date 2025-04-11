#version 330 core
out vec4 FragColor;

in vec2 gTexCoord;
in vec3 gColor;

uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
	vec2 uv = gTexCoord;
	float g = uv.y - 1.0f; // if g < 0, then the fragment lies inside the curve

	vec2 uxvx = dFdx(gTexCoord); //the following is anti-aliasing
	vec2 uyvy = dFdy(gTexCoord);

	float gx = uxvx.y;
	float gy = uyvy.y;

	float sd = g / sqrt(gx * gx + gy * gy);

	float alpha = clamp(-sd + 0.5, 0.0, 1.0);

	FragColor = vec4(gColor, alpha);

	if (alpha <= 0)
	{
		FragColor = vec4(0, 0, 0, 0.2);
		//discard;
	}

/*	if (alpha > 0)
	{
		FragColor = vec4(gColor, alpha);
	}
	else
	{
		discard;
	}*/

}