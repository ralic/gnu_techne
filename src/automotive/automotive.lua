-- Copyright (C) 2009 Papavasileiou Dimitris                             
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

local core = require 'automotive.core'

if not options.drawbodies then
   return core
end

local primitives = require 'primitives'
local shading = require 'shading'
local shapes = require 'shapes'
local table = require 'table'
local array = require 'array'
local arraymath = require 'arraymath'
local math = require 'math'
 
local lightblue2 = {0.4064, 0.7526, 0.8644}
local lightblue3 = {0.2805, 0.5343, 0.6179}

return {
   wheel = function (parameters)
	    local wheel, oldmeta

	    wheel = core.wheel (parameters)

	    wheel.volume = shading.flat {
	       color = {1, 0, 0, 1},

	       shape = shapes.torus {
		  wireframe = true,
		  radii = wheel.radii,
		  segments = {16, 7},
		  ranges = {{0, 2 * math.pi},
			    {-math.pi / 2, math.pi / 2}},
		  -- draw = function (self) print (self) end,
	       },
	    }

	    oldmeta = getmetatable(wheel)
	    replacemetatable(wheel, {
				__newindex = function (self, key, value)
						if key == "radii" then
						   self.volume.shape.radii = value
						end
						
						oldmeta.__newindex (self, key, value)
					     end
			     })

	    return wheel
   end,

   fourstroke = function (parameters)
	    local fourstroke, oldmeta

	    fourstroke = core.fourstroke (parameters)

	    fourstroke.schematic = shading.flat {
	       color = lightblue2,

	       draw = function (self)
		  local a, d, c, e

		  d = self.bias or 0.1
		  c = self.gain or 0.3 / 628

		  a = fourstroke.anchor
		  e = arraymath.combine(a, fourstroke.axis,
                                        1, d + c * fourstroke.state[2])

		  self.lines.positions = array.doubles {a, e}
		  self.points.positions = array.doubles {a, e}
	       end,

	       lines = shapes.line {},
	       points = shapes.points {},
	    }

	    return fourstroke
   end,

   chain = function (parameters)
	    local chain, oldmeta

	    chain = core.chain (parameters)

	    chain.schematic = primitives.graphic {
	       draw = function (self)
		  local a, b, c, e

		  c = self.gain or 0.01

		  contacts = chain.contacts
		  
		  if contacts then
		     a, b = table.unpack(contacts)

		     self.run.lines.positions = array.doubles {a, b}
		     self.run.points.positions = array.doubles {a, b}

		     e = arraymath.combine(a, chain.velocity, 1, c)

		     self.vector.lines.positions = array.doubles {a, e}
		  end
	       end,

	       run = shading.flat {
		  color = {1, 1, 0, 1},
		  lines = shapes.line {},
		  points = shapes.points {},
				  },

	       vector = shading.flat {
		  color = {0, 1, 0, 1},
		  lines = shapes.line {},
			    }
					      }

	    return chain
   end,

   racetrack = core.racetrack,
       }
