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

local string = require "string"
local math = require "math"
local techne = require "techne"
local primitives = require "primitives"
local core = require 'topography.core'

if not options.profile then
   return core
end

local topography, seedsprofiler, shapeprofiler

local function aggregate(bins)
   local sum = 0
   
   for i, bin in ipairs(bins) do
      sum = sum + math.round(bin[1]) * bin[2]
   end

   return sum
end

seedsprofiler = primitives.graphic {
   link = function(self)
      local parent = self.parent

      self.triangles = parent.triangles
      self.error = parent.error
      self.bins = parent.bins
      self.seeds = aggregate(parent.bins)
      self.start = techne.iterations
   end,
   
   draw = function(self)
      local parent = self.parent
      
      for i, bin in ipairs(self.bins) do
         bin[1] = bin[1] + parent.bins[i][1]
         bin[2] = bin[2] + parent.bins[i][2]
         bin[3] = parent.bins[i][3]
      end

      self.seeds = self.seeds + aggregate(parent.bins)

      self.triangles[1] = self.triangles[1] + parent.triangles[1]
      self.triangles[2] = self.triangles[2] + parent.triangles[2]
      self.error = self.error + parent.error
   end,

   unlink = function(self)
      local n

      n = techne.iterations - self.start + 1
      
      message(string.format([[
Seed profile for node: %s
Mean seed count per iteration: %.1f
Within-cluster RMS error: %.1f
Mean number of triangles visited: %d roam, %d fine

                  +----------------------+
                  |         Seeds        |
+------+----------+----------+-----------+
| Bin  |  Center  |   Mean   |  Alloc'd  |
+------+----------+----------+-----------+
]],
                            tostring(self.parent),
                            self.seeds / n, math.sqrt(self.error / self.seeds),
                            self.triangles[1] / n, self.triangles[2] / n))

      for i, bin in ipairs(self.bins) do
         message(string.format([[
|% 6d|% 10.1f|% 10d|% 11d|
]],
                               i, bin[1] / n, bin[2] / n, bin[3]))
      end
      
      message([[
+------+----------+----------+-----------+
]])
   end
}

shapeprofiler = primitives.graphic {
   link = function(self)
      local parent = self.parent

      self.triangles = parent.triangles
      self.diamonds = parent.diamonds

      self.start = techne.iterations
   end,
   
   draw = function(self)
      local parent = self.parent

      for i = 1, #self.triangles do
         self.triangles[i] = self.triangles[i] + parent.triangles[i]
      end
      
      for i = 1, #self.diamonds do
         self.diamonds[i] = self.diamonds[i] + parent.diamonds[i]
      end
   end,

   unlink = function(self)
      local n

      n = techne.iterations - self.start + 1

      message(string.format([[
Triangulation profile for node: %s
 
+----------------------------------+-------------------------------+          
|  Triangles                       |  Diamonds                     |          
+-----------+----------+-----------+-----------+---------+---------+
|  Alloc'd  |  Culled  |  Visible  |  Alloc'd  |  Split  |  Merge  |
+-----------+----------+-----------+-----------+---------+---------+
|% 11d|% 10d|% 11d|% 11d|% 9d|% 9d|                
+-----------+----------+-----------+-----------+---------+---------+
 
+----------------+
|  Times (ms)    |
+-------+--------+---------+------------+
| Setup | Recull | Reorder | Tessellate |
+-------+--------+---------+------------+
|% 7.1f|% 8.1f|% 9.1f|% 12.1f|
+-------+--------+---------+------------+
]],
                            tostring(self.parent),
                            self.triangles[1] / n, self.triangles[2] / n,
                            self.triangles[3] / n,
                            self.diamonds[1] / n, self.diamonds[2] / n,
                            self.diamonds[3] / n,
                            1e3 * self.parent.profile[3] / n,
                            1e3 * self.parent.profile[4] / n,
                            1e3 * self.parent.profile[5] / n,
                            1e3 * self.parent.profile[6] / n))
   end
}

topography = {
   elevation = function (parameters)
      local elevation, oldmeta

      elevation = core.elevation(parameters)
      oldmeta = getmetatable(elevation)

      return replacemetatable(elevation, {
                                 __index = function(self, key)
                                    local value

                                    value = oldmeta.__index(self, key)
                                    
                                    if key == 'seeds' then
                                       return function(parameters)
                                          local seeds

                                          seeds = value(parameters)
                                          seeds.profiler = seedsprofiler

                                          return seeds
                                       end
                                    elseif key == 'shape' then
                                       return function(parameters)
                                          local shape

                                          shape = value(parameters)
                                          shape.profiler = shapeprofiler
                                          
                                          return shape
                                       end
                                    else
                                       return value
                                    end
                                 end
                                         })
   end
}

for key, value in pairs(core) do
   if not topography[key] then
       topography[key] = value
   end
end

return topography
