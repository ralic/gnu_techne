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

local arraymath = require 'arraymath'
local primitives = require 'primitives.core'

function primitives.switch (parameters)
   local node, contacts, setting

   contacts = {}
   node = primitives.node {}

   oldmeta = getmetatable(node)
   replacemetatable(node, {
      __index = function (self, key)
	       if key == "setting" then
		  return setting
	       elseif type(key) == 'number' then
		  return contacts[key]
	       else
		  return oldmeta.__index(self, key)
	       end
	    end,

      __newindex = function (self, key, value)
	       local v
	       
	       if key == "setting" then
		  if setting == value then
		     return
		  end

		  if setting then
		     self.child = nil
		  end

		  setting = value

		  if setting then
		     self.child = contacts[setting]
		  end
	       elseif type(key) == 'number' then
		  contacts[key] = value
	       else
		  oldmeta.__newindex(self, key, value)
	       end
	    end
   })
   
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

function primitives.gimbal (parameters)
   local node

   node = primitives.transform {
      machinery = primitives.transform {
         transform = function (self)
            self.parent.orientation = arraymath.transpose(self.ancestors[2].orientation)
         end,
                                       }
                          }
      
   for key, value in pairs (parameters) do
      node[key] = value
   end

   return node
end

return primitives
