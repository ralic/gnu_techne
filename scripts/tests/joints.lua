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

local resources = require "resources"
local graphics = require "graphics"
local dynamics = require "dynamics"
local primitives = require "primitives"
local joints = require "joints"
local bodies = require "bodies"
local physics = require "physics"
local units = require "units"

resources.dofile "utils/basic.lua"

local r_w, d_w = 0.65, 0.4

graphics.perspective = {units.degrees(50), 0.1, 10000}
dynamics.gravity = {0, 0, -9.81}

root = primitives.root {
   orbit = resources.dofile ("utils/orbit.lua", -10, units.degrees(0), units.degrees(60)),

   environment = bodies.environment {
      plane = bodies.plane {},
                                    },

   base = bodies.box {
      position = {-1, 0, 0.2},

      size = {0.5, 0.5, 0.2},
      mass = physics.boxmass (0.0001, 0.5, 0.5, 0.2),

      slider = joints.slider {
         axis = {0, 0, 1},
         stops = {{-1, -1}, {3000, 1000}, 0.1},

         otherbox = bodies.box {
            position = {-1, 0, 0.7},

            size = {0.4, 0.4, 0.2},
            mass = physics.boxmass (0.0001, 0.4, 0.4, 0.2),
                           },
                                  }
                        },

   otherbase = bodies.box {
      position = {0, 0, 0.2},

      size = {0.5, 0.5, 0.2},
      mass = physics.boxmass (0.0001, 0.5, 0.5, 0.2),

      hinge = joints.hinge {
         axis = {0, 0, 1},
         motor = {10, 100},

         flywheel = bodies.cylinder {
            position = {0, 0, 0.5},

            radius = 0.5,
            length = 0.2,

            mass = physics.cylindermass (0.0001, 0.5, 0.2),
                                    },
                           },

      slider = joints.slider {
         inverted = true,
         axis = {0, 0, 1},
         stops = {{-1, -1}, {3000, 1000}, 0.1},
                             }
                          },
                       }
