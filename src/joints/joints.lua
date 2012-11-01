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

	       shape = shapes.line {
		  prepare = function (self)
		     self.positions = array.doubles {
		     	hinge.anchor,
		     	hinge.anchor + hinge.axis
		     				   }
		  end,
				   },
				       }

	    hinge.arms = shading.flat {
	       color = {0.3, 0.47, 0.62, 1},

	       shape = shapes.line {
	       	  prepare = function (self) 
	       	     local a, b, pair

	       	     pair = hinge.pair
		     
	       	     if pair then
	       		local positions = {}

	       		a, b = pair[1], pair[2]

	       		if a and b then
			   self.positions = array.doubles{
			      a.position,
			      hinge.anchor,
			      b.position
							 }
			elseif a then
			   self.positions = array.doubles{
			      hinge.anchor,
			      a.position
							 }
			elseif b then
			   self.positions = array.doubles{
			      hinge.anchor,
			      b.position
							 }
	       		end
	       	     end
	       	  end,
	       },
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
