/* Copyright (C) 2009 Papavasileiou Dimitris
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _METEOROLOGY_H_
#define _METEOROLOGY_H_

#include "builtin.h"

@interface Meteorology: Builtin {
}

-(int) _get_temperature;
-(int) _get_pressure;
-(int) _get_density;
-(void) _set_temperature;
-(void) _set_pressure;
-(void) _set_density;

@end

int luaopen_meteorology (lua_State *L);

#endif
