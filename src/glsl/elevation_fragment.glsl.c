out vec4 output;
in vec2 uv;

uniform sampler2D base;

/* uniform elevation_fragment { */
/* } */

void main()
{
    output = texture2D(base, uv);
}
