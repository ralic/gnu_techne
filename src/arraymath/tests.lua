local array = require "array"
local arraymath = require "arraymath"
local table = require "table"
local string = require "string"
local math = require "math"

local function test(A, R, p)
   if type(A) == "number" then
      local delta = math.abs(A - R)

      if delta > p then
         return delta
      end
   else
      for i = 1, #A do
         local delta = test(A[i], R[i], p)

         if delta > 0 then
            return delta
         end
      end
   end

   return 0
end

local function testtypes(T, P, R, f, ...)
   for j = 1, #T do
      local args = {}

      for i, A in ipairs{...} do
         args[i] = type(A) == 'number' and A or array[T[j]](A)
      end

      local B = arraymath[f](table.unpack(args))
      local delta = test(B, R, P[j])

      if delta > 0 then
         local m = string.format("%s, %s failed (delta = %g).\n\nArguments:\n",
                                 f, T[j], delta)

         for _, A in ipairs(args) do
            m = m .. string.format("%s\n", array.pretty(A))
         end

         m = m .. string.format("\nResult:\n%s\n", array.pretty(B))

         m = m .. string.format("\nExpected Result:\n%s\n", array.pretty(R))

         error(m)
      end
   end
end

local function testreal(R, f, ...)
   testtypes({"doubles", "floats", "nints", "nuints",
              "nshorts", "nushorts", "nchars", "nuchars"},
             {2^-53, 2^-24,  2^-28, 2^-29,
              2^-12, 2^-13,  2^-4, 2^-5},
             R, f, ...)
end

local function testfloat(R, f, ...)
   testtypes({"doubles", "floats"},
             {2^-49, 2^-23}, R, f, ...)
end

local function testinteger(R, f, ...)
   testtypes({"ints", "uints", "shorts", "ushorts", "chars", "uchars"},
             {0, 0,  0, 0,  0, 0},
             R, f, ...)
end

local function testcustom(T, D, R, f, ...)
   testtypes(
             R, f, ...)
end

A = {{16, 64, 32},
     {32, 16, 64},
     {32, 32, 64}}

B = {{2, 1, 8},
     {2, 4, 4},
     {8, 4, 1}}

C = {{0.25,  0.5,  0.5},
     {   1, 0.25,  0.5},
     { 0.5,    1, 0.25}}

D = {{ 0.03125, 0.03125,    0.125},
     {       0,  0.0625,   0.0625},
     {   0.125,       0, 0.015625}}

-- arraymath.add

testreal({{0.53125, 0.53125, 0.625},
          {0.5,     0.5625,  0.5625},
          {0.625,   0.5,     0.515625}}, "add", 0.5, D)

testreal({{0.53125, 0.53125, 0.625},
          {0.5,     0.5625,  0.5625},
          {0.625,   0.5,     0.515625}}, "add", D, 0.5)

testreal({{0.28125, 0.53125,    0.625},
          {      1,  0.3125,   0.5625},
          {  0.625,       1, 0.265625}}, "add", C, D)

testinteger({{18, 65, 40},
             {34, 20, 68},
             {40, 36, 65}}, "add", A, B)

testinteger({{0, 48, 16},
             {16, 0, 48},
             {16, 16, 48}}, "add", A, -16)

testinteger({{0, 48, 16},
             {16, 0, 48},
             {16, 16, 48}}, "add", -16, A)

-- arraymath.subtract

testreal({{0.21875, 0.46875, 0.375},
          {1,       0.1875,  0.4375},
          {0.375,   1,       0.234375}}, "subtract", C, D)

testreal({{0,    0.25, 0.25},
          {0.75, 0,    0.25},
          {0.25, 0.75,    0}}, "subtract", C, 0.25)

testreal({{0.75, 0.5,  0.5},
          {0,    0.75, 0.5},
          {0.5,  0,    0.75}}, "subtract", 1, C)

testinteger({{14, 63, 24},
             {30, 12, 60},
             {24, 28, 63}}, "subtract", A, B)

testinteger({{0, 48, 16},
             {16, 0, 48},
             {16, 16, 48}}, "subtract", A, 16)

testinteger({{48, 0, 32},
             {32, 48, 0},
             {32, 32, 0}}, "subtract", 64, A)

-- arraymath.multiply

testreal({{0.125, 0.25,  0.25},
          {0.5,   0.125, 0.25},
          {0.25,  0.5,   0.125}}, "multiply", C, 0.5)

testreal({{0.125, 0.25,  0.25},
          {0.5,   0.125, 0.25},
          {0.25,  0.5,   0.125}}, "multiply", 0.5, C)

testreal({{0.0078125, 0.015625, 0.0625},
          {0,         0.015625, 0.03125},
          {0.0625,    0,        0.00390625}}, "multiply", C, D)

testinteger({{ 4,  1, 64},
             { 4, 16, 16},
             {64, 16,  1}}, "multiply", B, B)

testinteger({{16, 8, 64},
             {16, 32, 32},
             {64, 32, 8}}, "multiply", B, 8)

testinteger({{16, 8, 64},
             {16, 32, 32},
             {64, 32, 8}}, "multiply", 8, B)

-- arraymath.divide

testreal({{0.25, 0.25,     1},
          {0,    0.5,    0.5},
          {1,    0,    0.125}}, "divide", D, 0.125)

trace(arraymath.divide(-0.25, array.nints(C)))

testreal({{1,    0.5,  0.5},
          {0.25, 1,    0.5},
          {0.5,  0.25,   1}}, "divide", 0.25, C)

testreal({{0.125, 0.0625, 0.25},
          {0,     0.25,   0.125},
          {0.25,  0,      0.0625}}, "divide", D, C)

testinteger({{8,  64, 4},
             {16, 4,  16},
             {4,  8,  64}}, "divide", A, B)

-- Comparisons

testreal({{1, 1, 1},
          {1, 1, 1},
          {1, 1, 1}}, "greater", C, D)

testreal({{0, 1, 1},
          {1, 0, 1},
          {1, 1, 0}}, "greater", C, 0.25)

testreal({{1, 1, 1},
          {0, 1, 1},
          {1, 0, 1}}, "logicaland", C, D)

testreal({{1, 1, 1},
          {0, 1, 1},
          {1, 0, 1}}, "logicaland", D, 1)

testreal({{0, 0, 0},
          {1, 0, 0},
          {0, 1, 0}}, "logicalnot", D)

testinteger({{1, 1, 1},
             {1, 1, 1},
             {1, 1, 1}}, "greater", A, B)

testinteger({{0, 1, 1},
             {1, 0, 1},
             {1, 1, 1}}, "greater", A, 16)

testinteger({{1, 1, 1},
             {1, 1, 1},
             {1, 1, 1}}, "logicaland", A, B)

testinteger({{1, 1, 1},
             {1, 1, 1},
             {1, 1, 1}}, "logicaland", B, 1)

testinteger({{0, 0, 0},
             {0, 0, 0},
             {0, 0, 0}}, "logicalnot", B)

-- arraymath.clamp

testreal({{0.3, 0.5, 0.5},
          {0.5, 0.3, 0.5},
          {0.5, 0.5, 0.3}}, "clamp", C, 0.3, 0.5)

testinteger({{20, 30, 30},
             {30, 20, 30},
             {30, 30, 30}}, "clamp", A, 20, 30)

-- arraymath.range

testreal({{0.03125, 0.125},
          {0, 0.0625},
          {0, 0.125}}, "range", D)

testinteger({{1, 8},
             {2, 4},
             {1, 8}}, "range", B)

-- arraymath.sum

testreal({0.1875, 0.125, 0.140625}, "sum", D)
testreal(0.1875, "sum", D[1])
testinteger({11, 10, 13}, "sum", B)
testinteger(11, "sum", B[1])

-- arraymath.product

testreal({0.0001220703125, 0, 0}, "product", D)
testreal(0.0001220703125, "product", D[1])
testinteger({16, 32, 32}, "product", B)
testinteger(16, "product", B[1])

-- arraymath.power

testfloat({{math.sqrt(2) / 2, math.sqrt(2) / 2, math.sqrt(2) / 2},
           {               1, math.sqrt(2) / 2, math.sqrt(2) / 2},
           {math.sqrt(2) / 2,                1, math.sqrt(2) / 2}},
           "power", C, C)

testfloat({{0.0625,   0.25,   0.25},
           {     1, 0.0625,   0.25},
           {  0.25,      1, 0.0625}}, "power", C, 2)

testfloat({{math.sqrt(2) / 2, 0.5, 0.5},
           {0.25, math.sqrt(2) / 2, 0.5},
           {0.5, 0.25, math.sqrt(2) / 2}}, "power", 0.25, C)

E = {{math.pi / 3, 0},
     {math.pi / 6, math.pi / 4}}

F = {{math.exp(1), math.exp(2)},
     {math.exp(3), math.exp(4)}}

-- Unary functions

testfloat({{3, 8}, {21, 55}}, "ceiling", F)
testfloat({{2, 7}, {20, 54}}, "floor", F)

testfloat({{math.sqrt(3) / 2, 0},
           {0.5, math.sqrt(2) / 2}}, "sine", E)

testfloat({{0.5, 1}, {math.sqrt(3) / 2, math.sqrt(2) / 2}}, "cosine", E)

testfloat({{math.sqrt(3), 0}, {1 / math.sqrt(3), 1}}, "tangent", E)

testfloat({{1, 2}, {3, 4}}, "logarithm", F)

G = {{-0.5, 0.5, -0.5},
     { 0.5, 0.5, -0.5},
     {-0.5, 0.5,  0.5}}

H = {{-5, 5, -5},
     { 5, 5, -5},
     {-5, 5,  5}}

-- arraymath.absolute

testtypes({"doubles", "floats", "nints",
           "nshorts", "nchars"},
          {2^-51, 2^-23,  2^-28, 2^-12, 2^-4},
          {{0.5, 0.5, 0.5},
           {0.5, 0.5, 0.5},
           {0.5, 0.5, 0.5}}, "absolute", G)

testtypes({"ints", "shorts", "chars"},
          {0, 0, 0},
          {{5, 5, 5},
           {5, 5, 5},
           {5, 5, 5}}, "absolute", H)

U = {1, 4, 8}
V = {16, 4, 8}

-- Linear algebra.

testfloat(96, "dot", U, V)
testfloat({0, 120, -60}, "cross", U, V)
testfloat({1 / 9, 4 / 9, 8 / 9}, "normalize", U)
testfloat(9, "length", U)
testfloat(15, "distance", U, V)

-- Matrices

testfloat({{416, 400, 416},
           {608, 352, 384},
           {640, 416, 448}}, "matrixmultiply", A, B)

testfloat({528, 608, 672}, "matrixmultiply", A, U)
testfloat({400, 384, 800}, "matrixmultiply", U, A)

-- Transforms

testfloat({3 / 2 * math.sqrt(2), -3 / 2 * math.sqrt(2), 0},
          "matrixmultiply", arraymath.rotation(math.pi / 4, 3), {3, 0, 0})

testfloat({3 / 2 * math.sqrt(2), -3 / 2 * math.sqrt(2), 0},
          "matrixmultiply",
          arraymath.rotation(math.pi / 4, {0, 0, 1}), {3, 0, 0})

testfloat({3 / 2 * math.sqrt(2), -3 / 2 * math.sqrt(2), 0},
          "matrixmultiply",
          arraymath.rotation({0, 0, 0.382683432365090, 0.923879532511287}),
          {3, 0, 0})
