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

local math = require "math"
local arraymath = require "arraymath"
local primitives = require "primitives"
local bodies = require "bodies"
local joints = require "joints"
local physics = require "physics"

local staging = {}

function staging.orbit (values)
   local orbit
   local command, compliance, mass, rest

   compliance = {3000, 400}
   mass = physics.spheremass (0.0001, 0.1)
   rest = values.command
   command = values.command
   values.command = nil

   orbit = joints.universal {
      dummy = primitives.node {
         link = function (self)
            local p, R, R_p, RT, R_pT

            p = self.ancestors[2].position or {0, 0, 0}
            R_p = self.ancestors[2].orientation or arraymath.scaling(1)
            R_pT = arraymath.transpose(R_p)
            R = arraymath.concatenate(R_p,
                                      arraymath.relue (0, rest[3], rest[2]))
            RT = arraymath.transpose(R)

            -- Configure the rig.

            self.parent.anchor = p
            self.parent.axes = {
               R_pT[3], R_pT[2], 
            }

            self.parent.stops = {
               {{0, 0}, compliance, 0},
               {{0, 0}, compliance, 0},
            }
            
            self.parent.torso = bodies.point {
               position = p,
               mass = mass,

               neck = joints.slider {
                  stops = {{0, 0}, compliance, 0},
                  axis = RT[3],

                  head = bodies.point {
                     position = arraymath.matrixmultiplyadd (RT, {0, 0, -rest[1]}, p),

                     orientation = R,

                     mass = mass,
                     eye = primitives.observer {}
                                      },
                                    },
                                             }
         end,
                              }
                           }

   oldmeta = getmetatable(orbit)

   replacemetatable (orbit, {
                        __index = function(self, key)
                           if key == "command" then
                              return command
                           elseif key == "compliance" then
                              return compliance
                           elseif key == "mass" then
                              return mass
                           else 
                              return oldmeta.__index(self, key)
                           end
                        end,

                        __newindex = function(self, key, value)
                           if key == "command" then
                              local stops

                              command = {
                                 value[1],
                                 math.clamp (value[2],
                                             -math.pi + rest[2],
                                             math.pi + rest[2]),
                                 math.clamp (value[3],
                                             -math.pi + rest[3],
                                             math.pi + rest[3])
                              }

                              -- Reconfigure the slider.

                              stops = self.torso.neck.stops

                              a = command[1] - rest[1]
                              stops[1] = {a, a}

                              self.torso.neck.stops = stops

                              -- Reconfigure the ball joint srping.

                              stops = self.stops

                              a = math.clamp (command[2] - rest[2],
                                              -math.pi, math.pi)
                              b = math.clamp (command[3] - rest[3],
                                              -math.pi, math.pi)

                              stops[2][1] = {a, a}
                              stops[1][1] = {b, b}

                              self.stops = stops
                           elseif key == "compliance" then
                              local stops

                              compliance = value

                              -- Reconfigure the slider.

                              stops = self.torso.neck.stops
                              stops[2] = value
                              self.torso.neck.stops = stops

                              -- Reconfigure the ball joint srping.

                              self.stops = stops
                              stops[2] = value
                              stops = self.stops
                           elseif key == "mass" then
                              mass = value
                              
                              self.torso.mass = value
                              self.torso.neck.head.mass = value
                           else
                              oldmeta.__newindex(self, key, value)
                           end
                        end
                           })
   
   for key, value in pairs(values) do
      orbit[key] = value
   end

   return orbit
end

return staging
