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

math = require "math"
resources = require "resources"
graphics = require "graphics"
primitives = require "primitives"
widgets = require "widgets"

resources.dofile "utils/basic.lua"

graphics.perspective = {45, 0.1, 10000}

root = primitives.root {
   display = widgets.display {
      padding = {0.1, 0.1, 0.1, 0.1},

      column = widgets.column {
         align = {-1, -1},

         first = widgets.row {
            padding = {0.01, 0.025, 0.01, 0.02},

            foo = widgets.layout {
               text = '<span font="Sans 17" color="White">foo</span>',
               color = {1, 1, 1},
               align = {-1, 1},
               padding = {0.03, 0.01, 0.01, 0.02},
                           },

            bar = widgets.layout {
               text = '<span font="Sans 17" color="White">bar</span>',
               color = {1, 1, 1},
               align = {1, 1},

               padding = {0.01, 0.025, 0.01, 0.02},
                           },
                     },

         second = widgets.row {
            padding = {0.01, 0.025, 0.01, 0.02},

            foobar = widgets.layout {
               text = '<span font="Sans 17" color="White">foobar</span>',
               color = {1, 1, 1},

               padding = {0.03, 0.01, 0.01, 0.02},
                           },

            barfoo = widgets.layout {
               text = '<span font="Sans 17" color="White">barfoo</span>',
               color = {1, 1, 1},

               padding = {0.01, 0.025, 0.01, 0.02},
                           },
                     },
                     },

      assembly = widgets.assembly {
         align = {1, 1},

         link = function(self)
            for i = 1, 12 do
               self[i] = widgets.layout {
                  offset = {0.1 * math.cos(-(i - 4) / 12 * 2 * math.pi),
                            0.1 * math.sin(-(i - 4) / 12 * 2 * math.pi)},

                  text = '<span font="Sans 17" color="White">' .. tostring(i) .. '</span>',
                  color = {1, 1, 1},
                                        }
            end
         end
                       }
                         }
                       }
