uvec2 next;

const float UINT_MAX = ldexp(1, 32);

void srand(uvec2 seed)
{
    next = seed;
}

uvec2 srand()
{
    return next;
}

vec2 rand2(uvec2 seed)
{
    next = seed;
    return vec2 (next) / vec2(UINT_MAX);
}

vec2 hash(uvec2 U, unsigned int k)
{
    uvec2 Uprime;
    const unsigned int M = 0xCD9E8D57;
    int i;

    for (i = 0 ; i < 4 ; i += 1) {
        umulExtended(U.x, M, Uprime.x, Uprime.y);

        U = uvec2(Uprime.x ^ U.y ^ k, Uprime.y);
        k += 0x9E3779B9;
    }

    next = U;
    return vec2(U) / UINT_MAX ;
}

vec2 rand2()
{
    const uint a = 1664525u;
    const uint c = 1013904223u;
    next = next * a + c;
    return vec2 (next) / vec2(UINT_MAX);
}

vec4 rand4()
{
    const uvec2 a = {1664525u, 69069u};
    const uvec2 c = {1013904223u, 1u};
    uvec4 new = next.xyxy * a.xxyy + c.xxyy;
    next = new.xz;
    return vec4 (new) / vec4(UINT_MAX);
}
