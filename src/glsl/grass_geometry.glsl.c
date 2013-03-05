uniform Grass_parameters {
    float bias, segments, stiffness, height, width;
};

vec2 rand(void);

void Grass_main (mat4 M, mat4 P, vec3 c, vec3 n, vec3 s, vec3 t, float r, float d)
{
    const float pi = 3.1415926535;
    const float pi_2 = 1.5707963268;
    mat4 PM;
    vec4 p;
    vec2 u;
    float psi, cospsi, sinpsi, l, h, theta, k, x, y, z;
    int i, n_s;

    l = height * d;
    h = 0.5 * width * d;
    
    u = rand();
    psi = u.x * pi;
    cospsi = cos(psi); sinpsi = sin(psi);
    theta = 0.01;
    
    z = min((M * vec4 (c, 1)).z, -bias);
    n_s = int(max(-bias * segments / z, 1));
    k = stiffness * l * l;
    
    p = vec4(c + r * u.x * s + r * u.y * t, 1);
    PM = P * M;
    
    for (i = 0, x = y = 0 ; i < n_s ; i += 1) {
        float A, D, l_i, phi;

        l_i = l - i * l / n_s;
        A = l_i * l_i / k;
        D = 1 - 4 / 2.42 * A * (pi_2 - theta - A);
        phi = (1 - sqrt(D)) / (2 / 2.42 * A);

        if (phi > pi_2) {
            phi = pi - phi;
        }
        
        theta += pi_2 - theta - phi;
        
        gl_Position = PM * (p + vec4(x * cospsi - h * sinpsi,
                                    x * sinpsi + h * cospsi,
                                    y, 0));
        EmitVertex();

        gl_Position = PM * (p + vec4(x * cospsi + h * sinpsi,
                                    x * sinpsi - h * cospsi,
                                    y, 0));
        EmitVertex();

        x += l / n_s * cos(phi);
        y += l / n_s * sin(phi);
    }
    
    gl_Position = PM * (p + vec4(x * cospsi, x * sinpsi, y, 0));
    EmitVertex();

    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
