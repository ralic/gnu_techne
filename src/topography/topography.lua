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

vegetationprofiler = primitives.graphic {
   link = function(self)
      local parent = self.parent

      self.initial = {techne.iterations,
                      parent.infertile,
                      parent.drawn,
                      parent.parent.segments}
   end,
   
   -- draw = function(self)
   --    trace (self.parent.segments)
   -- end,
   
   unlink = function(self)
      if self.initial[2] then 
         local parent = self.parent
         local c = 1 / (techne.iterations - self.initial[1])
         local s, t, q
         
         s = c * (parent.infertile - self.initial[2])
         t = c * (parent.drawn - self.initial[3])
         q = c * (parent.parent.segments - self.initial[4])
         
         message(string.format([[

Vegetation profile for node: %s
 
+----------------------------------------------+
| Seeds                                        |
+-----------+-----------+-----------+----------+
| Infertile |   Drawn   |   Total   | Segments |
+-----------+-----------+-----------+----------+
|% 11.0f|% 11.0f|% 11.0f|% 10.0f|
+-----------+-----------+-----------+----------+
]],
                               tostring(self.parent), s, t, s + t, q))
      end
   end
}
              
seedsprofiler = primitives.graphic {
   link = function(self)
      local parent = self.parent

      self.iterations = 0
      self.triangles = {0, 0}
      self.error = 0
      self.bins = {}

      for i = 1, #parent.bins do
         self.bins[i] = {0, 0, 0, 0}
      end
   end,
   
   draw = function(self)
      local parent = self.parent
      local patches = 0
      
      for i = 1, #parent.bins do
         self.bins[i][1] = self.bins[i][1] + parent.bins[i][1]
         self.bins[i][2] = self.bins[i][2] + parent.bins[i][2]
         self.bins[i][3] = self.bins[i][3] + parent.bins[i][3]
         self.bins[i][4] = parent.bins[i][4]

         patches = patches + parent.bins[i][2]
      end
      
      self.triangles[1] = self.triangles[1] + parent.triangles[1]
      self.triangles[2] = self.triangles[2] + parent.triangles[2]

      if parent.error > 0 then
         self.error = self.error + math.sqrt(parent.error / patches)
      end

      self.iterations = self.iterations + 1
   end,

   unlink = function(self)
      local c, cummulative

      c = 1 / self.iterations
      
      message(string.format([[

Seed profile for node: %s
Within-cluster RMS error: %.1f
Mean number of triangles visited: %d roam, %d fine

+-----+--------+-----------+----------+------------+---------+
| Bin | Center | Triangles | Clusters | Cum. Clus. | Alloc'd |
+-----+--------+-----------+----------+------------+---------+
]],
                            tostring(self.parent), self.error,
                            c * self.triangles[1], c * self.triangles[2]))

      cummulative = 0
      
      for i = 1, #self.bins do
         cummulative = cummulative + self.bins[i][3]
         
         message(string.format([[
|% 5d|% 8.1f|% 11d|% 10d|% 12d|% 9d|
]],
                               i, c * self.bins[i][1], c * self.bins[i][2],
                               c * self.bins[i][3], c * cummulative,
                               self.bins[i][4]))
      end
      
      message([[
+-----+--------+-----------+---------+------------+---------+
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
 
+---------------------------------------+
|  Times (ms)                           |
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
                                    elseif key == 'vegetation' then
                                       return function(parameters)
                                          local vegetation

                                          vegetation = value(parameters)
                                          vegetation.profiler = vegetationprofiler
                                          
                                          return vegetation
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
