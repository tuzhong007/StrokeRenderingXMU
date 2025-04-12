#version 450 core
out vec4 FragColor;

in vec2 gTexCoord;
in vec3 gColor;
in vec2 posInLocalSpace; // position of the vertex in local space
in vec4 p0d0;
in vec4 p1d1;
in vec4 p2d2;
in vec2 initParameters;

uniform bool isStencil;
uniform sampler2D tex0;


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

vec2 Psi(float u, float v, vec2 p0, vec2 d0, vec2 p1, vec2 d1, vec2 p2, vec2 d2)
{
	float v2_1 = 2*v-1;
	return (1-u)*(1-u) * (p0 + v2_1 * d0) + 2*u*(1-u) * (p1 + v2_1 * d1) + u*u * (p2 + v2_1 * d2);
}

void solveUVByNewton()
{
	float u, v, v2_1;
	vec2 p0, d0, p1, d1, p2, d2, uv;
	vec2 dPsi_du, dPsi_dv, PsiUV;
	mat2 JPsi, inverseJPsi;
	p0 = p0d0.xy, d0 = p0d0.zw, p1 = p1d1.xy, d1 = p1d1.zw, p2 = p2d2.xy, d2 = p2d2.zw;
	uv = initParameters;
	//uv = vec2(0.5,0.5);

	/*for (int i = 0; i < 9; ++i)
	{
		u = uv.x, v = uv.y;
		v2_1 = 2*v-1;
		dPsi_du = 2*(1-u) * ((p1 + v2_1 * d1) - (p0 + v2_1 * d0)) + 2*u * ((p2 + v2_1 * d2) - (p1 + v2_1 * d1));
		dPsi_dv = (1-u)*(1-u) * (p0 + 2 * d0) + 2*u*(1-u) * (p1 + 2 * d1) + u*u * (p2 + 2 * d2);
		PsiUV = Psi( u,  v,  p0,  d0,  p1,  d1,  p2,  d2);
		JPsi = mat2(dPsi_du, dPsi_dv);
		inverseJPsi = inverse(JPsi);
		uv = inverseJPsi * (posInLocalSpace - PsiUV) + uv;
	}*/

	
	do
	{
		u = uv.x, v = uv.y;
		v2_1 = 2*v-1;
		dPsi_du = 2*(1-u) * ((p1 + v2_1 * d1) - (p0 + v2_1 * d0)) + 2*u * ((p2 + v2_1 * d2) - (p1 + v2_1 * d1));
		dPsi_dv = (1-u)*(1-u) * (p0 + 2 * d0) + 2*u*(1-u) * (p1 + 2 * d1) + u*u * (p2 + 2 * d2);
		PsiUV = Psi( u,  v,  p0,  d0,  p1,  d1,  p2,  d2);
		JPsi = mat2(dPsi_du, dPsi_dv);
		inverseJPsi = inverse(JPsi);
		uv = inverseJPsi * (posInLocalSpace - PsiUV) + uv;
	} while (length(PsiUV - posInLocalSpace) > 0.5); 


	u = uv.x, v = uv.y;
	float epsilon = 0.01;
	float interval = 0.1;
	if (abs(mod(u, interval)) < epsilon || abs(mod(v, interval)) < epsilon) {
    // texCoord is a multiple of 0.2
		FragColor = vec4(0,0,0, 1);
	}

	FragColor = texture(tex0, vec2(u * 3, v));
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
	if (sd > 0.5) discard;
	if (isStencil) return;

//	float alpha = clamp(-sd + 0.5, 0.0, 1.0);
	float alpha = clamp(-sd + 0.5, 0.0, 1.0);
	
	FragColor = vec4(gColor, alpha);
	//FragColor = vec4(gColor, 1);


	// uv for parametrization
	 //solveUVByNewton();



}