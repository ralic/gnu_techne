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

local array = require 'array'
local arraymath = require 'arraymath.core'

function arraymath.zero(...)
   return arraymath.scale(array.doubles(...), 0)
end

function arraymath.tensor(u, v)
   return arraymath.matrixmultiply(array.cast(#u, 1, u), array.cast (1, #v, v))
end

function arraymath.euler(theta, phi, psi)
   return arraymath.concatenate(arraymath.rotation(theta, 1),
                                arraymath.rotation(phi, 2),
                                arraymath.rotation(psi, 3))
end

function arraymath.relue(theta, phi, psi)
   return arraymath.concatenate(arraymath.rotation(psi, 3),
                                arraymath.rotation(phi, 2),
                                arraymath.rotation(theta, 1))
end

function arraymath.mapaxis(u, v)
   return arraymath.rotation (math.acos(arraymath.dot (u, v)),
                              arraymath.cross (u, v))
end

function arraymath.fromnode(node, vector)
   return arraymath.matrixmultiply (node.orientation, vector)
end

function arraymath.tonode(node, vector)
   return arraymath.matrixmultiply (array.transpose(node.orientation), vector)
end

function arraymath.concatenate(...)
   local matrices = {...}
   local M

   M = arraymath.matrixmultiply(matrices[1], matrices[2])

   for i = 3, #matrices do
      M = arraymath.matrixmultiply(M, matrices[i])
   end

   return M
end

return arraymath
