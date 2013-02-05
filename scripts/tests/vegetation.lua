-- Copyright (C) 2010-2011 Papavasileiou Dimitris                           
--                                                                      
-- This program is free software: you can redistribute it and/or modify 
-- it under the terms of the GNU General Public License as published by 
-- the Free Software Foundation, either version 3 of the License, or    
-- (at your option) any later version.                                  
--                                                                      
-- This program is distributed in the hope that it will be useful,      
-- but WITHOUT ANY WARRANTY; without even the implied warranty of       
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
-- GNU General Public License for more details.                         
--                                                                      
-- You should have received a copy of the GNU General Public License    
-- along with this program.  If not, see <http://www.gnu.org/licenses/>.

resources.dofile "common.lua"

graphics.perspective = {units.degrees(50), 0.1, 10000}

local heights = resources.dofile ("diamondsquare.lua", 512, 0.0012)
local base = {}

for i = 1, 128 do
   base[i] = {}

   for j = 1, 128 do
      base[i][j] = {
         (j >= 0 and j < 43) and 1 or 0,
         (j >= 44 and j < 88) and 1 or 0,
         (j >= 89 and j < 129) and 1 or 0
      }
   end
end

local red = textures.planar {
   texels = array.nuchars {
      {{1, 0, 0},{1, 0, 0}},
      {{1, 0, 0},{1, 0, 0}}
                          }
                            }

local green = textures.planar {
   texels = array.nuchars {
      {{0, 1, 0},{0, 1, 0}},
      {{0, 1, 0},{0, 1, 0}}
                          }
                              }

elevation = topography.elevation {
   depth = 9,
   resolution = {3000 / 512, 3000 / 512},

   tiles = {
      {
         {array.nushorts(heights), nil, array.nuchars(base), {500, 0}}
      }
   }
}

root = primitives.root {
   orbit = resources.dofile ("orbit.lua", -1000, 0, 0),

   atmosphere = topography.atmosphere {
      size = {1024, 512},

      turbidity = 3,

      rayleigh = {6.95e-06, 1.18e-05, 2.44e-05},
      mie = 7e-5,

      sun = {1.74, units.degrees(45)},
                                      },

   shader = topography.splat {
      tag = "elevation",

      index = -1,
      albedo = 1.5,
      separation = 1,

      palette = {
         {red, {1, 1}, {0, .99, .99}},
         {green, {1, 1}, {120 / 360, .99, .99}},
      },

      shape = elevation.shape {
         target = 15000,
                              }
                             },

   grass = topography.grass {
      tag = "vegetation",

      separation = 1,

      palette = {
         {{1, 1, 0}, {0, .99, .99}},
         {{0, 1, 1}, {120 / 360, .99, .99}},
      },
      
      shape = elevation.vegetation {
                                   }
                            },

   cameraman = options.timed and primitives.timer {
      period = 1,
      
      tick = function(self, ticks)
         local command = {-1000, units.degrees(65),
                          (ticks % 2 > 0 and 1 or -1) * units.degrees(90)}

         self.parent.orbit.command = command

         if ticks > 4 then
            techne.iterate = false
         end
      end,
                                }
                       }

bindings['h'] = function()
   root.wireframe.shader.shape.optimize = not root.wireframe.shader.shape.optimize
end
