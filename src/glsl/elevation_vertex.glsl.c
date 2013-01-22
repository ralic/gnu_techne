/* uniform elevation_vertex { */
/* }; */

in vec4 positions;
out vec3 eye;
out vec2 uv;

uniform float scale;
uniform vec2 offset;

void main()
{
    vec4 Mp;

    Mp = modelview * positions;
    
    gl_Position = projection * Mp;
    eye = vec3(Mp);
    uv = scale * positions.xy - offset;
}
