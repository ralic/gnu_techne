layout(isolines, equal_spacing) in;

in vec3 position_tc[];

patch in vec3 color_tc;
patch in int index_tc;

out vec3 color_te;
out vec4 position_te;
flat out int index_te;
