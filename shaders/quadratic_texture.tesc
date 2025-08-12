#version 450 core
// Max segment number for curvature tessellation
#define MAX_SEG 7

layout(vertices = 3) out;  // Triangle patches

uniform float l1; // the length of each solid line
uniform float l2; // the gap length of two solid lines

in vec2 TexCoord[];
in vec3 color[];

out vec2 tcsTexCoord[];
out vec3 tcsColor[];
//out float Phi[];
//out float cosPhi[];
//out int segments[]; // Number of segments for each edge
patch out float t_i[MAX_SEG + 1];
patch out float s_ti[MAX_SEG + 1];
// Number of solid lines in i-th segment
patch out int solidLines_Num_i[MAX_SEG];
//patch out vec2 p0[MAX_SEG + 1];
//patch out vec2 p1[MAX_SEG + 1];
//patch out vec2 p2[MAX_SEG + 1];

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
    // Pass attributes to TES
    
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

    // Set constant tessellation levels
    if (gl_InvocationID == 0) {
        vec2 P0 = gl_in[0].gl_Position.xy; 
        vec2 P1 = gl_in[1].gl_Position.xy;
        vec2 P2 = gl_in[2].gl_Position.xy;
        vec2 P0P1 = gl_in[1].gl_Position.xy - gl_in[0].gl_Position.xy;
        vec2 P1P2 = gl_in[2].gl_Position.xy - gl_in[1].gl_Position.xy;

        tcsTexCoord[gl_InvocationID] = TexCoord[gl_InvocationID];
        tcsColor[gl_InvocationID] = color[gl_InvocationID];
        // the tangent angle variation Phi
        float cosPhi = dot(normalize(P0P1), normalize(P1P2));
        float Phi = acos(cosPhi);
        // the segment numbers of curvature-tessellated quadratic curve
        int seg = int(ceil((Phi + 1e-3) / 0.5));
        // the parameters t_i and arc lengths s_t[i] for the endpoints
        t_i[0] = 0; s_ti[0] = 0;
        t_i[seg] = 1; s_ti[seg] = getArcLength_t(P0, P1, P2, 1);
        int maxSolidLineNum = -1;
        // properties for interior segment points obtained by curvature tessellation
        for (int i = 1; i < seg; ++i) {
            float u = 1.0 * i / seg;
            float A = 2 * length(P0P1), B = 2 * length(P1P2), C = cosPhi, D = cos(u * Phi);
            // calculate t_i from the correspoding angle relation: the angle between the tangent at t_i and the tangent at 0 is u * Phi
            vec2 t1t2 = calculate_ti_From_Phii(A, B, C, D);
            float t1 = t1t2.x, t2 = t1t2.y;
            float parameterT = 0.0;

            // determine whether t1 or t2 is the solution
            // a fast validity judge: check if t1 and t2 are in the range [0, 1]
            if (t1 < 0 || t1 > 1) {
                parameterT = t2;
            }
	        else if (t2 < 0 || t2 > 1) {
		        parameterT = t1;
	        }
            else {
                // check if the tangent at t1 and the tangent at 1 has the angle (1-u) * Phi
                // if so, t1 is the solution
                // otherwise, t2 is the solution
                vec2 tangent_t1 = 2 * (1 - t1) * P0P1 + 2 * t1 * P1P2;
                float angle_t1 = acos(dot(normalize(tangent_t1), normalize(P2 - P0)));

                if (abs(angle_t1 - (1-u) * Phi) < 1e-3) {
		            parameterT = t1;
	            }
	            else {
		            parameterT = t2;
	            }
            }
            
            // calculate the arc length s_t[i] from t=0 to t=t_i
            s_ti[i] = getArcLength_t(P0, P1, P2, parameterT);
            t_i[i] = parameterT;
            float segLength = s_ti[i] - s_ti[i-1];
            solidLines_Num_i[i-1] = int(ceil(segLength / (l1 + l2)))+1;
            maxSolidLineNum = max(maxSolidLineNum, solidLines_Num_i[i-1]);
        }
        float segLength = s_ti[seg] - s_ti[seg - 1];
        solidLines_Num_i[seg - 1] = int(ceil(segLength / (l1 + l2))) + 1;
        maxSolidLineNum = max(maxSolidLineNum, solidLines_Num_i[seg - 1]);

        // tessellate the bending curve into {seg} flat ones
        gl_TessLevelOuter[0] = seg;
        // just an upperbound of the solid line endpoint numbers per flat curve
        gl_TessLevelOuter[1] = maxSolidLineNum * 2; 
        
    }
}
