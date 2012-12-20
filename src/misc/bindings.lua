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
	 if type (binding) == "function" then
	    binding(name, ...)
	 end
      end
   elseif catchall then
      if type (catchall) == "function" then
         catchall(name, ...)
      end
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

local function motion (self, prefix, axis, value, ...)
   local button

   button = next(self.buttons)

   if button then
      push (prefix .. "drag-axis-" ..  tostring(axis), true, value, button)
   else
      push (prefix .. "axis-" ..  tostring(axis), true, value)
   end
end

local root = primitives.root {
   event = primitives.cursor {
      buttons = {},
      axes = {},

      keypress = function (self, key)
         press (self, "", key)
      end,

      keyrelease = function (self, key)
         release (self, "", key)
      end,

      buttonpress = function (self, button)
         self.buttons[button] = true

	 press (self, "", "button-" ..  tostring(button), button)
      end,

      buttonrelease = function (self, button)
         self.buttons[button] = nil

	 release (self, "", "button-" ..  tostring(button), button)
      end,

      motion = function (self, x, y)
         if x ~= self.axes[1] then
            motion(self, "", 1, x)
         end

         if y ~= self.axes[2] then
            motion(self, "", 2, y)
         end

         self.axes[1], self.axes[2] = x, y
      end,

      scroll = function (self, direction)
	 push("scroll-" .. direction, true)
      end,
			    }
				 }

for name, device in pairs(controllers) do
   root[name] = device {
      buttons = {},
 
      buttonpress = function (self, key)
         press (self, '[' .. name .. ']', "button-" ..  tostring(key))
      end,

      buttonrelease = function (self, key)
         release (self, '[' .. name .. ']', "button-" ..  tostring(key))
      end,

      motion = function (self, axis, value)
         motion(self, '[' .. name .. ']', axis, value)
      end,
     
                       }
end

bindings[root] = root

return bindings
