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

return {
   hinge = function (parameters)
	    local hinge, oldmeta, a, b

	    hinge = core.hinge (parameters)

	    hinge.shaft = shading.flat {
	       color = {0.45, 0.66, 0.86, 1},

	       prepare = function (self)
		  local a, d, c, e

		  d = self.bias or 0.1
		  c = self.gain or 1 / 628

		  a = hinge.anchor
		  e = a + (d + c * hinge.state[2]) * hinge.axis

		  self.lines.positions = array.doubles {a, e}
		  self.points.positions = array.doubles {a, e}
	       end,

	       lines = shapes.line {},
	       points = shapes.points {},
				       }

	    hinge.arms = shading.flat {
	       color = {0.3, 0.47, 0.62, 1},

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

	    return hinge
	 end,

  spherical = core.spherical,
  angular = core.angular,
  planar = core.planar,
  clamp = core.clamp,
  slider = core.slider,
  contact = core.contact,
  euler = core.euler,
  gearing = core.gearing,
  universal = core.universal,
  doublehinge = core.doublehinge,
  doubleball = core.doubleball,
  linear = core.linear,
}
