local table = require "table"
local arraymath = require "arraymath"
local primitives = require "primitives"
local shapes = require "shapes"
local shading = require "shading"

local parameters = ...
local s = parameters.scale or 1

return primitives.transform {
   orientation = arraymath.diagonal(3, s),

   x = shading.flat {
      color = {1, 0, 0},

      shapes.line {
         positions = {{0, 0, 0}, {1, 0, 0}}
      }
   },

   y = shading.flat {
      color = {0, 1, 0},

      shapes.line {
         positions = {{0, 0, 0}, {0, 1, 0}}
      }
   },

   z = shading.flat {
      color = {0, 0, 1},

      shapes.line {
         positions = {{0, 0, 0}, {0, 0, 1}}
      }
   },

   plane = shading.flat {
      color = {1, 1, 0},

      shapes.strip {
         positions = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}}
      }
   },
}
