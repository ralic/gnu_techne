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
resources.dofile "orbit.lua"

graphics.perspective = {units.degrees(90), 0.1, 10000}

local heights = {}

for i = 1, 513 do
   heights[i] = {}
   for j = 1, 513 do
      heights[i][j] = (i > 256 and 0.5 or 0) + (j > 256 and 0.5 or 0)
   end
end

elevation = topography.elevation {
   depth = 9,
   resolution = {3 / 512, 3 / 512},

   tiles = {
      {
         {array.nushorts(heights), nil, nil}
      }
   }
}

root = primitives.root {
   shading.flat {
      color = {1, 1, 0, 1},

      elevation.shape {
         target = 5000,
                      }
                }
                       }
