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
local buttons= {}
local index = bindings
local down = nil

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

local function terminate ()
   index, sequence = bindings, {}
end

local function fire (suffix, ...)
   local binding

   -- print (table.concat (sequence, " ") .. " " .. suffix)

   binding = index[suffix]

   if binding then
      sequence[#sequence + 1] = suffix

      if type (binding) == "table" then
	 index = binding

	 return false
      else
	 if type (binding) == "function" then
	    binding(sequence, ...)
	 end

	 return true
      end
   end

   return true
end

local root = primitives.root {
   event = primitives.cursor {
      keypress = function (self, key, ...)
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
	    down = key
	    locked = {table.unpack (modifiers)}

	    fire (state() .. "down-" .. key, ...)
	 end

      end,

      keyrelease = function (self, key, ...)
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
	    local p, q

	    p = fire (state() .. "up-" .. key, ...)
	    
	    if down == key then
	       q = fire(state(locked) .. key, ...)
	    end

	    if down ~= key or (p and q) then
	       terminate()
	    end

	    down = nil
	    locked = nil
	 end		   
      end,

      buttonpress = function (self, button)
         buttons[button] = true

	 self.keypress (self, "button-" ..  tostring(button),
			button)
      end,

      motion = function (self, x, y)
         local button

         button = next(buttons)

	 if button then
	    fire (state() .. "drag-button-" ..  tostring(button),
		  button, x, y)
	 else
	    fire (state() .. "motion", x, y)
	 end
      end,

      buttonrelease = function (self, button)
         buttons[button] = nil

	 self.keyrelease (self, "button-" ..  tostring(button),
			  button)
      end,

      scroll = function (self, direction)
	 fire(state() .. "scroll-" .. direction)
      end,
			    }
				 }

bindings[root] = root

return bindings
