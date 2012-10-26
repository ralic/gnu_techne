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

local shading = require 'shading'
local shapes = require 'shapes'

return {
   hinge = function (parameters)
	    local hinge, oldmeta

	    hinge = core.hinge (parameters)

	    hinge.schematic = shading.flat {
	       color = {0, 0, 1, 1},

	       shaft = shapes.line {
                  positions = {{0, 0, 0}},

		  -- traverse = function (self) print (self) end,
	       },
	    }

	    oldmeta = getmetatable(hinge)
	    replacemetatable(hinge, {
				__newindex = function (self, key, value)
						if key == "axis" then
						   self.schematic.shaft.positions =
                                                      {{0, 0, 0},
                                                       value}
						end
						
						oldmeta.__newindex (self, key, value)
					     end
			     })

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
