layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;
              
in vec3 color_te[];
in vec4 position_te[];
flat in int index_te[];
out vec3 color_g;
