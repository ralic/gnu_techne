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

local graphics = require 'graphics'
local dynamics = require 'dynamics'
local primitives = require 'primitives'
local string = require 'string'
local io = require 'io'
local array = require 'array'
local arraymath = require 'arraymath'
local pipe

if options.tape then
   vcr = primitives.root {
      index = 1 / 0,

      encoder = primitives.graphic {
         recording = true,

         link = function (self)
            local command

            dynamics.interval = 1 / (options.framerate or 24)
            command = string.format ("mencoder /dev/stdin -really-quiet -demuxer rawvideo -rawvideo w=%d:h=%d:format=rgba:fps=%d -flip -ovc x264 -x264encopts tune=%s:preset=%s:crf=%d -o %s", graphics.window[1],
                                     graphics.window[2],
                                     options.framerate or 24,
                                     options.tune or "animation",
                                     options.preset or "veryslow",
                                     options.crf or 23,
                                     options.tape)

            print(string.format ("Output is being recorded using " ..
                                    "the command:\n%s", command))

            pipe = io.popen (command, "w")
         end,

         unlink = function (self)
            io.close (pipe)
         end,

         draw = function (self)
            if self.recording then

               if (options.fadein and type(options.fadein) == "number" and
                   graphics.frames < options.fadein) then
                  pipe:write (array.dump(
                                 graphics.colorbuffer * (graphics.frames /
                                                            options.fadein)))
               elseif (options.fadein and type(options.fadein) == "table" and
                          (graphics.frames >= options.fadein[1] and
                           graphics.frames < options.fadein[2])) then
                  local c

                  c = (graphics.frames - options.fadein[1]) /
                     (options.fadein[2] - options.fadein[1])

                  pipe:write (array.dump(graphics.colorbuffer, c))
               elseif (options.fadeout and
                       graphics.frames > options.fadeout[1]) then
                  local c

                  c = (options.fadeout[2] - graphics.frames) /
                      (options.fadeout[2] - options.fadeout[1])
                  pipe:write (array.dump(graphics.colorbuffer * c))
               else
                  pipe:write (array.dump(graphics.colorbuffer))
               end
            end
         end,

         remote = primitives.event {
            keypress = function (self, key)
               if key == "control_l" then
                  self.armed = true
               elseif key == "r" and self.armed then
                  self.parent.recording = not self.parent.recording
               end
            end,

            keyrelease = function (self, key)
               if key == "control_l" then
                  self.armed = false
               end
            end,
         }
      }
   }
end
