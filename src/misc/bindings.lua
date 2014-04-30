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

local package = require "package"
local string = require "string"
local table = require "table"
local primitives = require "primitives"
local controllers = require "controllers"

local bindings = {}
local context

local function call(name, ...)
   for key, value in pairs(bindings) do
      local binding = value[name]
      if binding then
         binding(...)
         return true
      end
   end

   return false
end

local function push (suffix, terminal, ...)
   local name

   -- print (table.concat (context.sequence, " ") .. " " .. suffix)

   context.sequence[#context.sequence + 1] = suffix
   name = table.concat (context.sequence, ' ')

   if not call(name, name, ...) then
      call("*", name, ...)
   end

   if terminal then
      context.sequence = {}
      context.depth = 0
   end

   return true
end

local function pop (n)
   for i = 1, n do
      context.sequence[#context.sequence] = nil
   end
end

local function press (self, prefix, key, ...)
   local suffix

   context.depth = context.depth + 1
   context.current = key

   push (prefix .. "down-" .. key, false, ...)
end

local function release (self, prefix, key, ...)
   local suffix

   context.depth = context.depth - 1

   push (prefix .. "up-" .. tostring(key), context.depth <= 0 and context.current ~= key, ...)

   if context.current == key then
      pop(2)
      push (prefix .. tostring(key), context.depth <= 0, ...)
   end

   context.current = nil
end

local function motion (self, prefix, motion, button, axis, value, ...)
   if button then
      push (prefix .. "drag-" .. motion .. "-axis-" ..  tostring(axis), true, value, button)
   else
      push (prefix .. motion .. "-axis-" ..  tostring(axis), true, value)
   end
end

local root = primitives.root {}

for name, device in pairs(controllers) do
   local prefix, state
   local buttons = {}

   if name == "Core pointer" or name == "Core keyboard" then
      prefix = ""
   else
      prefix = "[" .. name .. "]"
   end

   state = {
      sequence = {},
      depth = 0,
      current = nil
   }

   root[name] = device {
      buttonpress = function (self, key)
         context = state
         buttons[key] = true

         if type(key) ~= "string" then
            key = "button-" ..  tostring(key)
         end

         press (self, prefix, key)
      end,

      buttonrelease = function (self, key)
         context = state
         buttons[key] = nil

         if type(key) ~= "string" then
            key = "button-" ..  tostring(key)
         end

         release (self, prefix, key)
      end,

      absolute = function (self, axis, value)
         context = state
         motion(self, prefix, "absolute", next(buttons), axis, value)
      end,

      relative = function (self, axis, value)
         context = state
         motion(self, prefix, "relative", next(buttons), axis, value)
      end,

   }
end

bindings[root] = root

local function createmap(name)
   local t = {}

   bindings[#bindings + 1] = t
   return t
end

package.searchers[#package.searchers + 1] = function (name)

   module, submodule = string.match(name, "(%w+)%.(%w+)")
   if module == "bindings" then
      return createmap, submodule
   end
end
