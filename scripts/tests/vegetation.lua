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

local resources = require "resources"

resources.dofile ("utils/basic.lua", "Vegetation test", 1024, 768)

require "bindings"

local io = require "io"
local array = require "array"
local arraymath = require "arraymath"
local graphics = require "graphics"
local primitives = require "primitives"
local topography = require "topography"
local shading = require "shading"
local textures = require "textures"
local units = require "units"
local bindings = require "bindings.default"

graphics.perspective = {units.degrees(50), 0.1, 10000}

local file = not options.generate and io.open (".heightmap")
local heights

if not file then
   local m, M

   heights = array.nushorts(resources.dofile ("tests/diamondsquare.lua", 4096, 0.000125))
   m, M = arraymath.range(heights)
   heights = (heights - m) / (M - m)

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

elevation = topography.elevation {
   depth = 12,
   resolution = {0.625, 0.625},

   albedo = 1.5,
   separation = 1,

   swatches = {
      {
         {0, .99, .99}, {},
      },

      {
         {120 / 360, .99, .99}, {},
      },

      {
         {240 / 360, .99, .99}, {},
      },
   },

   tiles = {
      {
         {heights, nil, array.nuchars(base), {200, 0}}
      }
   }
}

root = primitives.root {
   orbit = resources.dofile ("utils/orbit.lua", {
                                radius = -1000,
                                azimuth = units.degrees(0),
                                elevation = units.degrees(81.6)
   }),

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

      splat = elevation.splat {
         shape = elevation.shape {
            tag = "elevation",
            target = 15000,
         }
      },
   },

   grass = shading.wireframe {
      enabled = false,

      elevation.vegetation {
         tag = "vegetation",

         ceiling = 1000,
         density = 50000,
         horizon = 100,
         clustering = 8,

         topography.seeds {
            color = {0.5, 0.5, 0.5},
            height = 0.1,
            threshold = 0,
         },

         topography.seeds {
            color = {0.5, 0.5, 0.5},
            height = 0.1,
            threshold = 0,
         },

         topography.seeds {
            color = {0.5, 0.5, 0.5},
            height = 0.1,
            threshold = 0,
         },
      },
   },

   cameraman = options.timed and primitives.timer {
      period = 2,

      tick = function(self, ticks)
         self.parent.orbit.azimuth = (ticks % 2 > 0 and 1 or 0) * units.degrees(90)
         self.parent.orbit.elevation = ticks % 2 > 0 and units.degrees(79.5) or units.degrees(81.6)

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
