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

local core = require 'joints.core'

if not options.drawjoints then
   return core
end

local math = require 'math'
local table = require 'table'
local shading = require 'shading'
local shapes = require 'shapes'
local transform = require 'transform'
local array = require 'array'
local operations = require 'array.operations'
local primitives = require 'primitives'

local red = {1, 0, 0, 1}
local white = {1, 1, 1, 1}
local gold = {1, 0.69, 0, 1}
local lightslategray = {0.1445, 0.2281, 0.3036}
local steelblue = {0.0374, 0.2031, 0.4535}
local lightsteelblue = {0.5812, 0.6223, 0.4535}
local lightgoldenrod = {0.8441, 0.8667, 0.6463}

return {
   hinge = function (parameters)
	    local hinge, oldmeta, a, b

	    hinge = core.hinge (parameters)

	    hinge.schematic = primitives.node {
	       shaft = shading.flat {
		  color = lightgoldenrod,

		  prepare = function (self)
		     local a, d, c, e

		     d = self.bias or 0.1
		     c = self.gain or 1 / 628

		     a = hinge.anchor
		     e = operations.add(a, operations.scale (hinge.axis, d + c * hinge.state[2]))

		     self.lines.positions = array.doubles {a, e}
		     self.points.positions = array.doubles {a, e}
		  end,

		  lines = shapes.line {},
		  points = shapes.points {},
				    },

	       arms = shading.flat {
		  color = lightsteelblue,

		  prepare = function (self) 
		     local a, b, pair

		     pair = hinge.pair
		     
		     if pair then
			local positions = {}

			a = pair[1] and pair[1].position
			b = pair[2] and pair[2].position

			if a and b then
			   self.lines.positions = array.doubles{a,
								hinge.anchor,
								b}

			   self.points.positions = array.doubles{a, b}
			elseif a then
			   self.lines.positions = array.doubles{hinge.anchor, a}
			   self.points.positions = array.doubles{a}
			elseif b then
			   self.lines.positions = array.doubles{hinge.anchor, b}
			   self.points.positions = array.doubles{b}
			end
		     end
		  end,

		  lines = shapes.line {},
		  points = shapes.line {},
					 }
					      }
	    return hinge
	 end,

   slider = function (parameters)
	    local slider, oldmeta, a, b

	    slider = core.slider (parameters)

	    slider.schematic = primitives.node {
	       prepare = function (self)
	    	  local a, b, L, l, h, x, d, stops, pair

	    	  stops = slider.stops
	    	  pair = slider.pair

	    	  if not pair then
	    	     return
	    	  end

	    	  a = pair[1] and pair[1].position
	    	  b = pair[2] and pair[2].position
	    	  x = slider.axis
	    	  d = slider.state[1]

	    	  if a and b then
		     l, h = slider.stops[1][1], slider.stops[1][2]

	    	     if l ~= -1 / 0 or h ~= 1 / 0 then
			if math.abs(d - l) < 1e-3 * (h - l) then
			   self.tube.color = red
			else
			   self.tube.color = gold
			end

			if math.abs(d - h) < 1e-3 * (h - l) then
			   self.rod.color = red
			else
			   self.rod.color = white
			end

	    		d = d - l
		     else
			d = -0.5 * transform.dot (operations.subtract(b, a), x)
	    	     end
	    	  end
		  
		  self.tube.lines.positions = array.doubles {
		     a, operations.add(b, operations.scale(x, d))
							    }
		  
		  self.rod.lines.positions = array.doubles {
		     b, operations.add(b, operations.scale(x, d))
							   }
	       end,

	       rod = shading.flat {
		  color = white,

	    	  lines = shapes.line {},
	    	  points = shapes.points {},
	    			  },

	       tube = shading.flat {
		  color = gold,

	    	  lines = shapes.line {},
	    	  points = shapes.points {},
	    			   }
	    				       }

	    return slider
	 end,

  spherical = core.spherical,
  angular = core.angular,
  planar = core.planar,
  clamp = core.clamp,
  contact = core.contact,
  euler = core.euler,
  gearing = core.gearing,
  universal = core.universal,
  doublehinge = core.doublehinge,
  doubleball = core.doubleball,
  linear = core.linear,
}
