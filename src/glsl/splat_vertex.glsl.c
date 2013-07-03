in vec3 positions;
out vec3 eye;
out vec2 uv;

uniform vec2 offset, scale;

void main()
{
    vec4 Mp;

    Mp = modelview * vec4(positions, 1);
    
    gl_Position = projection * Mp;
    eye = vec3(Mp);
    uv = scale * positions.xy + offset;
}
