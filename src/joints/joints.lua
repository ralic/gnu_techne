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

local core = require 'joints.core'

if not options.drawjoints then
   return core
end

local math = require 'math'
local table = require 'table'
local shading = require 'shading'
local shapes = require 'shapes'
local arraymath = require 'arraymath'
local array = require 'array'
local primitives = require 'primitives'

local red = {1, 0, 0, 1}
local white = {1, 1, 1, 1}
local gold = {1, 0.69, 0, 1}
local lightslategray = {0.1445, 0.2281, 0.3036}
local steelblue = {0.0374, 0.2031, 0.4535}
local ivory3 = {0.5812, 0.6223, 0.4535}
local lightgoldenrod = {0.8441, 0.8667, 0.6463}
local yellowgreen = {0.2805, 0.6223, 0.0164}
local darkseagreen = {0.2319, 0.5084, 0.2559}

return {
   hinge = function (parameters)
	    local hinge

	    hinge = core.hinge (parameters)

	    hinge.schematic = primitives.node {
	       shaft = shading.flat {
		  color = lightgoldenrod,

		  draw = function (self)
		     local a, d, c, e

		     d = self.bias or 0.1
		     c = self.gain or 1 / 628

		     a = hinge.anchor
		     e = arraymath.combine(a, hinge.axis, 1, d + c * hinge.rate)

		     self.lines.positions = array.doubles {a, e}
		     self.points.positions = array.doubles {a, e}
		  end,

		  lines = shapes.line {},
		  points = shapes.points {},
				    },

	       arms = shading.flat {
		  color = ivory3,

		  draw = function (self) 
		     local a, b, pair

		     pair = hinge.pair
		     
		     if pair then
			local positions = {}

			a = pair[1] and pair[1].position
			b = pair[2] and pair[2].position

			if a and b then
			   self.lines.positions = array.doubles{a,
								hinge.anchor,
								b}

			   self.points.positions = array.doubles{a, b}
			elseif a then
			   self.lines.positions = array.doubles{hinge.anchor, a}
			   self.points.positions = array.doubles{a}
			elseif b then
			   self.lines.positions = array.doubles{hinge.anchor, b}
			   self.points.positions = array.doubles{b}
			end
		     end
		  end,

		  lines = shapes.line {},
		  points = shapes.points {},
					 }
					      }
	    return hinge
	 end,

   slider = function (parameters)
	    local slider

	    slider = core.slider (parameters)

	    slider.schematic = primitives.graphic {
	       draw = function (self)
	    	  local a, b, L, l, h, x, d, stops, pair

	    	  stops = slider.stops
	    	  pair = slider.pair

	    	  if not pair then
	    	     return
	    	  end

	    	  a = pair[1] and pair[1].position
	    	  b = pair[2] and pair[2].position
	    	  x = slider.axis
	    	  d = slider.position

	    	  if a and b then
		     l, h = slider.stops[1][1], slider.stops[1][2]

	    	     if l ~= -1 / 0 or h ~= 1 / 0 then
			if math.abs(d - l) < 1e-3 * (h - l) then
			   self.tube.color = red
			else
			   self.tube.color = gold
			end

			if math.abs(d - h) < 1e-3 * (h - l) then
			   self.rod.color = red
			else
			   self.rod.color = white
			end

	    		d = d - l
		     else
			d = -0.5 * arraymath.dot (arraymath.subtract(b, a), x)
	    	     end
	    	  end
		  
		  self.tube.lines.positions = array.doubles {
		     a, arraymath.combine(b, x, 1, d)
							    }
		  
		  self.rod.lines.positions = array.doubles {
		     b, arraymath.combine(b, x, 1, d)
							   }
	       end,

	       rod = shading.flat {
		  color = white,

	    	  lines = shapes.line {},
	    	  points = shapes.points {},
	    			  },

	       tube = shading.flat {
		  color = gold,

	    	  lines = shapes.line {},
	    	  points = shapes.points {},
	    			   }
	    				       }

	    return slider
	 end,

   universal = function (parameters)
      local universal

      universal = core.universal (parameters)

      universal.schematic = primitives.node {
	 shafts = shading.flat {
	    color = yellowgreen,

	    draw = function (self)
	       local a, d, c, e, f, x, y

	       d = self.bias or 0.1
	       c = self.gain or 1 / 628

	       a = universal.anchor
	       x, y = table.unpack(universal.axes)
	       e = arraymath.combine(a, x, 1, d + c * universal.rates[1])

	       f = arraymath.combine(a, y, 1, d + c * universal.rates[2])

	       self.lines.positions = array.doubles {f, a, e}
	       self.points.positions = array.doubles {f, a, e}
	    end,

	    lines = shapes.line {},
	    points = shapes.points {},
			      },

	 arms = shading.flat {
	    color = darkseagreen,

	    draw = function (self) 
	       local a, b, pair

	       pair = universal.pair
	       
	       if pair then
		  local positions = {}

		  a = pair[1] and pair[1].position
		  b = pair[2] and pair[2].position

		  if a and b then
		     self.lines.positions = array.doubles{a,
							  universal.anchor,
							  b}

		     self.points.positions = array.doubles{a, b}
		  elseif a then
		     self.lines.positions = array.doubles{universal.anchor, a}
		     self.points.positions = array.doubles{a}
		  elseif b then
		     self.lines.positions = array.doubles{universal.anchor, b}
		     self.points.positions = array.doubles{b}
		  end
	       end
	    end,

	    lines = shapes.line {},
	    points = shapes.points {},
			     }
					    }
      return universal
   end,

  spherical = function (parameters)
      local spherical

      spherical = core.spherical (parameters)

      spherical.schematic = primitives.node {
	 arms = shading.flat {
	    color = darkseagreen,

	    draw = function (self) 
	       local a, b, pair

	       pair = spherical.pair

	       if pair then
		  local positions

		  a = pair[1] and pair[1].position
		  b = pair[2] and pair[2].position

		  if a and b then
		     positions = array.doubles{a, spherical.anchor, b}
		  elseif a then
		     positions = array.doubles{spherical.anchor, a}
		  elseif b then
		     positions = array.doubles{spherical.anchor, b}
		  end

                  self.lines.positions = positions
                  self.points.positions = positions
	       end
	    end,

	    lines = shapes.line {},
	    points = shapes.points {},
			     }
					    }
      return spherical
  end,

  angular = core.angular,
  planar = core.planar,
  clamp = core.clamp,
  contact = core.contact,
  euler = core.euler,
  gearing = core.gearing,
  doublehinge = core.doublehinge,
  doubleball = core.doubleball,
  linear = core.linear,

  spring = function (parameters)
     local oldmeta, self
     local stiffness, damping, preload = 100, 10, 0

     self = joints.slider {
        stops = {{0, 0}, physics.spring(100, 10), 0},

        attach = function (self, a, b)
           if a and b then
              self.axis = arraymath.normalize(arraymath.subtract(a.position,
                                                                 b.position))
           end
        end,
                          }
     
     oldmeta = getmetatable (self)

     replacemetatable (self, {
                          __index = function (self, key)
                             local meta = getmetatable(self)

                             if key == "stiffness" then
                                return stiffness
                             elseif key == "damping" then
                                return damping
                             elseif key == "preload" then
                                return preload
                             else
                                return oldmeta.__index (self, key)
                             end
                          end,

                          __newindex = function (self, key, value)
                             local meta = getmetatable(self)

                             if key == "stiffness" then
                                stiffness = value
                             elseif key == "damping" then
                                damping = value
                             elseif key == "preload" then
                                preload = value
                             else
                                oldmeta.__newindex(self, key, value)
                             end

                             if key == "stiffness" or
                                key == "damping" then
                                local stops = self.stops

                                stops[2] = physics.spring (stiffness, damping)
                                self.stops = stops
                             end

                             if key == "preload" then
                                local stops = self.stops

                                stops[1] = {preload, preload}
                                self.stops = stops
                             end
                          end,

                          __tostring = function(self)
                             return "Spring"
                          end
                             })

     self.stiffness = 100
     self.damping = 10
     self.preload = 0

     for key, value in pairs(parameters) do
        self[key] = value
     end

     return self
  end,

  torsion = function (parameters)
     local oldmeta, self
     local stiffness = {100, 100, 100}
     local damping = {10, 10, 10}
     local preload = {0, 0, 0}

     self = joints.euler {
        stops = {
           {{0, 0}, physics.spring(100, 10), 0},
           {{0, 0}, physics.spring(100, 10), 0},
           {{0, 0}, physics.spring(100, 10), 0},
        }
                         }

     oldmeta = getmetatable (self)

     replacemetatable (self, {
                          __index = function (self, key)
                             local meta = getmetatable(self)

                             if key == "stiffness" then
                                return stiffness
                             elseif key == "damping" then
                                return damping
                             elseif key == "preload" then
                                return preload
                             else
                                return oldmeta.__index (self, key)
                             end
                          end,

                          __newindex = function (self, key, value)
                             local meta = getmetatable(self)

                             if key == "stiffness" then
                                stiffness = value
                             elseif key == "damping" then
                                damping = value
                             elseif key == "preload" then
                                preload = value
                             else
                                oldmeta.__newindex(self, key, value)
                             end

                             if key == "stiffness" or
                                key == "damping" then
                                local k_s = stiffness
                                local k_d = damping
                                local stops = self.stops

                                if type(k_s) == "number" then
                                   k_s = {k_s, k_s, k_s}
                                end

                                if type(k_d) == "number" then
                                   k_d = {k_d, k_d, k_d}
                                end

                                for i = 1, 3 do
                                   stops[i][2] = physics.spring (k_s[i], k_d[i])
                                end

                                self.stops = stops
                             end

                             if key == "preload" then
                                local stops = self.stops
                                local delta = {
                                   preload[1],
                                   preload[2],
                                   preload[3]
                                }

                                for i = 1, 3 do
                                   stops[i][1] = {delta[i], delta[i]}
                                end

                                self.stops = stops
                             end
                          end,

                          __tostring = function(self)
                             return "Torsion"
                          end
                             })
     
     for key, value in pairs(parameters) do
        self[key] = value
     end

     return self
  end
}
