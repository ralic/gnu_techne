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

local arraymath = require "arraymath"
local resources = require "resources"
local graphics = require "graphics"
local dynamics = require "dynamics"
local primitives = require "primitives"
local topography = require "topography"
local units = require "units"

resources.dofile "utils/basic.lua"

graphics.perspective = {units.degrees(90), 0.1, 10000}
dynamics.timescale = 0

root = primitives.root {
   atmosphere = topography.atmosphere {
      size = {1024, 512},

      turbidity = 4,

      rayleigh = {6.95e-06, 1.18e-05, 2.44e-05},
      mie = 7e-5,

      sun = {1.74, units.degrees(15)},
                                      },
   observer = primitives.observer {
      spinner = primitives.timer {
         period = 0,

         tick = function (self, ticks, delta, elapsed)
            self.parent.orientation = arraymath.euler (0, units.degrees(130), elapsed)
         end,
                                     }
                                  }
                       }
