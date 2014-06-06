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

local techne = require "techne"
local graphics = require "graphics"
local title, defaultwidth, defaultheight = ...

techne.iterate = true
graphics.samples = 0
graphics.window = options.fullscreen and graphics.screen or
                  {options.width or defaultwidth or 800,
                   options.height or defaultheight or 600}
graphics.title = title or 'Techne'
graphics.hide = false

require "bindings"
local bindings = require "bindings.basic"

bindings['escape'] = function ()
                        techne.iterate = false
                     end

bindings['q'] = function ()
                   techne.iterate = false
                end

bindings['down-control_l c'] = function ()
   techne.interactive = true
end
