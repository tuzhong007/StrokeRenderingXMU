#version 450 core
out vec4 FragColor;

in vec2 gTexCoord;
in vec2 hTexCoord;
in vec3 gColor;
flat in int isConvex;
flat in int isIntersected;

uniform sampler2D tex0;
uniform float w;

void main()
{
	// uv for implicit equation
	vec2 uv = gTexCoord;
	vec2 uxvx = dFdx(gTexCoord); //anti-aliasing
	vec2 uyvy = dFdy(gTexCoord);

	float g, gx, gy;
	g = uv.x * uv.x - uv.y;
	gx = 2 * uv.x * uxvx.x -  uxvx.y;
	gy = 2 * uv.x * uyvy.x -  uyvy.y;

	float sd = g / sqrt(gx * gx + gy * gy);
	if (isConvex == 0) sd = -sd; 
	else {
		// if the offset curve O2(t) and the triangle p0', p1', p2' are intersected
		if (isIntersected == 1){
			// sd to the lower offset curve
			uv = hTexCoord;
			uxvx = dFdx(hTexCoord); //anti-aliasing
			uyvy = dFdy(hTexCoord);

			float h, hx, hy;
			h = uv.x * uv.x - uv.y;
			hx = 2 * uv.x * uxvx.x -  uxvx.y;
			hy = 2 * uv.x * uyvy.x -  uyvy.y;

			float sd2 = -h / sqrt(hx * hx + hy * hy);
			sd = max(sd, sd2);
		}
	}
	if (sd > 0.5) discard;
	//if (isStencil) return;

//	float alpha = clamp(-sd + 0.5, 0.0, 1.0);
	float alpha = clamp(-sd + 0.5, 0.0, 1.0);
	
	FragColor = vec4(gColor, alpha);
}