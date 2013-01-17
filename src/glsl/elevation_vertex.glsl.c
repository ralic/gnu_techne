/* uniform elevation_vertex { */
/* }; */

in vec4 positions;
out vec2 uv;

uniform float scale;
uniform vec2 offset;

void main()
{
    gl_Position = projection * modelview * positions;
    uv = scale * positions.xy - offset;
}
