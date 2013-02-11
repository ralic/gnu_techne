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

if not options.drawbodies then
   return core
end

local shading = require 'shading'
local shapes = require 'shapes'

return {
   point = function (parameters)
	    local point, oldmeta

	    point = core.point (parameters)

	    point.volume = shading.flat {
	       color = {1, 1, 0, 1},

	       shape = shapes.points {
		  positions = {{0, 0, 0}},

		  -- draw = function (self) print (self.vertices) end,
	       },
	    }

	    return point
	 end,

   box = function (parameters)
	    local box, oldmeta

	    box = core.box (parameters)

	    box.volume = shading.flat {
	       color = {1, 0, 0, 1},

	       shape = shapes.box {
		  wireframe = true,
		  size = box.size,

		  -- draw = function (self) print (self) end,
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
	 end,

   ball = function (parameters)
	    local ball, oldmeta

	    ball = core.ball (parameters)

	    ball.volume = shading.flat {
	       color = {1, 0, 0, 1},

	       shape = shapes.circle {
		  wireframe = true,

		  segments = 16,
		  radius = ball.radius,
	       },
	    }

	    oldmeta = getmetatable(ball)
	    replacemetatable(ball, {
				__newindex = function (self, key, value)
						if key == "radius" then
						   self.volume.shape.radius = value
						end
						
						oldmeta.__newindex (self, key, value)
					     end
			     })

	    return ball
	 end,

   environment = core.environment,
   plane = core.plane,
   capsule = core.capsule,
   polyhedron = core.polyhedron,
   system = core.system,
} 
