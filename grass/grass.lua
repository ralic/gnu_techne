local graphics = require 'graphics'
local units = require 'units'

graphics.samples = 4
graphics.window = options.fullscreen and graphics.screen or {options.width or 1280, options.height or 800}
graphics.title = "Grass"
graphics.hide = false

graphics.perspective = {units.degrees(45), .1, 50000}

require 'input'
require 'bindings'

local bindings = require 'bindings.default'
local resources = require 'resources'
local textures = require 'textures'
local primitives = require 'primitives'
local topography = require 'topography'

local techne = require 'techne'
local dynamics = require 'dynamics'

local math = require 'math'
local array = require 'array'
local arraymath = require 'arraymath'
local string = require 'string'

techne.iterate = true

dynamics.stepsize = 0.001
dynamics.ceiling = 0.1
dynamics.timescale = 1
dynamics.surfacelayer = 10e-3
dynamics.gravity = {0, 0, -9.81}

local samples = resources.dofile ("grass/elevation.lc")
local imagery = resources.dofile ("grass/imagery.lc")
local r = 1

local elevation = topography.elevation {
   depth = 11,
   resolution = {5, 5},

   albedo = 1.3,
   separation = 4,

   swatches = {
      {
         {120 / 360, 0.3, nil},

         {
            {
               textures.planar{
                  texels = resources.dofile ("grass/grass-detail-1.lc"),
                  wrap = {"repeat", "repeat"},
                  filter = {"linear-mipmap-linear", "linear"},
               },
               arraymath.scale({1, 1}, 0.1 * r),
            },

            {
               textures.planar{
                  texels = resources.dofile ("grass/grass-detail-2.lc"),
                  wrap = {"repeat", "repeat"},
                  filter = {"linear-mipmap-linear", "linear"},
               },
               arraymath.scale({1, 1}, 0.01 * r),
            },

            {
               textures.planar{
                  texels = resources.dofile ("grass/grass-detail-3.lc"),
                  wrap = {"repeat", "repeat"},
                  filter = {"linear-mipmap-linear", "linear"},
               },
               arraymath.scale({1, 1}, 0.001 * r),
            },
         },
      },

      {
         {40 / 360, 0.2, nil},

         {
            {
               textures.planar {
                  texels = resources.dofile ("grass/dirt-detail-1.lc"),
                  wrap = {"repeat", "repeat"},
                  filter = {"linear-mipmap-linear", "linear"},
               },
               arraymath.scale({1, 1}, 0.1 * r),
            },

            {
               textures.planar {
                  texels = resources.dofile ("grass/dirt-detail-2.lc"),
                  wrap = {"repeat", "repeat"},
                  filter = {"linear-mipmap-linear", "linear"},
               },
               arraymath.scale({1, 1}, 0.01 * r),
            },

            {
               textures.planar {
                  texels = resources.dofile ("grass/dirt-detail-3.lc"),
                  wrap = {"repeat", "repeat"},
                  filter = {"linear-mipmap-linear", "linear"},
               },
               arraymath.scale({1, 1}, 0.001 * r),
            },
         },
      },
   },

   tiles = {
      {
         {
            array.transpose(array.reshape(2049, 2049, samples[1]), 1, 2),
            array.transpose(array.reshape(2049, 2049, samples[2]), 1, 2),
            array.transpose(array.reshape(2049, 2049, 3, imagery), 1, 2, 3),
            {900, 0},
         }
      }
   }
}

local blade = textures.planar{
   texels = resources.dofile ("grass/blade.lc"),
   wrap = {"clamp", "clamp"},
   filter = {"linear-mipmap-linear", "linear"},
}

root = primitives.root {
   atmosphere = topography.atmosphere {
      tag = "atmosphere",

      size = {1024, 512},

      turbidity = 4,

      rayleigh = {6.95e-06, 1.18e-05, 2.44e-05},
      mie = 7e-5,

      sun = {units.degrees(60), units.degrees(40)},
   },

   splat = elevation.splat {
      index = -1,
      tag = "splat",
      position = {0, 0, 0},

      shape = elevation.shape {
         tag = "elevation",

         target = 20000,
      },
   },

   vegetation = elevation.vegetation {
      tag = "vegetation",

      horizon = 70,
      rolloff = 0.75,
      ceiling = 9455,
      density = options.density or 100000,
      clustering = options.clustering or 8,

      topography.grass {
         mask = blade,
         detail = 8,
         height = {0.035, 0.065},
         width = {0.0025, 0.002},
         ambient = {0.7, 0.7},
         diffuse = {1.1, 0.5},
         attenuation = 0.075;
         specular = {0.3, 12},
         stiffness = 0.15,
         threshold = 1000,
      },

      -- topography.seeds {
      --    tag = "seeds",
      --    color = {0, 1, 0},
      --    height = 0.001,
      --    threshold = 000,
      -- },

      -- topography.seeds {
      --    tag = "moreseeds",
      --    color = {1, 0, 0},
      --    height = 0.001,
      --    threshold = 000,
      -- },
   },

   timer = options.timed and primitives.timer {
      source = "dynamics",
      period = 30,

      tick = function(self)
         techne.iterate = false
      end,
   }
}

local staging = require 'staging'
local c = {
   {0.18400, 0.00000, -55.20000, 370.00000},
   {0.00000, -0.00000, 0.00000, 2.00000},
   {-0.03400, 1.02000, 0.00000, 2.00000},
}

root.path = staging.bezier {
   vertices = resources.dofile ("grass/path.lua"),

   step = function (self, h, t)
   end,

   transform = function (self)
      local t = dynamics.time

      if t < 10 then
         self.speed = c[1][1] * t * t * t +
            c[1][2] * t * t +
            c[1][3] * t +
            c[1][4]
      elseif t < 20 then
         t = t - 10
         self.speed = c[2][1] * t * t * t +
            c[2][2] * t * t +
            c[2][3] * t +
            c[2][4]
      else
         t = t - 20
         self.speed = c[3][1] * t * t * t +
            c[3][2] * t * t +
            c[3][3] * t +
            c[3][4]
      end

      -- trace (dynamics.time, self.speed)
   end,

   camera = primitives.observer {
      orientation = arraymath.diagonal {1, 1, 1},
   }
}

--------------

-- bindings['*'] = print
bindings['escape'] = function ()
   techne.iterate = false
end

bindings['f'] = function ()
   if (graphics.window[1] == graphics.screen[1] and
       graphics.window[2] == graphics.screen[2]) then
      graphics.window = {1024, 768}
   else
      graphics.window = graphics.screen
   end
end

bindings['down-control_l c'] = function ()
   techne.interactive = true
end

local grassnode
bindings['g'] = function ()
   if not grassnode then
      grassnode = root.grass
      root.grass = nil
   else
      root.grass = grassnode
      grassnode = nil
   end
end

bindings['c'] = function()
   tags.vegetation.clustering = tags.vegetation.clustering == 1 and 8 or 1
end

tags.splat.splatmin = 1.5
tags.vegetation.splatmin = tags.splat.splatmin
tags.splat.splatmax = 5.5
tags.vegetation.splatmax = tags.splat.splatmax
