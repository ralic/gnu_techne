uniform sampler2D texture;

out vec4 fragment;
in vec2 uv;

void main()
{
    fragment = texture2D(texture, uv);
}
