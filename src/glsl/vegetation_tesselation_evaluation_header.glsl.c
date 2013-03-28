layout(isolines, equal_spacing) in;

in plant_attributes {
    vec3 position;
} plant[1];

patch in vec3 color_tc;
patch in int index_tc;

out vec3 color_te;
out flat int index_te;
