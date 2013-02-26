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

local io = require "io"
local math = require "math"
local array = require "array"
local arraymath = require "arraymath"
local resources = require "resources"
local graphics = require "graphics"
local primitives = require "primitives"
local topography = require "topography"
local shading = require "shading"
local textures = require "textures"
local units = require "units"
local bindings = require "bindings.default"

resources.dofile "utils/basic.lua"

graphics.perspective = {units.degrees(50), 0.1, 10000}

local file = not options.generate and io.open (".heightmap")
local heights

if not file then
   local m, M

   heights = array.nushorts(resources.dofile ("tests/diamondsquare.lua", 4096, 0.000125))
   m, M = arraymath.range(heights)
   heights = arraymath.scale(arraymath.offset (heights, -m), 1 / (M - m))

   file = io.open (".heightmap", "w")
   file:write (array.dump(heights))
else
   heights = array.nushorts(4097, 4097, file:read("*a"))
end

file:close()

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
   depth = 12,
   resolution = {0.625, 0.625},

   tiles = {
      {
         {heights, nil, array.nuchars(base), {375, 0}}
      }
   }
}

root = primitives.root {
   orbit = resources.dofile ("utils/orbit.lua", -1000, 0, units.degrees(71)),

   atmosphere = topography.atmosphere {
      size = {1024, 512},

      turbidity = 3,

      rayleigh = {6.95e-06, 1.18e-05, 2.44e-05},
      mie = 7e-5,

      sun = {1.74, units.degrees(45)},
                                      },

   wireframe = shading.wireframe {
      enabled = false,
      index = -1,

      splat = topography.splat {
         tag = "elevation",

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
                             },

   grass = topography.grass {
      tag = "seeds",

      amplification = 10,
      separation = 1,

      palette = {
         {{1, 1, 0}, {0, .99, .99}},
         {{1, 0.4, 0}, {120 / 360, .99, .99}},
      },
      
      shape = elevation.seeds {
         bias = 3,
         density = 1000,
                                   }
                            },

   cameraman = options.timed and primitives.timer {
      period = 2,
      
      tick = function(self, ticks)
         local command = {-1000,
                          (ticks % 2 > 0 and 1 or -1) * units.degrees(90),
                          ticks % 2 > 0 and units.degrees(70.65) or units.degrees(68.75)}

         self.parent.orbit.command = command

         if ticks > 4 then
            techne.iterate = false
         end
      end,
                                }
                       }

bindings['h'] = function()
   root.wireframe.shader.shape.optimize = not root.wireframe.splat.shape.optimize
end

bindings['w'] = function()
   root.wireframe.enabled = not root.wireframe.enabled
end

bindings['f'] = function()
   root.wireframe.splat = shading.flat {
      color = {0, 1, 0, 1},
      shape = root.wireframe.splat.shape
                                       }
end

bindings['space'] = function()
   root.grass.debug = not root.grass.debug

   trace (root.grass.debug)
end
