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

local bindings = {}
local sequence = {}
local modifiers = {false, false, false, false, false}
local buttons = {}
local axes = {}
local index = bindings
local down, current = 0, nil

local prefixes = {"control-", "alt-", "super-", "hyper-", "meta-"}

local function state (locked)
   local string = ""

   for i = 1, #modifiers do
      if modifiers[i] or (locked and locked[i]) then
	 string = string .. prefixes[i]
      end
   end

   return string
end

local function push (suffix, terminal, ...)
   local binding, catchall

   -- print (table.concat (sequence, " ") .. " " .. suffix)

   binding, catchall = index[suffix], index["*"]
   sequence[#sequence + 1] = suffix

   if binding then
      if type (binding) == "table" then
	 index = binding

	 return false
      else
	 if type (binding) == "function" then
	    binding(table.concat(sequence, ' '), ...)
	 end
      end
   elseif catchall then
      if type (catchall) == "function" then
         catchall(table.concat(sequence, ' '), ...)
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

local function press (self, key, ...)
   local suffix

   if string.find (key, "control") then
      modifiers[1] = true
   elseif string.find (key, "alt") then
      modifiers[2] = true
   elseif string.find (key, "super") then
      modifiers[3] = true
   elseif string.find (key, "hyper") then
      modifiers[4] = true
   elseif string.find (key, "meta") then
      modifiers[5] = true
   elseif string.find (key, "shift") then
      -- Ignore shifts.
   else
      down = down + 1
      current = key
      locked = {table.unpack (modifiers)}

      push (state() .. "down-" .. key, false, ...)
   end

end

function release (self, key, ...)
   local suffix

   if string.find (key, "control") then
      modifiers[1] = false
   elseif string.find (key, "alt") then
      modifiers[2] = false
   elseif string.find (key, "super") then
      modifiers[3] = false
   elseif string.find (key, "hyper") then
      modifiers[4] = false
   elseif string.find (key, "meta") then
      modifiers[5] = false
   elseif string.find (key, "shift") then
      -- Ignore shifts.
   else
      down = down - 1

      push (state() .. "up-" .. key, down == 0 and current ~= key, ...)
      
      if current == key then
         pop(2)
         push(state(locked) .. key, down == 0, ...)
      end

      current = nil
      locked = nil
   end		   
end

local root = primitives.root {
   event = primitives.cursor {
      keypress = press,
      keyrelease = release,

      buttonpress = function (self, button)
         buttons[button] = true

	 press (self, "button-" ..  tostring(button), button)
      end,

      buttonrelease = function (self, button)
         buttons[button] = nil

	 release (self, "button-" ..  tostring(button), button)
      end,

      motion = function (self, x, y)
         local button

         button = next(buttons)

	 if button then
            if x ~= axes[1]then
               push (state() .. "drag-axis-x" ..  tostring(button),
                     true, button, x)
            end

            if y ~= axes[2]then
               push (state() .. "drag-axis-y" ..  tostring(button),
                     true, button, y)
            end
	 else
            if x ~= axes[1]then
               push (state() .. "axis-x", true, x)
            end

            if y ~= axes[2]then
               push (state() .. "axis-y", true, y)
            end
	 end

         axes[1], axes[2] = x, y
      end,

      scroll = function (self, direction)
	 push(state() .. "scroll-" .. direction, true)
      end,
			    }
				 }

bindings[root] = root

return bindings
