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
local transform = require 'transform.core'

function transform.tensor(u, v)
   return transform.multiply(array.cast(#u, 1, u), array.cast (1, #v, v))
end

function transform.euler(theta, phi, psi)
   return transform.concatenate(transform.rotation(theta, 1),
				transform.rotation(phi, 2),
				transform.rotation(psi, 3))
end

function transform.relue(theta, phi, psi)
   return transform.concatenate(transform.rotation(theta, 3),
				transform.rotation(phi, 2),
				transform.rotation(psi, 1))
end

function transform.fromnode(node, vector)
   return transform.apply (node.orientation, vector)
end

function transform.tonode(node, vector)
   return transform.apply (transform.transpose(node.orientation), vector)
end

return transform