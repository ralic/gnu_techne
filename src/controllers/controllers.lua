-- Copyright (C) 2012 Papavasileiou Dimitris                             
--
-- Permission is hereby granted, free of charge, to any person
-- obtaining a copy of this software and associated documentation
-- files (the "Software"), to deal in the Software without
-- restriction, including without limitation the rights to use, copy,
-- modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
-- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
-- BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
-- ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
-- CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.

local controllers = require "controllers.core"
local graphics = require "graphics"
local primitives = require "primitives"
local array = require "array"
local shading = require "shading"
local shapes = require "shapes"

local buttonpress, buttonrelease, absolute, relative

controllers['Rubberband'] = function(parameters)
   local oldmeta, anchor, engaged, engage, rubberband

   rubberband = primitives.event {
      overlay = shading.overlay {
         normalized = false,

         -- The actual mouse cursor.

         cursor = shading.flat {
            color = {1, 0.7, 0, 0.9},
            
            shape = shapes.points {},
                               },
                                },

      pointer = controllers['Core pointer'] {
         input = function (self)
            local overlay = self.parent.overlay
            local position = self.axes

            overlay.cursor.shape.positions = array.floats {position}

            if engaged then
               overlay.knob.shape.positions = array.floats {anchor}
               overlay.vector.shape.positions = array.floats {anchor, position}

               overlay.vector.color = position[2] > anchor[2] and
                  {1, 0, 0, 0.8} or {0.1607, 0.9763, 0, 0.8}
            end
         end,
            
         absolute = function (self, axis, value)
            if engaged and absolute then
               absolute (self.ancestors[2], axis, value - anchor[axis + 1])
            end
         end,
         
         relative = function (self, axis, value)
            if relative then
               relative (self.ancestors[2], axis, value)
            end
         end,
         
         buttonpress = function (self, button)
            if buttonpress then
               buttonpress (self.ancestors[2], button)
            end
         end,
         
         buttonrelease = function (self, button)
            if buttonrelease then
               buttonrelease (self.ancestors[2], button)
            end
         end,
                                            },
                                 }

   oldmeta = getmetatable(rubberband)

   replacemetatable(rubberband, {
                       __index = function (self, key)
                          if key == "engaged" then
                             return engaged
                          elseif key == "buttonpress" then
                             return buttonpress
                          elseif key == "buttonrelease" then
                             return buttonrelease
                          elseif key == "absolute" then
                             return absolute
                          elseif key == "relative" then
                             return relative
                          else
                             return oldmeta.__index (self, key)
                          end
                       end,

                       __newindex = function (self, key, value)
                          if key == "engaged" then
                             if engaged ~= value then
                                if value then
                                   anchor = self.pointer.axes
                                   graphics.grabinput = true

                                   -- Add the rubber-band graphics.
         
                                   self.overlay.knob = shading.flat {
                                      color = {1, 0.7, 0, 0.9},
                                      
                                      shape = shapes.points {},
                                                                    }
                                   
                                   self.overlay.vector = shading.flat {
                                      shape = shapes.lines {},
                                                                      }
                                else
                                   graphics.grabinput = false

                                   self.overlay.knob = nil
                                   self.overlay.vector = nil
                                end
                                
                                engaged = value
                             end
                          elseif key == "buttonpress" then
                             buttonpress = value
                          elseif key == "buttonrelease" then
                             buttonrelease = value
                          elseif key == "absolute" then
                             absolute = value
                          elseif key == "relative" then
                             relative = value
                          else
                             oldmeta.__newindex (self, key, value)
                          end
                       end
                                })

   engage = parameters.engaged
   parameters.engaged = nil

   for key, value in pairs (parameters) do
      rubberband[key] = value
   end

   -- Engage the rubberband (if requested) after all configuration has
   -- been set.

   if engage then
      rubberband.engaged = true
   end
   
   return rubberband
end

return controllers
