#version 450 core
out vec4 FragColor;

in vec2 gTexCoord;
in vec2 hTexCoord;
in vec3 gColor;
flat in int isConvex;
flat in int isIntersected;
in vec2 posInLocalSpace; // position of the vertex in local space
in vec4 p0d0;
in vec4 p1d1;
in vec4 p2d2;
in vec2 initParameters;
in float segL;
in float lastTexCoord;
//in float currentStartLength[2];

uniform bool isStencil;
uniform sampler2D tex0;
uniform float l1;
uniform float w;



vec2 Psi(float u, float v, vec2 p0, vec2 d0, vec2 p1, vec2 d1, vec2 p2, vec2 d2)
{
	float v2_1 = 2*v-1;
	return (1-u)*(1-u) * (p0 + v2_1 * d0) + 2*u*(1-u) * (p1 + v2_1 * d1) + u*u * (p2 + v2_1 * d2);
}

vec2 solveUVByNewton()
{
	float u, v, v2_1;
	vec2 p0, d0, p1, d1, p2, d2, uv, upvp;
	vec2 dPsi_du, dPsi_dv, PsiUV;
	mat2 JPsi, inverseJPsi;
	p0 = p0d0.xy, d0 = p0d0.zw, p1 = p1d1.xy, d1 = p1d1.zw, p2 = p2d2.xy, d2 = p2d2.zw;
	upvp = initParameters;

	float ratio = segL / w;
	for (int i = 0; i < 1; ++i)
	{
		uv = vec2(upvp.x / ratio, upvp.y);
		u = uv.x, v = uv.y;
		v2_1 = 2*v-1;
		dPsi_du = 2*(1-u) * ((p1 + v2_1 * d1) - (p0 + v2_1 * d0)) + 2*u * ((p2 + v2_1 * d2) - (p1 + v2_1 * d1));
		dPsi_du /= ratio;
		dPsi_dv = (1-u)*(1-u) * (2 * d0) + 2*u*(1-u) * (2 * d1) + u*u * (2 * d2);

		PsiUV = Psi( u,  v,  p0,  d0,  p1,  d1,  p2,  d2);

		vec2 r = posInLocalSpace - PsiUV;
		float a = dPsi_du.x;
		float b = dPsi_du.y;
		float c = dPsi_dv.x;
		float d = dPsi_dv.y;
		float det = a * d - b * c;

		// Optional: avoid dividing by zero
		if (abs(det) > 1e-6) {
			highp float delta_u = (r.x * d - c * r.y) / det;
			highp float delta_v = (a * r.y - r.x * b) / det;
			upvp += vec2(delta_u, delta_v);
		}
		//upvp = inverseJPsi * (posInLocalSpace - PsiUV) + upvp;
	}


	return upvp;
	
}

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
	//FragColor = vec4(gColor, 1);


	// uv for parametrization
	uv = solveUVByNewton();
	float u = uv.x, v = uv.y;
	u = lastTexCoord + u, v = v;
	float epsilon = 0.02;
	float interval = 0.1;

	FragColor = texture(tex0, vec2(u, v)) * alpha;
	// uncomment the following to draw the isoparametric lines
//	if (abs(mod(u, interval)) < epsilon || abs(mod(v, interval)) < epsilon) {
//		FragColor = vec4(0,0,0, 1);
//	}
	
}