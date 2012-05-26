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

local array = require 'array'
local shapes = require 'shapes.core'

function shapes.circle(parameters)
   local node
   local r, n

   node = shapes.line {
      get = function (self, key)
	       if key == "radius" then
		  return r
	       elseif key == "segments" then
		  return n
	       else
		  return self[key]
	       end
	    end,

      set = function (self, key, value)
	       local v
	       
	       if key == "radius" then
		  r = value
	       elseif key == "segments" then
		  self.vertices = array.doubles (value, 3)
		  n = value
	       else
		  self[key] = value
		  return
	       end

	       v = self.vertices

	       if v then
		  for i = 1, n do
		     theta = 2 * math.pi * (i - 1) / (n - 1)
		     v[i] = {r * math.cos (theta), r * math.sin(theta), 0}
		  end
	       end
	    end
   }
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

return shapes
