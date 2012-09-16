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

local table = require 'table'
local math = require 'math'
local array = require 'array'
local shapes = require 'shapes.core'

function shapes.circle(parameters)
   local node
   local r, n = 1, 16

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
		  n = value
	       else
		  self[key] = value
		  return
	       end

	       local positions = array.floats(n, 3)

	       for i = 1, n do
		  theta = 2 * math.pi * (i - 1) / (n - 1)

		  positions[i][1] = r * math.cos (theta)
		  positions[i][2] = r * math.sin(theta)
		  positions[i][3] = 0
	       end

	       self.positions = positions
	    end
   }
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

function shapes.box(parameters)
   local node
   local a, b, c

   node = shapes.triangles {
      get = function (self, key)
	       if key == "size" then
		  return {a, b, c}
	       else
		  return self[key]
	       end
	    end,

      set = function (self, key, value)
	       local v
	       
	       if key == "size" then
		  a, b, c = table.unpack(value)
	       else
		  self[key] = value
		  return
	       end

	       self.positions = array.floats {
		  -- Near.

		  {-a, -b, c}, {a, -b, c}, {-a, b, c},
		  {-a, b, c}, {a, -b, c}, {a, b, c},

		  -- Far.

		  {-a, -b, -c}, {-a, b, -c}, {a, -b, -c}, 
		  {-a, b, -c}, {a, b, -c}, {a, -b, -c}, 

		  -- Right.

		  {a, -b, -c}, {a, b, -c}, {a, -b, c},
		  {a, -b, c}, {a, b, -c}, {a, b, c},

		  -- Left.

		  {-a, -b, -c}, {-a, -b, c}, {-a, b, -c}, 
		  {-a, -b, c}, {-a, b, c}, {-a, b, -c}, 

		  -- Top.

		  {-a, b, -c}, {-a, b, c}, {a, b, -c}, 
		  {a, b, -c}, {-a, b, c}, {a, b, c}, 

		  -- Bottom.

		  {-a, -b, -c}, {a, -b, -c}, {-a, -b, c},
		  {a, -b, -c}, {a, -b, c}, {-a, -b, c},
	       }
	    end
   }
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

return shapes
