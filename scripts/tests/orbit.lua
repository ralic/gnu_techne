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

local staging = require "staging"
local rubberband = require "rubberband"
local current

rubberband.momentary = true

orbit = primitives.root {
   rig = staging.orbit {
      command = {3, units.degrees(0), units.degrees(0)},
                       },

   pointer = controllers['Core pointer'] {
      buttonrelease = function (self, button)
         if button == 1 then
            rubberband.engaged = false
         end         
      end,

      buttonpress = function (self, button)
         if button == 1 then
            rubberband.engaged = true
            current = self.parent.rig.command
         end         
      end
                                         }
                        }

bindings['[Rubberband]absolute-axis-0'] = function(sequence, value)
   local command = orbit.rig.command
   command[3] = current[3] - units.degrees(value) * 0.4
   orbit.rig.command = command
end

bindings['[Rubberband]absolute-axis-1'] = function(sequence, value)
   local command = orbit.rig.command
   command[2] = current[2] - units.degrees(value) * 0.4
   orbit.rig.command = command
end

