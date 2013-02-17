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
local controllers = require "controllers"

local rubberband = {
   button = 2,
}

local overlay = shading.overlay {
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

   pointer = controllers['Core pointer'] {},

   link = function (self)
      local a, O, P, binding

      a = rubberband.anchor or self.pointer.axes
      O = array.floats (a)
      P = array.copy (O)

      graphics.grabinput = true
      self.pointer.axes = a

      local function update(i, x)
         P[i + 1] = x

         self.cursor.shape.positions = array.floats {P}
         self.knob.shape.positions = array.floats {O}
         self.vector.shape.positions = array.floats {O, P}

         self.vector.color = P[2] > O[2] and
            {1, 0, 0, 0.8} or {0.1607, 0.9763, 0, 0.8}
      end
      
      self.pointer.absolute = function (self, axis, value)
         local sequence = '[Rubberband]absolute-axis-' .. tostring(axis)
         local binding = bindings[sequence]

         update(axis, value)

         if binding then
            binding (sequence, value - O[axis + 1])
         end
      end
   end,
                                }

local engaged = false
local root = primitives.root {}

_G[rubberband] = root

setmetatable(rubberband, {
                __index = function (self, key)
                   if key == "engaged" then
                      return engaged
                   else
                      return rawget (self, key)
                   end
                end,

                __newindex = function (self, key, value)
                   if key == "engaged" then
                      if engaged ~= value then
                         local sequence, binding

                         if value then
                            sequence = '[Rubberband]engage'
                            root.overlay = overlay
                         else
                            sequence = '[Rubberband]disengage'
                            root.overlay = nil
                         end
                         
                         engaged = value

                         -- Fire the binding.

                         binding = bindings[sequence]
                         if binding then
                            binding (sequence)
                         end                            
                      end
                   else
                      rawset (self, key, value)
                   end
                end
                         })

return rubberband

