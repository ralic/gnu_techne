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

if not options.profile then
   return
end

local table = require "table"
local string = require "string"
local primitives = require "primitives"
local techne = require "techne"

local function accumulate(node)
   local cummulative = node.profile

   for _, child in pairs(node.children) do
      local subtree = accumulate(child)
      
      cummulative[1] = cummulative[1] + subtree[1]
      cummulative[2] = cummulative[2] + subtree[2]
   end

   return cummulative
end

local function compare(a, b)
   return a.profile[1] + a.profile[2] > b.profile[1] + b.profile[2]
end

local function dump(node, nodes)
   nodes[#nodes + 1] = node

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
      local c, nodes, output

      if techne.iterations == 0 then
         return
      end

      c = 1e3 / techne.iterations

      message(string.format([[
Ran a total of %d iterations in %.1f seconds at %.1f ms per iteration (%.1f Hz).
]],
                            techne.iterations, techne.time,
                            techne.time * c, techne.iterations / techne.time))
      
      -- Collect to-be-profiled nodes.
      
      nodes = {}
      
      if type(options.profile) ~= "boolean" then
         local specifications

         -- Wrap a single node in a table.
         
         if type(options.profile) == "table" then
            specifications = options.profile
         else
            specifications = {options.profile}
         end

         -- Evaluate each node specification and collect the result
         -- into the table.
         
         for _, specification in pairs(specifications) do
            local node, chunk

            chunk = loadstring("return " .. specification)

            if not chunk then
               warn(string.format("Could not resolve node at '%s'\n",
                                  specification))
            else
               node = chunk()

               if type(node) == "userdata" then
                  nodes[#nodes + 1] = node
               else
                  warn(string.format(
                          "Value '%s' is not a node (it is a %s value).\n",
                          specification, type(node)))
               end
            end
         end
      else
         local all, total = {}, 0

         -- No nodes specified explicitly; collect all roots and
         -- built-ins.
         
         for _, root in siblings(self) do
            dump(root, all)
         end
         
         for _, builtin in siblings(techne) do
            dump(builtin, all)
         end

         dump(techne, all)

         table.sort(all, compare)

         -- Just include the nodes that take up 95% of execution time.
         
         for _, node in ipairs(all) do
            total = total + node.profile[1] + node.profile[2]
            nodes[#nodes + 1] = node

            if total > 0.95 * techne.time then
               break
            end
         end
      end

      -- Arrange node profiling information in a neat table.
      
      if #nodes > 0 then
         local reverse, total
         
         message ([[
+------------------------------------------------------+------------------------+
|                                 cummulative (ms)     |   self (ms)            |
|node                             core    user   total |   core    user   total |
+------------------------------------------------------+------------------------+
]])

         total = {{0, 0}, {0, 0}}
         reverse = {}

         for _, node in ipairs(nodes) do
            reverse[node] = true
         end
         
         for i, node in ipairs(nodes) do
            local skip, profile, label

            profile = {node.profile, accumulate(node)}
            
            label = tostring(node)
            if #label > 30 then
               label = string.sub(label, 1, 27) .. "..."
            end
            
            message (string.format(
                        [[
|%-30s %6.1f  %6.1f  %6.1f | %6.1f  %6.1f  %6.1f |
]],
                        label,
                        (profile[2][1] - profile[2][2]) * c,
                        profile[2][2] * c, profile[2][1] * c,
                        (profile[1][1] - profile[1][2]) * c,
                        profile[1][2] * c, profile[1][1] * c))

            skip = false
            
            for _, ancestor in pairs(node.ancestors) do
               if reverse[ancestor] then
                  skip = true
                  break
               end
            end

            if not skip then
               total[2][1] = total[2][1] + profile[2][1]
               total[2][2] = total[2][2] + profile[2][2]
            end
            
            total[1][1] = total[1][1] + profile[1][1]
            total[1][2] = total[1][2] + profile[1][2]
         end

         message (string.format(
                     [[
+------------------------------------------------------+------------------------+
|total                          %6.1f  %6.1f  %6.1f | %6.1f  %6.1f  %6.1f |
+------------------------------------------------------+------------------------+
]],
                     (total[2][1] - total[2][2]) * c,
                     total[2][2] * c, total[2][1] * c,
                     (total[1][1] - total[1][2]) * c,
                     total[1][2] * c, total[1][1] * c))
      end
   end,
}
