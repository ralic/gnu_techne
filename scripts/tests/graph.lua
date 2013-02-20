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

primitives = require "primitives"

root = primitives.root {}

function populate (node, j)
   for i = 1, 3 do
      node[i] = primitives.node {}

      if j <= 3 then
         populate(node[i], j + 1)
      end
   end
end

function flatten (node)
   for i, child in children(node) do
      flatten (child)

      child.parent = root
   end
end

function collect (node)
   for i, child in children(node) do
      flatten (child)

      child.parent = nil
   end
end

populate (root, 1)
flatten (root)
assert (#root.descendants == 120)
collect (root)
