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

local math = require "math"

local units = {
   -- Weight.
   
   pounds = function (m)
      return m * 0.45359237
   end,
   
   kilograms = function (m)
      return m
   end,

   -- Distance.
   
   meters = function (l)
      return l
   end,

   kilometers = function (l)
      return 1000 * l
   end,

   inches = function (l)
      return l * 0.0254
   end,

   feet = function (l)
      return l * 0.3048
   end,

   miles = function (l)
      return l * 1609.344
   end,

   -- Speed.
   
   milesperhour = function (v)
      return v * 0.44704
   end,

   kilometersperhour = function (v)
      return v / 3.6
   end,

   knots = function (v)
      return v * 0.5144
   end,

   -- Area.

   squaremeters = function (A)
      return A
   end,

   squarefeet = function (A)
      return A * 0.3048^2
   end,

   -- Moment of inertia.
   
   kilogramssquaremeters = function (I)
      return I
   end,
   
   slugsquarefeet = function (I)
      return I * 14.593902 * 0.3048^2
   end,

   -- Power

   watts = function (P)
      return P
   end,

   metrichorsepower = function (P)
      return P * 735.5
   end,

   mechanicalhorsepower = function (P)
      return P * 745.7
   end,

   -- Frequency.

   rotationsperminute = function (nu)
      return nu / 30 * math.pi
   end,

   hertz = function (N)
      return N * 2 * math.pi / 60
   end,

   -- Angles.
   
   degrees = function (theta)
      return theta / 180 * math.pi
   end,

   radians = function (theta)
      return theta
   end,

   -- Temperature.
   
   kelvin = function (T)
      return T
   end,

   celsius = function (T)
      return T + 273.15
   end,

   -- Density.
   
   kilogramspercubicmeter = function (rho)
      return rho
   end,

   slugspercubicfeet = function (P)
      return P * 14.593902 / 0.3048^3
   end,

   -- Pressure.
   
   newtonspersquaremeter = function (P)
      return rho
   end,
   
   --
   
   slugspersquarefeet = function (P)
      return P * 14.593902 / 0.3048^2
   end,
}

local function wrapper (f)
   return function (x)
	     if type(x) == "table" then
		local t = {}

		for i, v in ipairs (x) do
		   t[i] = f(x[i])
		end

		return t
	     else
		return f(x)
	     end
	  end
end

local wrapped = {}

for k, v in pairs (units) do
   if type(v) == "function" then
      wrapped[k] = wrapper (v)
   else
      wrapped[k] = v
   end
end

return wrapped
