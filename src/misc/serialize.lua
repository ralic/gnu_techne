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

local table = require "table"
local string = require "string"

local serialize = {}

local lookup = {
   ["\b"] = "\\b",
   ["\f"] = "\\f",
   ["\n"] = "\\n",
   ["\r"] = "\\r",
   ["\t"] = "\\t"
}

for i = 0, 31 do
   if not lookup[string.char(i)] then
      lookup[string.char(i)] = string.format("\\u00%02x", i)
   end
end

local function escape(s)

   s = string.gsub(s, "\\", "\\\\")
   s = string.gsub(s, "/", "\\/")
   s = string.gsub(s, "%c", lookup)

   return s
end

local function tolua(value)
   local meta, array, hash

   meta = getmetatable(value)   
   array = (type(value) == "table") or (meta and meta.__ipairs ~= nil)
   hash = (type(value) == "table") or (meta and meta.__pairs ~= nil)

   if array or hash then
      local serialized, fields = {}, {}

      if array then
	 for i, element in ipairs (value) do
	    table.insert (fields, string.format ("%s,", tolua(element)))

	    serialized[i] = true
	 end
      end

      if hash then
	 for key, element in pairs (value) do
	    if not serialized[key] then
	       table.insert (fields, string.format ("[%s] = %s,",
						    tolua(key),
						    tolua(element)))
	    end
	 end
      end
      
      return "{" .. table.concat(fields) .. "}"
   elseif type(value) == "string" then
      return string.format ("%q", value)
   elseif type(value) == "number" then
      return string.format ("%.17g", value)
   else
      return tostring(value)
   end
end

local function tojson(value)
   local meta, array, hash

   meta = getmetatable(value)   
   array = (type(value) == "table") or (meta and meta.__ipairs ~= nil)
   hash = (type(value) == "table") or (meta and meta.__pairs ~= nil)

   if array or hash then
      local serialized, fields = {}, {}

      if array and #value > 0 then
         for i, element in ipairs (value) do
            table.insert (fields,
                          string.format ("%s%s",
                                         i == 1 and "" or ",",
                                         tojson(element)))
         end
      
         return "[" .. table.concat(fields) .. "]"
      elseif hash then
         if not serialized[value] then
	    local first = true

            for key, element in pairs (value) do
	       if type(key) == "string" then
		  table.insert (fields, string.format ("%s%s: %s",
						       first and "" or ",",
						       tojson(key),
						       tojson(element) or "null"))
               
		  first = false
	       end
            end

	    serialized[value] = true
         end
      
         return "{" .. table.concat(fields) .. "}"
      end
   elseif type(value) == "string" then
      return "\"" .. escape (value) .. "\""
   elseif type(value) == "number" then
      return string.format ("%g", value)
   else
      return tostring(value)
   end
end

serialize.tolua = tolua
serialize.tojson = tojson

return serialize
