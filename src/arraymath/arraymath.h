/* Copyright (C) 2012 Papavasileiou Dimitris
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _ARRAYMATH_H_
#define _ARRAYMATH_H_

int arraymath_add(lua_State *L);
int arraymath_multiply(lua_State *L);
int arraymath_subtract(lua_State *L);
int arraymath_divide(lua_State *L);
int arraymath_scale(lua_State *L);
int arraymath_offset(lua_State *L);
int arraymath_raise(lua_State *L);
int arraymath_range(lua_State *L);
void arraymath_combine(lua_State *L);
double arraymath_dot(lua_State *L);
void arraymath_cross(lua_State *L);
double arraymath_length(lua_State *L);
double arraymath_distance(lua_State *L);
array_Array *arraymath_normalize(lua_State *L);
array_Array *arraymath_transpose(lua_State *L);
array_Array *arraymath_matrix_multiply(lua_State *L);
array_Array *arraymath_matrix_multiplyadd(lua_State *L);
array_Array *arraymath_apply (lua_State *L, int i);

#endif
