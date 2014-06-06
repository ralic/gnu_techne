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

resources.dofile "utils/basic.lua"

local math = require "math"
local joints = require "joints"
local bodies = require "bodies"
local primitives = require "primitives"
local physics = require "physics"
local units = require "units"

graphics.perspective = {45, 0.1, 10000}
dynamics.gravity = {0, 0, -9.81}

root = primitives.root {
   orbit = resources.dofile ("utils/orbit.lua", {
                                radius = -3,
                                azimuth = units.degrees(0),
                                elevation = units.degrees(60)}),
   environment = bodies.environment {
      ground = bodies.plane {
         position = {0, 0, -0.7},
      }
   },
}

-- Torsion springs.

local a = {}

for i = 1, 3 do
   a[i] = bodies.box {
      position = {0, -1, -1 + 0.5 * i},
      size = {0.1, 0.1, 0.4},

      mass = {
         0.04,
         {0, 0, 0},
         {
            {0.001, -0.0008, 0},
            {-0.0008, 0.001, 0},
            {0,       0,     0.0009}
         },
      }
   }
end

for i = 1, 2 do
   a[i].foo = primitives.joint {
      ball = joints.spherical {
         anchor = {0, -1, -0.75 + 0.5 * i},
      },

      spring = joints.euler {
         stops = {
            {{0, 0}, {3, 0.1}, 0},
            {{0, 0}, {3, 0.1}, 0},
            {{0, 0}, {3, 0.1}, 0},
         }
      },

      bar = a[i + 1],

      attach = function (self)
         local pair = self.pair

         self.ball.bodies = self.pair
         self.spring.bodies = self.pair
      end
   }
end

root.environment[1] = a[1]

-- Compression springs.

local b = {}

for i = 1, 3 do
   b[i] = bodies.box {
      position = {0, 1, -1.25 + 0.75 * i},
      size = {0.1, 0.1, 0.4},

      mass = {
         0.04,
         {0, 0, 0},
         {
            {0.001, 0,     0},
            {0,     0.001, 0},
            {0,     0,     0.0009}
         },
      }
   }
end

for i = 1, 2 do
   b[i].foo = joints.slider {
      stops = {{0, 0}, {50, 1}, 0},
      bar = b[i + 1]
   }
end

root.environment[2] = b[1]

root.timer = primitives.timer {
   period = 3,

   tick = function()
      physics.addforce(a[3], {100 * math.random() - 50,
                              100 * math.random() - 50,
                              0})

      physics.addforce(b[3], {0, 0, 100 * math.random() - 50})
   end
}
