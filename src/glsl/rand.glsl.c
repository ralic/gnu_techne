uvec2 next;

void srand(uvec2 seed)
{
    next = seed;
}

vec2 rand(void)
{
    const uvec2 a = uvec2(1103515245u, 1013904223u);
    const uvec2 c = uvec2(12345u, 1u);
    next = (next * a + c) & uvec2(2147483647u, 4294967295u);
    return vec2 (next) / vec2(1073741824.0, 2147483648.0) - 1.0;
}
