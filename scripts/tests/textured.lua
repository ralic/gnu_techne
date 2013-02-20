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

array = require "array"
resources = require "resources"
graphics = require "graphics"
dynamics = require "dynamics"
primitives = require "primitives"
shapes = require "shapes"
shading = require "shading"
textures = require "textures"

resources.dofile "utils/basic.lua"

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
