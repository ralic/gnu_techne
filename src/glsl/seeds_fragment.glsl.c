flat in vec3 color_te;
out vec4 fragment;

uniform vec3 intensity;

uniform seeds_fragment {
    vec3 color;
};

void main()
{
    fragment = vec4(color * intensity * color_te, 1);
}
