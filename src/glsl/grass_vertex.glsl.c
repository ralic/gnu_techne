in vec3 positions;
in vec3 normals;
in float sizes;

uniform sampler2D base;
uniform vec3 references[N], weights[N];
uniform float power;

uniform float scale;
uniform vec2 offset;

uniform public_attributes {
    int amplification;
};

out cluster_attributes {
    vec3 center, normal;
    float size;
    int counts[N];
} cluster;

void main()
{
    vec3 texel, hsv;
    vec2 uv;
    float distances[N], D, r, z;
    int i;

    uv = scale * positions.xy - offset;
    texel = vec3(texture2D(base, uv));
    hsv = rgb_to_hsv(texel);
    
    for (i = 0, D = 0 ; i < N ; i += 1) {
        distances[i] = hsv_distance (hsv, references[i], weights[i], power);
        D += distances[i];
    }

    z = min((modelview * vec4 (positions, 1)).z, -1);

    for (i = 0, r = 0 ; i < N ; i += 1) {
        float c_0, c;

        c_0 = distances[i] / D * amplification + r;
        c = round(c_0);
        r = c - c_0;

        cluster.counts[i] = int(c);
    }
    
    cluster.center = positions;
    cluster.normal = normals;
    cluster.size = sizes;
}