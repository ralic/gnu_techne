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

local string = require "string"
local math = require "math"
local array = require "array"
local arraymath = require "arraymath"
local units = require "units"
local primitives = require "primitives"
local bodies = require "bodies"
local joints = require "joints"
local bindings = require "bindings.orbit"
local widgets = require "widgets"
local controllers = require "controllers"
local utilities = require "utilities"

local parameters = ...

local rest = {parameters.radius,
              parameters.azimuth,
              parameters.elevation}
local initial = {parameters.radius,
                 parameters.azimuth,
                 parameters.elevation}
local current = {parameters.radius,
                 parameters.azimuth,
                 parameters.elevation}
local sensitivity = 3
local compliance = {1000000, 150000}
local mass = {
   1e-3,
   arraymath.zero(3),
   arraymath.diagonal(3, 1e-6)
}

local function update (self, rho, theta, phi)
   local stops

   if rho then
      current[1] = rho
   end

   if theta then
      current[2] = math.clamp (theta, -math.pi + rest[2], math.pi + rest[2])
   end

   if phi then
      current[3] = math.clamp (phi, -math.pi + rest[3], math.pi + rest[3])
   end

   -- Reconfigure the slider.

   stops = self.torso.neck.stops

   a = current[1] - rest[1]
   stops[1] = {a, a}

   self.torso.neck.stops = stops

   -- Reconfigure the ball joint srping.

   stops = self.stops

   a = math.clamp (current[2] - rest[2], -math.pi, math.pi)
   b = math.clamp (current[3] - rest[3], -math.pi, math.pi)

   stops[2][1] = {a, a}
   stops[1][1] = {b, b}

   self.stops = stops
end

local info = primitives.root {
   display = widgets.display {
      layout = widgets.layout {
         align = {-1, -1},
         color = {1, 1, 1},
         padding = {0.01, 0, 0.01, 0},
      }
   }
}

local orbit = primitives.transform {
   position = parameters.position,

   universal = joints.universal {
      link = function (self)
         local p, R, R_p, RT, R_pT

         p = self.parent.position or {0, 0, 0}
         R_p = self.parent.orientation or arraymath.diagonal(3, 1)
         R_pT = array.transpose(R_p)
         R = arraymath.concatenate(R_p, arraymath.relue (0, rest[3], rest[2]))
         RT = array.transpose(R)

         -- Configure the rig.

         self.anchor = p
         self.axes = {
            R_pT[3], RT[1],
         }

         self.stops = {
            {{0, 0}, compliance, 0},
            {{0, 0}, compliance, 0},
         }

         self.torso = bodies.point {
            position = p,
            mass = mass,

            neck = joints.slider {
               stops = {{0, 0}, compliance, 0},
               axis = RT[3],

               head = bodies.point {
                  position = arraymath.matrixmultiplyadd (RT, {0, 0, rest[1]}, p),

                  orientation = R,

                  mass = mass,
                  eye = primitives.observer {}
               },
            },
         }

         bindings['[Rubberband]down-button-1'] = function(sequence, button)
            initial = {current[1], current[2], current[3]}
         end

         bindings['[Rubberband]down-button-3'] = function(sequence, button)
            initial = {current[1], current[2], current[3]}
         end

         bindings['[Rubberband]relative-axis-0'] = function(sequence, value)
            sensitivity = math.clamp (sensitivity + value, 1, 10)

            info.timer = primitives.timer {
               period = 1,

               link = function(self)
                  info.display.layout.text = string.format ('<span font="Sans 12" color="white">Orbit camera sensitivity: %d</span>',sensitivity)
               end,

               tick = function(self, tick)
                  info.display.layout.text = ""
                  self.period = nil
               end
            }
         end

         bindings['[Rubberband]drag-absolute-axis-0'] = function(sequence, value, button)
            if button == 1 then
               update(self, nil,
                      initial[2] - math.ldexp(units.degrees(value), -sensitivity))
            end
         end

         bindings['[Rubberband]drag-absolute-axis-1'] = function(sequence, value, button)
            if button == 3 then
               update(self, initial[1] - math.ldexp(value, -sensitivity))
            elseif button == 1 then
               update(self, nil, nil,
                      initial[3] - math.ldexp(units.degrees(value), -sensitivity))
            end
         end
      end,
   }
}

local oldmeta = getmetatable(orbit)

replacemetatable(orbit, {
                    __index = function (self, key)
                       if key == "radius" then
                          return current[1]
                       elseif key == "azimuth" then
                          return current[2]
                       elseif key == "elevation" then
                          return current[3]
                       else
                          return oldmeta.__index (self, key)
                       end
                    end,

                    __newindex = function (self, key, value)
                       if key == "radius" then
                          update(self.universal, value, nil, nil)
                       elseif key == "azimuth" then
                          update(self.universal, nil, value, nil)
                       elseif key == "elevation" then
                          update(self.universal, nil, nil, value)
                       else
                          oldmeta.__newindex (self, key, value)
                       end
                    end
                        })

return orbit
