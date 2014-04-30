-- Copyright (C) 2013 Papavasileiou Dimitris
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

local io = require 'io'
local table = require "table"
local string = require "string"
local primitives = require "primitives"
local techne = require "techne"

if not techne.profiling then
   message("Gathering of profiling data is not enabled.\n")
   return
end

if options.periodicprofile then
   local expressions
   local last = {}

   if type(options.profile) == "string" then
      expressions = {loadstring("return " .. options.profile)}
   elseif type(options.profile) == "table" then
      expressions = {}
      for i, expression in ipairs(options.profile) do
         expressions[i] = assert(loadstring("return " .. expression))
      end
   end

   if expressions then
      local printrow, file

      if type(options.periodicprofile) == "string" then
         file = io.open(options.periodicprofile, "w")

         printrow = function (row)
            file:write(row)
         end
      else
         printrow = message
      end

      profiler = primitives.root {
         tag = "profiler",
         index = -1 / 0,

         timer = primitives.timer {
            source = "iterations",
            period = options.profilingperiod or 30,

            tick = function (self, tick, delta, elapsed)
               local n, row = #expressions, ""

               for i = 1, n  do
                  local x
                  local this, that = expressions[i](), last[i]

                  if that then
                     x = (this[2] * this[1] - that[2] * that[1]) /
                        (this[2] - that[2])
                  else
                     x = this and this[1] or 0
                  end

                  last[i] = this

                  row = row .. string.format("%f", x) ..
                               (i < n and ", " or "\n")
               end

               printrow (row)
            end,

            unlink = function (self)
               if file then
                  file:close()
               end
            end
         }
      }
   end
else
   local function ownprofile(node)
      own = {node.core, node.user, node.graphics}
      for _, child in pairs(node.children) do
         own[1][1] = own[1][1] - child.core[1]
         own[2][1] = own[2][1] - child.user[1]
         if own[3] and child.graphics ~= nil then
            own[3][1] = own[3][1] - child.graphics[1]
         end
      end

      return own
   end

   -- Sort based on total own per-frame time.

   local function comparenodes(a, b)
      return (a[3][1][1] + (a[3][3] and a[3][3][1] or 0) >
                 b[3][1][1] + (b[3][3] and b[3][3][1] or 0))
   end

   local function dump(node, nodes)
      nodes[#nodes + 1] = {
         tostring(node),
         {node.core, node.user, node.graphics},
         ownprofile(node)
      }

      for _, child in pairs(node.children) do
         dump(child, nodes)
      end
   end

   -- Add a root with an index of negative infinity.  This ensures that
   -- this node will be unlinked first, before any other roots have been
   -- unlinked and it also gives us access to all roots via the siblings
   -- table.

   profiler = primitives.root {
      tag = "node-profiler",
      index = -1 / 0,

      unlink = function(self)
         local nodes, output

         if techne.iterations == 0 then
            return
         end

         -- Collect to-be-profiled nodes.

         if options.profile then
            local specifications

            -- Wrap a single node in a table.

            if type(options.profile) == "table" then
               specifications = options.profile
            else
               specifications = {options.profile}
            end

            -- Evaluate each node specification and collect the result
            -- into the table.

            nodes = {}
            for _, specification in pairs(specifications) do
               local node, chunk

               if type(specification) == 'string' then
                  chunk = loadstring("return " .. specification)

                  if not chunk then
                     warn(string.format("Could not resolve node at '%s'\n",
                                        specification))
                  else
                     node = chunk()

                     if type(node) == "userdata" then
                        nodes[#nodes + 1] = {
                           tostring(node),
                           {node.core, node.user, node.graphics},
                           ownprofile(node)
                        }
                     else
                        warn(string.format(
                                "Value '%s' is not a node (it is a %s value).\n",
                                specification, type(node)))
                     end
                  end
               end
            end
         else
            local total

            -- No nodes specified explicitly; collect all roots and
            -- built-ins.

            nodes = {}
            for _, root in pairs(self.siblings) do
               dump(root, nodes)
            end

            dump(techne, nodes)

            table.sort(nodes, comparenodes)

            -- Just include the nodes that take up 95% of execution time.

            total = 0
            for i = 1, #nodes do
               if total < 0.95 * techne.time then
                  total = total + nodes[i][3][1][1]
               else
                  nodes[i] = nil
               end
            end
         end

         -- Arrange node profiling information in a neat table.

         if #nodes > 0 then
            local total

            message ([[
+---------------------------------------------------------------------------------+
|                                 cummulative ms(samples x 100)/self ms           |
|node                             total            user             graphics      |
+---------------------------------------------------------------------------------+
]])

            for i, node in ipairs(nodes) do
               local skip, cummulative, own, label

               cummulative = node[2]
               own = node[3]

               label = tostring(node[1])
               if #label > 30 then
                  label = string.sub(label, 1, 27) .. "..."
               end

               message (string.format(
                           [[
|%-30s %5.1f(%2d)/%5.1f  %5.1f(%2d)/%5.1f  %5.1f(%2d)/%5.1f |
]],
                           label,
                           cummulative[1][1] * 1e3, cummulative[1][2] / 100,
                           own[1][1] * 1e3,
                           cummulative[2][1] * 1e3, cummulative[2][2] / 100,
                           own[2][1] * 1e3,
                           (cummulative[3] and cummulative[3][1] or 0) * 1e3,
                           (cummulative[3] and cummulative[3][2] / 100 or 0),
                           (own[3] and own[3][1] or 0) * 1e3))
            end

            message ([[
+---------------------------------------------------------------------------------+
]])
         end
      end,
   }
end
