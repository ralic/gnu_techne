in vec3 apex, left, right;

out vec3 color_v;
out float score_v;
out int index_v;
out vec3 apex_v, left_v, right_v;
out unsigned int instance_v;

uniform float factor, thresholds[SWATCHES];
uniform vec2 offset, scale;

vec2 hash(unsigned int R, unsigned int L, unsigned int k);
vec2 rand2();
uvec2 srand();

vec3 compose(vec2 uv);
float splat_score(vec3 hsv, int i);
vec3 cluster_center(vec3 apex, vec3 left, vec3 right, unsigned int instance);

#ifdef COLLECT_STATISTICS
layout(binding = 0, offset = 0) uniform atomic_uint infertile, fertile;
#endif

void main()
{
    vec3 rgb, hsv, c;
    vec2 uv, z;
    float s_0, s_1, r;
    int i, i_0, i_1;

    c = cluster_center(apex, left, right, uint(gl_InstanceID));

    /* Calculate scores for all terrain types. */

    uv = scale * c.xy + offset;
    rgb = compose(uv);
    hsv = rgb_to_hsv(rgb);

    for (i = 0, s_0 = s_1 = -infinity, i_0 = i_1 = -1;
         i < SWATCHES;
         i += 1) {
        float d;

        d = splat_score(hsv, i);

        /* Keep track of the two highest scores. */

        if (d > s_0) {
            s_1 = s_0;
            i_1 = i_0;

            s_0 = d;
            i_0 = i;
        } else if (d > s_1) {
            s_1 = d;
            i_1 = i;
        }
    }

    z = rand2();

    /* Randomly mix neighboring species based on their scores.  Don't
     * go for the second-best species if it wouldn't result in a seed
     * to avoid reducing plant density. */

    if (z.x < s_1 / (s_0 + s_1) && s_1 >= thresholds[i_1]) {
        index_v = i_1;
        score_v = s_1 / s_0;
    } else {
        /* Skip the rest if the seed is infertile. */

        if (s_0 < thresholds[i_0]) {
#ifdef COLLECT_STATISTICS
            atomicCounterIncrement(infertile);
#endif

            index_v = -1;
            return;
        }

        index_v = i_0;
        score_v = 1 - s_1 / s_0;
    }
    /* index_v = 0; score_v = 1; color_v = vec4(1, 0, 0, 1); */

#ifdef COLLECT_STATISTICS
    atomicCounterIncrement(fertile);
#endif

    color_v = factor * rgb;
    apex_v = apex; left_v = left; right_v = right;
    instance_v = uint(gl_InstanceID);
}
