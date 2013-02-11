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

local sensitivity = 3

local info = primitives.root {
   display = widgets.display {
      layout = widgets.layout {
         align = {-1, -1},
         color = {1, 1, 1},
         padding = {0.01, 0, 0.01, 0},
                              }
                             }
                       }

local orbit = staging.orbit {
   command = {...},

   link = function (self)
      rubberband.momentary = true

      bindings['[Rubberband]absolute-axis-0'] = function(sequence, value)
         local command = self.command

         if not rubberband.zoom then
            command[2] = current[2] - math.ldexp(units.degrees(value), -sensitivity)

            self.command = command
         end
      end

      bindings['[Rubberband]absolute-axis-1'] = function(sequence, value)
         local command = self.command

         if rubberband.zoom then
            command[1] = current[1] - math.ldexp(value, -sensitivity)
         else
            command[3] = current[3] - math.ldexp(units.degrees(value), -sensitivity)
         end

         self.command = command
      end
   end,

   pointer = controllers['Core pointer'] {
      relative = function(self, axis, value)
         sensitivity = math.clamp (sensitivity + value, 1, 10)

         info.timer = primitives.timer {
            period = 1,

            link = function(self)
               info.display.layout.text = string.format ('<span font="Sans 12" color="white">Orbit camera sensitivity: %d</span>',sensitivity)
            end,

            tick = function(self, tick)
               info.display.layout.text = ""
               self.period = nil
            end
                                       }
      end,

      buttonrelease = function (self, button)
         if button == 1 or button == 3 then
            rubberband.engaged = false
         end         
      end,

      buttonpress = function (self, button)
         if button == 1 then
            rubberband.engaged = true
            rubberband.zoom = false
            current = self.parent.command
         elseif button == 3 then
            rubberband.engaged = true
            rubberband.zoom = true
            current = self.parent.command
         end         
      end
                                         }
                      }

return orbit
