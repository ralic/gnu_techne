layout(points) in;
layout(triangle_strip, max_vertices = 100) out;

uniform sampler2D base, foo;
uniform vec3 references[N], weights[N];
uniform float power;

uniform float scale;
uniform vec2 offset;

uniform public_attributes {
    int amplification;
};
               
in cluster_attributes {
    vec3 center, normal;
    float size;
} cluster[1];

out vec4 color;

vec3 rgb_to_hsv (vec3);
float hsv_distance (vec3, vec3, vec3, float);
void srand(uvec2 seed);

void main()
{
    vec3 texel, hsv, n, s, t, c, p;
    vec2 uv;
    float distances[N], D, r, e;
    int counts[N], i, j;

    r = cluster[0].size;
    c = cluster[0].center;
    n = cluster[0].normal;

    /* ... */

    uv = scale * c.xy - offset;
    texel = vec3(texture2D(base, uv));
    hsv = rgb_to_hsv(texel);
    
    for (i = 0, D = 0 ; i < N ; i += 1) {
        distances[i] = hsv_distance (hsv, references[i], weights[i], power);
        D += distances[i];
    }
    
    for (i = 0, e = 0 ; i < N ; i += 1) {
        float c_0, c;

        distances[i] /= D;
        c_0 = distances[i] * amplification + e;
        c = round(c_0);
        e = c - c_0;

        counts[i] = int(c);
    }

    /* ... */
    
    if (n.z < 1.0) {
        s = normalize(vec3(n.y, -n.x, 0));
        t = cross (s, n);
    } else {
        s = vec3(1, 0, 0);
        t = vec3(0, 1, 0);
    }

    srand(floatBitsToUint (c.xy));

#define EXPAND_CLUSTER(FUNCTION, i)                                     \
    for (j = 0 ; j < counts[i] ; j += 1) {                              \
        color = 2 * (texel.r + texel.g + texel.b) / 3.0 * vec4(vec3(0.742141, 0.681327, 0.588593) * texture2D(foo, uv / 0.0025).rgb, 1.0); \
                                                                        \
        FUNCTION(modelview, projection, c, n, s, t, r, distances[i]);   \
    }

        /* Continued in vegetation_geometry_botom.glsl.c */
