layout(location=0) in vec3 positions;
layout(location=1) in vec3 normals;
layout(location=2) in float radius;

uniform sampler2D base;
uniform vec3 references[N], weights[N];
uniform float power;

uniform float scale;
uniform vec2 offset;

out cluster_attributes {
    vec3 center, normal;
    float size;
    int counts[N];
} cluster;

void main()
{
    vec3 texel, hsv;
    vec2 uv;
    float distances[N], D, r;
    int i;

    uv = scale * positions.xy - offset;
    texel = vec3(texture2D(base, uv));
    hsv = rgb_to_hsv(texel);
    
    for (i = 0, D = 0 ; i < N ; i += 1) {
        distances[i] = hsv_distance (hsv, references[i], weights[i], power);
        D += distances[i];
    }

    for (i = 0, r = 0 ; i < N ; i += 1) {
        float c_0, c;

        c_0 = distances[i] / D * 5 + r;
        c = round(c_0);
        r = c - c_0;

        cluster.counts[i] = int(c);
    }
    
    cluster.center = positions;
    cluster.normal = normals;
    cluster.size = radius;
}
