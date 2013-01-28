resources.dofile "common.lua"

graphics.perspective = {units.degrees(90), 0.1, 10000}
dynamics.timescale = 0

root = primitives.root {
   atmosphere = topography.atmosphere {
      size = {1024, 512},

      turbidity = 4,

      rayleigh = {6.95e-06, 1.18e-05, 2.44e-05},
      mie = 7e-5,

      sun = {1.74, units.degrees(15)},
                                      },
   observer = primitives.observer {
      spinner = primitives.timer {
         period = 0,

         tick = function (self, ticks, delta, elapsed)
            self.parent.orientation = arraymath.euler (0, units.degrees(130), elapsed*0)
         end,
                                     }
                                  }
                       }
