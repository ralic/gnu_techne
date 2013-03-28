layout(vertices = 1) out;

in vec3 position[], color[];
in int index[];

out vec3 position_tc[];

patch out vec3 color_tc;
patch out int index_tc;
