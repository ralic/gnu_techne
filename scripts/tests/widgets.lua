resources.dofile "common.lua"

graphics.perspective = {45, 0.1, 10000}
dynamics.timescale = 0

root = primitives.root {
   foo = widgets.display {
      padding = {0.1, 0.1, 0.1, 0.1},

      widgets.column {
         align = {-1, -1},

         widgets.row {
            padding = {0.01, 0.025, 0.01, 0.02},

            widgets.layout {
               text = '<span font="Sans 17" color="White">foo</span>',
               color = {1, 1, 1},
               align = {-1, 1},
               padding = {0.03, 0.01, 0.01, 0.02},
                           },

            widgets.layout {
               text = '<span font="Sans 17" color="White">bar</span>',
               color = {1, 1, 1},
               align = {1, 1},

               padding = {0.01, 0.025, 0.01, 0.02},
                           },
                     },

         widgets.row {
            padding = {0.01, 0.025, 0.01, 0.02},

            widgets.layout {
               text = '<span font="Sans 17" color="White">foobar</span>',
               color = {1, 1, 1},

               padding = {0.03, 0.01, 0.01, 0.02},
                           },

            widgets.layout {
               text = '<span font="Sans 17" color="White">barfoo</span>',
               color = {1, 1, 1},

               padding = {0.01, 0.025, 0.01, 0.02},
                           },
                     },
                     },

      widgets.assembly {
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
