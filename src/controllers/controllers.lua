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

controllers['Rubberband'] = function(parameters)
   local oldmeta, anchor, engaged, rubberband
   local buttonpress, buttonrelease, absolute, relative, latched
   local color = {0, 1, 0}

   local function engage(rubberband, button)
      anchor = rubberband.pointer.axes
      graphics.grabinput = true

      -- Add the rubber-band graphics.

      rubberband.overlay.knob = shading.flat {
         color = {1, 0.7, 0, 0.9},

         shape = shapes.points {},
      }

      rubberband.overlay.vector = shading.flat {
         color = color,
         shape = shapes.lines {},
      }

      engaged = button
   end

   local function disengage(rubberband, button)
      graphics.grabinput = false

      rubberband.overlay.knob = nil
      rubberband.overlay.vector = nil

      engaged = false
   end

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
            end
         end,

         absolute = function (self, axis, value)
            if absolute then
               absolute (self.ancestors[2], axis,
                         value - (anchor and anchor[axis + 1] or 0))
            end
         end,

         relative = function (self, axis, value)
            if relative then
               relative (self.ancestors[2], axis, value)
            end
         end,

         buttonpress = function (self, button)
            if not engaged then
               engage(self.parent, button)
            end

            if buttonpress and (button ~= 2 or not latched) then
               buttonpress (self.ancestors[2], button)
            end
         end,

         buttonrelease = function (self, button)
            if (button ~= 2 or latched) and engaged == button then
               disengage(self.parent, button)
            end

            if button == 2 then
               latched = not latched
            end

            if buttonrelease and (button ~= 2 or not latched) then
               buttonrelease (self.ancestors[2], button)
            end
         end,
                                            },
   }

   oldmeta = getmetatable(rubberband)

   replacemetatable(rubberband, {
                       __index = function (self, key)
                          if key == "buttonpress" then
                             return buttonpress
                          elseif key == "buttonrelease" then
                             return buttonrelease
                          elseif key == "absolute" then
                             return absolute
                          elseif key == "relative" then
                             return relative
                          elseif key == "color" then
                             return color
                          else
                             return oldmeta.__index (self, key)
                          end
                       end,

                       __newindex = function (self, key, value)
                          if key == "buttonpress" then
                             buttonpress = value
                          elseif key == "buttonrelease" then
                             buttonrelease = value
                          elseif key == "absolute" then
                             absolute = value
                          elseif key == "relative" then
                             relative = value
                          elseif key == "color" then
                             color = value

                             if self.overlay.vector then
                                self.overlay.vector.color = value
                             end
                          else
                             oldmeta.__newindex (self, key, value)
                          end
                       end
   })

   for key, value in pairs (parameters) do
      rubberband[key] = value
   end

   return rubberband
end

return controllers
