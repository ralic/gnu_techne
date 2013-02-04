-- Copyright (C) 2010-2011 Papavasileiou Dimitris                           
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

-- An adaptatiob of the heightmap module
-- Copyright (C) 2011 Marc Lepage

local max, random = math.max, math.random
local size, roughness = ...
local width, height = size, size

-- Square step.
-- Sets map[x][y] from square of radius d using height function f.

local function square(map, x, y, d, f)
   local sum, num = 0, 0

   if 0 <= x - d then
      if 0 <= y - d then
         sum, num = sum + map[x - d + 1][y - d + 1], num + 1
      end
      
      if y + d <= height then
         sum, num = sum + map[x - d + 1][y + d + 1], num + 1
      end
   end

   if x + d <= width then
      if 0 <= y - d then
         sum, num = sum + map[x + d + 1][y - d + 1], num + 1
      end

      if y + d <= height then
         sum, num = sum + map[x + d + 1][y + d + 1], num + 1
      end
   end

   map[x + 1][y + 1] = f(map, x, y, d, sum / num)
end

-- Diamond step.
-- Sets map[x][y] from diamond of radius d using height function f.

local function diamond(map, x, y, d, f)
   local sum, num = 0, 0

   if 0 <= x - d then
      sum, num = sum + map[x - d + 1][y + 1], num + 1
   end

   if x + d <= width then
      sum, num = sum + map[x + d + 1][y + 1], num + 1
   end

   if 0 <= y - d then
      sum, num = sum + map[x + 1][y - d + 1], num + 1
   end

   if y + d <= height then
      sum, num = sum + map[x + 1][y + d + 1], num + 1
   end

   map[x + 1][y + 1] = f(map, x, y, d, sum / num)
end

-- Diamond square algorithm generates cloud/plasma fractal heightmap.
-- Size must be power of two.

local function diamondsquare(size, f)
   -- Create the map.

   local map = {}

   for c = 0, size do
      local t = {}
      for i = 0, size do t[i + 1] = 0 end
      map[c + 1] = t
   end

   -- Seed the four corners.

   local d = size

   map[0 + 1][0 + 1] = f(map, 0, 0, d, 0)
   map[0 + 1][d + 1] = f(map, 0, d, d, 0)
   map[d + 1][0 + 1] = f(map, d, 0, d, 0)
   map[d + 1][d + 1] = f(map, d, d, d, 0)
   d = d / 2

   -- Perform square and diamond steps.

   while 1 <= d do
      for x = d, width - 1, 2 * d do
         for y = d, height - 1, 2 * d do
            square(map, x, y, d, f)
         end
      end

      for x = d, width - 1, 2 * d do
         for y = 0, height, 2 * d do
            diamond(map, x, y, d, f)
         end
      end

      for x = 0, width, 2 * d do
         for y = d, height - 1, 2 * d do
            diamond(map, x, y, d, f)
         end
      end

      d = d / 2
   end
   return map
end

-- Default height function
-- d is depth (from size to 1 by powers of two)
-- h is mean height at map[x][y] (from square/diamond of radius d)
-- returns h' which is used to set map[x][y]

local function defaultf(map, x, y, d, h)
   local h_prime = h + random() * d * roughness

   if h_prime > 1 then
      h_prime = 1
   end

   return h_prime
end

return diamondsquare(size, defaultf)

