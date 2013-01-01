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

local string = require "string"
local table = require "table"
local primitives = require "primitives"
local controllers = require 'controllers'

local bindings = {}
local sequence = {}
local index = bindings
local down, current = 0, nil

local function push (suffix, terminal, ...)
   local binding, catchall, name

   -- print (table.concat (sequence, " ") .. " " .. suffix)

   sequence[#sequence + 1] = suffix
   name = table.concat (sequence, ' ')
   binding, catchall = index[name], index["*"]

   if binding then
      if type (binding) == "table" then
	 index = binding

	 return false
      else
         binding(name, ...)
      end
   elseif catchall then
      catchall(name, ...)
   end

   if terminal then
      index, sequence = bindings, {}
   end

   return true
end

local function pop (n)
   for i = 1, n do
      sequence[#sequence] = nil
   end
end

local function press (self, prefix, key, ...)
   local suffix

   down = down + 1
   current = key

   push (prefix .. "down-" .. key, false, ...)
end

local function release (self, prefix, key, ...)
   local suffix

   down = down - 1

   push (prefix .. "up-" .. tostring(key), down == 0 and current ~= key, ...)
   
   if current == key then
      pop(2)
      push (prefix .. tostring(key), down == 0, ...)
   end

   current = nil
end

local function motion (self, prefix, button, axis, value, ...)
   if button then
      push (prefix .. "drag-axis-" ..  tostring(axis), true, value, button)
   else
      push (prefix .. "axis-" ..  tostring(axis), true, value)
   end
end

local root = primitives.root {}

for name, device in pairs(controllers) do
   local prefix
   local buttons = {}

   if name == "Core pointer" or name == "Core keyboard" then
      prefix = ""
   else
      prefix = "[" .. name .. "]"
   end

   root[name] = device { 
      buttonpress = function (self, key)
         buttons[key] = true

         if type(key) ~= "string" then
            key = "button-" ..  tostring(key)
         end

         press (self, prefix, key)
      end,

      buttonrelease = function (self, key)
         buttons[key] = nil

         if type(key) ~= "string" then
            key = "button-" ..  tostring(key)
         end

         release (self, prefix, key)
      end,

      motion = function (self, axis, value)
         motion(self, prefix, next(buttons), axis, value)
      end,
     
                       }
end

bindings[root] = root

return bindings
