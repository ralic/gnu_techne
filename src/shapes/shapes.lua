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

   node = shapes.loop {}
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

		      local positions = array.floats(n, 2)

		      for i = 1, n do
			 theta = 2 * math.pi * (i - 1) / n

			 positions[i][1] = r * math.cos (theta)
			 positions[i][2] = r * math.sin(theta)
		      end

		      self.positions = positions
		   end
   })
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

function shapes.rectangle(parameters)
   local node, oldmeta
   local a, b, a_2, b_2

   node = shapes.triangles {}
   oldmeta = getmetatable(node)

   replacemetatable (node, {
      __index = function (self, key)
	       if key == "size" then
		  return {a, b}
	       else
		  return oldmeta.__index(self, key)
	       end
	    end,

      __newindex = function (self, key, value)
	       local v
	       
	       if key == "size" then
		  a, b = table.unpack(value)
	       else
		  oldmeta.__newindex(self, key, value)
		  return
	       end

	       a_2, b_2 = 0.5 * a, 0.5 * b

	       self.positions = array.floats {
		  {-a_2, -b_2, 0}, {a_2, -b_2, 0}, {-a_2, b_2, 0},
		  {-a_2, b_2, 0}, {a_2, -b_2, 0}, {a_2, b_2, 0},
	       }

	       self.mapping = array.floats {
		  {0, 0}, {1, 0}, {0, 1},
		  {0, 1}, {1, 0}, {1, 1},
	       }
	    end
   })
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

function shapes.box(parameters)
   local node, oldmeta
   local a, b, c, a_2, b_2, c_2

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

	       a_2, b_2, c_2 = 0.5 * a, 0.5 * b, 0.5 * c

	       self.positions = array.floats {
		  -- Near.

		  {-a_2, -b_2, c_2}, {a_2, -b_2, c_2}, {-a_2, b_2, c_2},
		  {-a_2, b_2, c_2}, {a_2, -b_2, c_2}, {a_2, b_2, c_2},

		  -- Far.

		  {-a_2, -b_2, -c_2}, {-a_2, b_2, -c_2}, {a_2, -b_2, -c_2}, 
		  {-a_2, b_2, -c_2}, {a_2, b_2, -c_2}, {a_2, -b_2, -c_2}, 

		  -- Right.

		  {a_2, -b_2, -c_2}, {a_2, b_2, -c_2}, {a_2, -b_2, c_2},
		  {a_2, -b_2, c_2}, {a_2, b_2, -c_2}, {a_2, b_2, c_2},

		  -- Left.

		  {-a_2, -b_2, -c_2}, {-a_2, -b_2, c_2}, {-a_2, b_2, -c_2}, 
		  {-a_2, -b_2, c_2}, {-a_2, b_2, c_2}, {-a_2, b_2, -c_2}, 

		  -- Top.

		  {-a_2, b_2, -c_2}, {-a_2, b_2, c_2}, {a_2, b_2, -c_2}, 
		  {a_2, b_2, -c_2}, {-a_2, b_2, c_2}, {a_2, b_2, c_2}, 

		  -- Bottom.

		  {-a_2, -b_2, -c_2}, {a_2, -b_2, -c_2}, {-a_2, -b_2, c_2},
		  {a_2, -b_2, -c_2}, {a_2, -b_2, c_2}, {-a_2, -b_2, c_2},
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

function shapes.torus(parameters)
   local node, oldmeta
   local r, R, n, N = 1, 0.1, 16, 8
   local dtheta, dphi = {0, 2 * math.pi}, {0, 2 * math.pi}

   node = shapes.strip {}
   oldmeta = getmetatable(node)

   replacemetatable (node, {
      __index = function (self, key)
		   if key == "radii" then
		      return {R, r}
		   elseif key == "segments" then
		      return {N, n}
		   elseif key == "ranges" then
		      return {dtheta, dphi}
		   else
		      return oldmeta.__index(self, key)
		   end
		end,

      __newindex = function (self, key, value)
		      local v
		      
		      if key == "radii" then
			 R, r = table.unpack(value)
		      elseif key == "segments" then
			 N, n = table.unpack(value)
		      elseif key == "ranges" then
			 dtheta, dphi = table.unpack(value)
		      else
			 oldmeta.__newindex(self, key, value)
			 return
		      end

		      local positions = array.floats(2 * n * N, 3)

		      for i = 0, N - 1 do
			 for j = 0, n - 1 do
			    local k, phi, theta, rho

			    k = 2 * (i * n + j)

			    phi = dphi[1] + (dphi[2] - dphi[1]) * j / (n - 1)
			    theta = dtheta[1] +
			       (dtheta[2] - dtheta[1]) * i / (N - 1) + phi / N
			    rho = R + r * math.cos(phi)

			    positions[k + 1][1] = rho * math.sin(theta)
			    positions[k + 1][2] = r * math.sin(phi)
			    positions[k + 1][3] = rho * math.cos(theta)

			    theta = theta + 2 * math.pi / N

			    positions[k + 2][1] = rho * math.sin(theta)
			    positions[k + 2][2] = r * math.sin(phi)
			    positions[k + 2][3] = rho * math.cos(theta)
			 end
		      end

		      self.positions = positions
		   end
   })
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

return shapes
