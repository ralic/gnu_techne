uniform Grass_parameters {
    float bias, segments, stiffness, height, width;
};

vec2 rand(void);

void Grass_main (mat4 M, mat4 P, vec3 c, vec3 n, vec3 s, vec3 t, float r, float d)
{
    const float pi = 3.1415926535;
    const float pi_2 = 1.5707963268;
    const int n_s = 4;
    const vec4 sc = vec4 (0.625, 0.625, 1, 1);
    
    mat4 PM;
    vec4 p;
    vec2 u, nodes[n_s];
    float psi, cospsi, sinpsi, l, h, theta, k, x, y, z;
    int i, l_d;

    l = height * d;
    h = 0.5 * width * d;
    
    u = rand();
    psi = u.x * pi;
    cospsi = cos(psi); sinpsi = sin(psi);
    theta = 0;
    
    z = (M * vec4 (c, 1)).z;

    l_d = 1;//(1 << findMSB(1 + n_s - int(max(bias * bias * n_s / z / z, 1)))) - 1;
    /* l_d = 1-1; */
    /* l_d = gl_InvocationID == 0 ? 4 : 8; */
    /* l_d = z < 0 ? 4 : 1; */
    k = (1 + 0.1 * u.x) * l * l;

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

        x += l / n_s * cos(phi);
        y += l / n_s * sin(phi);

        nodes[i] = vec2(x, y);
    }
    
    gl_Position = PM * (p + sc * vec4(-h * sinpsi,
                                 h * cospsi,
                                 0, 0));
    EmitVertex();

    gl_Position = PM * (p + sc * vec4(h * sinpsi,
                                 -h * cospsi,
                                 0, 0));
    EmitVertex();
    
    for (i = 0 ; i < n_s ; i += 1) {
        if (((i + 1) & l_d) > 0) {
            continue;
        }

        if (i + 1 < n_s) {
            gl_Position = PM * (p + sc * vec4(nodes[i].x * cospsi - h * sinpsi,
                                         nodes[i].x * sinpsi + h * cospsi,
                                         nodes[i].y, 0));
            EmitVertex();

            gl_Position = PM * (p + sc * vec4(nodes[i].x * cospsi + h * sinpsi,
                                         nodes[i].x * sinpsi - h * cospsi,
                                         nodes[i].y, 0));
            EmitVertex();
        } else {
            gl_Position = PM * (p + sc * vec4(nodes[i].x * cospsi, x * sinpsi, nodes[i].y, 0));
            EmitVertex();
        }
    }
    
    gl_PrimitiveID = gl_PrimitiveIDIn;
    EndPrimitive();
}
