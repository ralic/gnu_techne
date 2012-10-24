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

local shading = require 'shading'
local shapes = require 'shapes'
local math = require 'math'

return {
   wheel = function (parameters)
	    local wheel, oldmeta

	    wheel = core.wheel (parameters)

	    wheel.volume = shading.flat {
	       color = {1, 0, 0, 1},

	       shape = shapes.torus {
		  wireframe = true,
		  radii = wheel.radii,
		  segments = {16, 4},
		  ranges = {{0, 2 * math.pi},
			    {-0.3 * math.pi, 0.3 * math.pi}},
		  -- traverse = function (self) print (self) end,
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

   fourstroke = core.fourstroke,
   racetrack = core.racetrack,
   chain = core.chain
       }
