uvec2 next;

void srand(uvec2 seed)
{
    next = seed;
}

vec2 hash(uvec2 U, unsigned int k)
{
    uvec2 Uprime;
    const unsigned int M = 0xCD9E8D57;
    int i;

    for (i = 0 ; i < 3 ; i += 1) {
        umulExtended(U.x, M, Uprime.x, Uprime.y);
        
        U = uvec2(Uprime.x ^ U.y ^ k, Uprime.y);
        k += 0x9E3779B9;
    }

    next = U;
    return vec2(U) / 4294967295.0 ;
}

vec2 rand()
{
    const uint a = 1664525u;
    const uint c = 1013904223u;
    next = next * a + c;
    return vec2 (next) / vec2(4294967295.0);
}
