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

local array = require "array"
local arraymath = require "arraymath"
local resources = require "resources"
local graphics = require "graphics"
local dynamics = require "dynamics"
local primitives = require "primitives"
local shapes = require "shapes"
local shading = require "shading"
local textures = require "textures"
local units = require "units"
local staging = require "staging"

resources.dofile "utils/basic.lua"

graphics.perspective = {45, 0.1, 10}
dynamics.timestep = 0.1
dynamics.timescale = 1

root = primitives.root {
   -- orbit = resources.dofile ("utils/orbit.lua", {
   --                              radius = -3,
   --                              azimuth = units.degrees(0),
   --                              elevation = units.degrees(0)
   -- }),

   camera = primitives.transform {
      position = {0, 0, -3},
      
   path = staging.bezier {
      speed = 1,
      vertices = {
         {-1.000000, 0.000000, 0.000000},
         {-0.992566, 0.998582, 0.124704},
         {1.003843, 0.980272, 0.540104},
         {1.000000, -0.000000, 0.737727},
         {0.996157, -0.980271, 0.935349},
         {-1.007554, -1.003466, 0.998973},
         {-1.006137, -0.003986, 1.232722},
      },
      
      frame = resources.dofile ("scripts/utils/frame.lua", {
                                   scale = 0.1,
      }),
   }}
}
