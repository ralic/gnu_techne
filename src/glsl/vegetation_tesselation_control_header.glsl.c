layout(vertices = 1) out;

in seed_attributes {
    vec3 position, color;
    int index;
} seed[1];

out plant_attributes {
    vec3 position;
} plant[1];

patch out vec3 color_tc;
patch out int index_tc;
