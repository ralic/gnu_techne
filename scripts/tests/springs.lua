resources.dofile "common.lua"

graphics.perspective = {45, 0.1, 10000}
dynamics.gravity = {0, 0, -9.81}

root = primitives.root {
   observer = primitives.observer {
      position = {3, 0, 0},
      orientation = arraymath.rotation(units.degrees(90), 2)
   },

   environment = bodies.environment {
                                    },
                       }

-- Torsion springs.

local a = {}

for i = 1, 3 do
   a[i] = bodies.box {
      position = {0, -1, -1 + 0.5 * i},
      size = {0.1, 0.1, 0.4},

      mass = {
         0.04,
         {0, 0, 0},
         {
            {0.001, -0.0008, 0}, 
            {-0.0008, 0.001, 0}, 
            {0,       0,     0.0009}
         }, 
      }
                         }
end

for i = 1, 2 do
   a[i].foo = primitives.joint {
      ball = joints.spherical {
         anchor = {0, -1, -0.75 + 0.5 * i},
                         },

      spring = joints.euler {
         stops = {
            {{0, 0}, {3, 0.1}, 0},
            {{0, 0}, {3, 0.1}, 0},
            {{0, 0}, {3, 0.1}, 0},
         }
                            },
      
      bar = a[i + 1],

      attach = function (self)
         local pair = self.pair

         self.ball.bodies = self.pair
         self.spring.bodies = self.pair
      end
                              }
end

root.environment[1] = a[1]

-- Compression springs.

local b = {}

for i = 1, 3 do
   b[i] = bodies.box {
      position = {0, 1, -1.25+ 0.75 * i},
      size = {0.1, 0.1, 0.4},

      mass = {
         0.04,
         {0, 0, 0},
         {
            {0.001, 0,     0}, 
            {0,     0.001, 0}, 
            {0,     0,     0.0009}
         }, 
      }
                         }
end

for i = 1, 2 do
   b[i].foo = joints.slider {
      stops = {{0, 0}, {50, 1}, 0},
      bar = b[i + 1]
                            }
end

root.environment[2] = b[1]

root.timer = primitives.timer {
   period = 3,

   tick = function()
      physics.addforce(a[3], {100 * math.random() - 50,
                              100 * math.random() - 50,
                              0})
      
      physics.addforce(b[3], {0, 0, 100 * math.random() - 50})
   end
                              }

