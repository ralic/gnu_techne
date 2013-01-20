uniform sampler2D texture;

out vec4 output;
in vec2 uv;

void main()
{
    output = texture2D(texture, uv);
}
