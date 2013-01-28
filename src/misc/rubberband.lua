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

local graphics = require "graphics"

local primitives = require "primitives"
local bindings = require "bindings"
local array = require "array"
local shading = require "shading"
local shapes = require "shapes"
local bindings = require "bindings"

local rubberband = {
   anchor = {0.5, 0.75},
}

_G[rubberband] = primitives.root {
   shading.overlay {
      normalized = false,

      -- The actual mouse cursor.

      cursor = shading.flat {
         color = {1, 0.7, 0, 0.9},
         
         shape = shapes.points {},
                            },

      -- The rest of the rubber-band.
      
      knob = shading.flat {
         color = {1, 0.7, 0, 0.9},
         
         shape = shapes.points {},
                          },
      
      vector = shading.flat {
         shape = shapes.lines {},
                            },

      link = function (self)
         local O, P
         
         O = array.floats {
            graphics.window[1] * rubberband.anchor[1],
            graphics.window[2] * rubberband.anchor[2]
         }

         P = array.copy (O)

         graphics.grabinput = true
         graphics.pointer = {O[1], O[2]}

         local function update(i, x)
            P[i] = x

            self.cursor.shape.positions = array.floats {P}
            self.knob.shape.positions = array.floats {O}
            self.vector.shape.positions = array.floats {O, P}

            self.vector.color = P[2] > O[2] and
               {1, 0, 0, 0.8} or {0.1607, 0.9763, 0, 0.8}
         end
         
         bindings['axis-0'] = function (sequence, x)
            local binding = bindings['[Rubberband]axis-0']

            update(1, x)

            if binding then
               binding ('[Rubberband]axis-0', x - O[2])
            end
         end
         
         bindings['axis-1'] = function (sequence, y)
            local binding = bindings['[Rubberband]axis-1']

            update(2, y)

            if binding then
               binding ('[Rubberband]axis-1', O[2] - y)
            end
         end
      end,
                   }
                      }

return rubberband

