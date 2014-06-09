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

local core = require 'textures.core'

local textures = {}

for k, v in pairs(core) do
   textures[k] = v
end

textures.planar = function (parameters)
   local planar
   local texels

   -- Specify the texels last so that we can properly decide whether
   -- to generate mipmaps or not.
   
   texels, parameters.texels = parameters.texels, nil
   planar = core.planar (parameters)
   planar.texels = texels

   return planar
end

return textures
