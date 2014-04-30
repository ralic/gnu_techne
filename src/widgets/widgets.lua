-- Copyright (C) 2012 Papavasileiou Dimitris
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

local core = require 'widgets.core'

if not options.drawwidgets then
   return core
end

local shading = require 'shading'
local shapes = require 'shapes'
local arraymath = require 'arraymath'
local array = require 'array'
local primitives = require 'primitives'

local widgets = {}
local lightgoldenrod = {0.8441, 0.8667, 0.6463}
local darkseagreen = {0.2319, 0.5084, 0.2559}

for key, corewidget in pairs(core) do
   widgets[key] = function (parameters)
      local widget

      widget = corewidget (parameters)

      widget.schematic = primitives.graphic {

         draw = function (self)
            local m = widget.content
            local p = widget.padding

            self.content.shape.positions = array.doubles {
               {-0.5 * m[1], -0.5 * m[2]},
               {0.5 * m[1], -0.5 * m[2]},
               {0.5 * m[1], 0.5 * m[2]},
               {-0.5 * m[1], 0.5 * m[2]},
               {-0.5 * m[1], -0.5 * m[2]},
                                                         }

            self.padding.shape.positions = array.doubles {
               {-0.5 * m[1] - p[1], -0.5 * m[2] - p[3]},
               {0.5 * m[1] + p[2], -0.5 * m[2] - p[3]},
               {0.5 * m[1] + p[2], 0.5 * m[2] + p[4]},
               {-0.5 * m[1] - p[1], 0.5 * m[2] + p[4]},
               {-0.5 * m[1] - p[1], -0.5 * m[2] - p[3]},
                                                         }
         end,

         content = shading.flat {
            color = lightgoldenrod,

            shape = shapes.line {},
                              },


         padding = shading.flat {
            color = darkseagreen,

            shape = shapes.line {},
                              },
                                            }

      return widget
   end
end

return widgets
