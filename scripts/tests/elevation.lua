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
local heights = resources.dofile ("diamondsquare.lua", 512, 0.0012)

graphics.perspective = {units.degrees(50), 0.1, 10000}

elevation = topography.elevation {
   depth = 9,
   resolution = {3000 / 512, 3000 / 512},

   tiles = {
      {
         {array.nushorts(heights), nil, nil, {500, 0}}
      }
   }
}

root = primitives.root {
   orbit = resources.dofile ("orbit.lua", -1000, 0, 0),

   wireframe = shading.wireframe {
      shader = shading.flat {
         color = {1, 1, 0, 1},

         shape = elevation.shape {
            target = 15000,
                                 }
                            }
                                 },

   cameraman = options.timed and primitives.timer {
      period = 1,
      
      tick = function(self, ticks)
         local command = {-1000, units.degrees(62),
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