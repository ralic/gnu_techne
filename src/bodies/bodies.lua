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

local core = require 'bodies.core'
local shading = require 'shading'
local shapes = require 'shapes'

local bodies = {
   box = function (parameters)
	    local box, oldmeta

	    box = core.box (parameters)

	    box.volume = shading.flat {
	       color = {1, 0, 0, 1},

	       shape = shapes.box {
		  wireframe = true,
		  size = box.size,
	       },
	    }

	    oldmeta = getmetatable(box)
	    replacemetatable(box, {
				__newindex = function (self, key, value)
						if key == "size" then
						   self.volume.shape.size = value
						end
						
						oldmeta.__newindex (self, key, value)
					     end
			     })

	    return box
	 end,

   cylinder = function (parameters)
	    local cylinder, oldmeta

	    cylinder = core.cylinder (parameters)

	    cylinder.volume = shading.flat {
	       color = {1, 0, 0, 1},

	       shape = shapes.cylinder {
		  wireframe = true,

		  segments = 16,
		  radius = cylinder.radius,
		  length = cylinder.length,
	       },
	    }

	    oldmeta = getmetatable(cylinder)
	    replacemetatable(cylinder, {
				__newindex = function (self, key, value)
						if key == "radius" then
						   self.volume.shape.radius = value
						elseif key == "length" then
						   self.volume.shape.length = value
						end
						
						oldmeta.__newindex (self, key, value)
					     end
			     })

	    return cylinder
	 end
}

return bodies