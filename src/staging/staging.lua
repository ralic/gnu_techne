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

arraymath = require "arraymath"

local staging = {}

function staging.orbit (values)
   local orbit
   local command, rest

   rest = values.command
   command = values.command
   values.command = nil

   orbit = primitives.joint {
      dummy = primitives.node {
         link = function (self)
            local p, R

            p = orbit.parent.position or {0, 0, 0}
            R = arraymath.euler (0, rest[2], rest[3])

            -- Configure the rig.

            orbit.ball = joints.spherical {
               anchor = p,
                                         }

            orbit.spring = joints.euler {
               axes = {
                  R[2], R[3], {0, 0, 0}
               },

               stops = {
                  {{0, 0}, {3000, 1000}, 0},
                  {{0, 0}, {3000, 1000}, 0},
                  {{0, 0}, {3000, 1000}, 0},
               }
                                       }

            orbit.attach = function (self)
               local pair = self.pair

               self.ball.bodies = self.pair
               self.spring.bodies = self.pair
            end
            
            orbit.torso = bodies.point {
               position = p,
               mass = physics.spheremass (0.0001, 0.1),

               neck = joints.slider {
                  stops = {{0, 0}, {3000, 1000}, 0},
                  axis = R[3],

                  head = bodies.point {
                     position = arraymath.matrixmultiplyadd (R, {0, 0, -rest[1]}, p),

                     orientation = R,

                     mass = physics.spheremass (0.0001, 0.1),
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

                              stops = self.spring.stops

                              a = math.clamp (command[2] - rest[2],
                                              -math.pi, math.pi)
                              b = math.clamp (command[3] - rest[3],
                                              -math.pi, math.pi)

                              stops[1][1] = {a, a}
                              stops[3][1] = {b, b}

                              self.spring.stops = stops
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
