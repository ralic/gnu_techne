-- Copyright (C) 2012 Papavasileiou Dimitris                             
--
-- Permission is hereby granted, free of charge, to any person
-- obtaining a copy of this software and associated documentation
-- files (the "Software"), to deal in the Software without
-- restriction, including without limitation the rights to use, copy,
-- modify, merge, publish, distribute, sublicense, and/or sell copies
-- of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
-- NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
-- BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
-- ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
-- CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
-- SOFTWARE.

local array = require 'array.core'

local function pretty(a, prefix)
   if type(a) == "number" then
      return tostring(a)
   else
      local s, m, last, number

      m = #a
      prefix = prefix or ""
      last = a[m]
      number = type(last) == "number"

      if m > 1 then
	 s = prefix .. "{"

	 if not number then
	    s = s .. "\n"
	 end
	 
	 for i = 1, m - 1 do
	    s = s .. pretty(a[i], prefix .. "  ") .. ", "

	    if not number then
	       s = s .. "\n"
	    end
	 end

	 s = s .. pretty(last, prefix .. "  ")

	 if not number then
	    s = s .. "\n" .. prefix
	 end
      end

      return s .. "}"
   end
end

local function size(a)
   return type(a) == "number" and {} or {#a, unpack(size(a[1]))}
end

array.pretty = pretty
array.size = size

local meta = getmetatable(array.doubles{})
local oldtostring = meta.__tostring
meta.__tostring = function(self)
                     return oldtostring(self) .. "\n" .. pretty(self)
                  end
return array