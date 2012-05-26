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

local io = require "io"
local resources = {}

local function resolve (name)
   local file

   if options.prefix then
      local list

      if type (options.prefix) == "string" then
	 list = {options.prefix}
      else
	 list = options.prefix
      end

      for _, prefix in ipairs (list) do
	 file = io.open (prefix .. "/" .. name)

	 if file then
	    io.close (file)

	    return prefix .. "/" .. name
	 end
      end
   end

   return name
end

function resources.readfile (name)
   print ("  " .. name)

   return io.open (resolve(name)):read "*a"
end

function resources.loadfile (name)
   print ("  " .. name)

   return loadfile(resolve(name))
end

function resources.dofile (name, ...)
   print ("  " .. name)

   local args = ... and unpack(...) or nil
   return assert(loadfile(resolve(name)))(args)
end

return resources