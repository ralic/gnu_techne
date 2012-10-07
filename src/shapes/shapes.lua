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
   local node, oldmeta
   local r, n = 1, 16

   node = shapes.line {}
   oldmeta = getmetatable(node)

   replacemetatable (node, {
      __index = function (self, key)
		   if key == "radius" then
		      return r
		   elseif key == "segments" then
		      return n
		   else
		      return oldmeta.__index(self, key)
		   end
		end,

      __newindex = function (self, key, value)
		      local v
		      
		      if key == "radius" then
			 r = value
		      elseif key == "segments" then
			 n = value
		      else
			 oldmeta.__newindex(self, key, value)
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
   })
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

function shapes.box(parameters)
   local node, oldmeta
   local a, b, c

   node = shapes.triangles {}
   oldmeta = getmetatable(node)

   replacemetatable (node, {
      __index = function (self, key)
	       if key == "size" then
		  return {a, b, c}
	       else
		  return oldmeta.__index(self, key)
	       end
	    end,

      __newindex = function (self, key, value)
	       local v
	       
	       if key == "size" then
		  a, b, c = table.unpack(value)
	       else
		  oldmeta.__newindex(self, key, value)
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
   })
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

function shapes.cylinder(parameters)
   local node, oldmeta
   local r, l, n = 1, 1, 16

   node = shapes.triangles {}
   oldmeta = getmetatable(node)

   replacemetatable (node, {
      __index = function (self, key)
		   if key == "radius" then
		      return r
		   elseif key == "length" then
		      return l
		   elseif key == "segments" then
		      return n
		   else
		      return oldmeta.__index(self, key)
		   end
		end,

      __newindex = function (self, key, value)
		      local v
		      
		      if key == "radius" then
			 r = value
		      elseif key == "length" then
			 l = value
		      elseif key == "segments" then
			 n = value
		      else
			 oldmeta.__newindex(self, key, value)
			 return
		      end

		      local positions = array.floats(2 * n, 3)
		      local indices = array.ushorts(4 * n - 3, 3)

		      for i = 1, n do
			 local theta, a, b

			 theta = 2 * math.pi * (i - 1) / n
			 a = r * math.cos (theta)
			 b = r * math.sin(theta)

			 positions[i][1] = a
			 positions[i][2] = b
			 positions[i][3] = l / 2

			 positions[n + i][1] = a
			 positions[n + i][2] = b
			 positions[n + i][3] = -l / 2
		      end

		      for i = 1, n - 2 do
		      	 local a

		      	 indices[i][1] = 0
		      	 indices[i][2] = i
		      	 indices[i][3] = i + 1

		      	 a = n + i - 2

		      	 indices[a][1] = n
		      	 indices[a][2] = n + i + 1
		      	 indices[a][3] = n + i
		      end

		      for i = 1, n do
		      	 local b

		      	 b = 2 * (n + i - 2)

		      	 indices[b][1] = i - 1
		      	 indices[b][2] = i + n - 1 
		      	 indices[b][3] = i 

		      	 indices[b + 1][1] = i
		      	 indices[b + 1][2] = i + n - 1
		      	 indices[b + 1][3] = i + n
		      end

		      indices[4 * n - 3][2] = 0
		      indices[4 * n - 3][3] = n - 1

		      self.positions = positions
		      self.indices = indices		      
		   end
   })
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

return shapes
