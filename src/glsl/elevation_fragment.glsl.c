out vec4 output;
in vec2 uv;

uniform sampler2D texture;

/* uniform elevation_fragment { */
/* } */

void main()
{
    output = texture2D(texture, uv);
}
