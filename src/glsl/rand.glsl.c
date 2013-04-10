uvec2 next;

void srand(vec2 seed)
{
    next = floatBitsToUint(seed);
}

vec2 rand()
{
    const uint a = 1664525u;
    const uint c = 1013904223u;
    next = next * a + c;
    return vec2 (next) / vec2(4294967296.0);
}
