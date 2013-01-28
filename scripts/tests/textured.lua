resources.dofile "common.lua"

graphics.perspective = {45, 0.1, 10000}
dynamics.timescale = 0

local pixels = {}

for i = 1, 256 do
   pixels[i] = {}
   for j = 1, 256 do
      pixels[i][j] = {i / 256, j / 256, 0}
   end
end

root = primitives.root {
   textured = shading.textured {
      position = {0, 0, -1},

      texture = textures.planar {
         texels = array.nuchars(pixels)
                                },
      
      shape = shapes.rectangle {
         size = {1, 1}
                               }
                           }
                       }
