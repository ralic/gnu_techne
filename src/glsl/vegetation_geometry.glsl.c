<@begin main @>
layout(points) in;
layout(points, max_vertices = 1) out;

in vec3 color_v[];
in float score_v[];
in int index_v[];
in unsigned int instance_v[];
in vec3 apex_v[], left_v[], right_v[];

<@ for i = 0, context.species - 1 do @>
layout(stream = <@= i @>) out stream_<@= i @> {
    vec3 stream_<@= i @>_color_g;
    vec3 stream_<@= i @>_apex_g, stream_<@= i @>_left_g, stream_<@= i @>_right_g;
    float stream_<@= i @>_score_g;
    float stream_<@= i @>_clustering_g;
    unsigned int stream_<@= i @>_instance_g;
};
<@ end @>

uniform float clustering;

void main()
{
    /* Sort the clusters by species. */

    switch (index_v[0]) {
    <@ for i = 0, context.species - 1 do @>
    case <@= i @>:
        stream_<@= i @>_color_g = color_v[0];
        stream_<@= i @>_score_g = score_v[0];
        stream_<@= i @>_apex_g = apex_v[0];
        stream_<@= i @>_left_g = left_v[0];
        stream_<@= i @>_right_g = right_v[0];
        stream_<@= i @>_clustering_g = clustering;
        stream_<@= i @>_instance_g = instance_v[0];

        EmitStreamVertex(<@= i @>);
        EndStreamPrimitive(<@= i @>);

        break;
    <@ end @>
    }
}
<@finish @>
