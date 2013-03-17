layout(triangles, equal_spacing) in;

in plant_attributes {
    vec3 position;
} plant[1];

patch in vec3 color;
patch in int index;

out vec3 shade;
