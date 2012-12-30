uniform sampler2D texture;

out vec4 outputColor;
in vec2 uv;

void main()
{
    outputColor = texture2D(texture, uv);
}
