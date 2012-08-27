/* Copyright (C) 2012 Papavasileiou Dimitris                             
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

#include <stdio.h>
#include <stdlib.h>

#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"

int main(int argc, char **argv)
{
    Techne *techne;
    
    /* Create and initialize the Lua state. */
    
    _L = luaL_newstate();

    luaL_requiref(_L, "base", luaopen_base, 1);
    luaL_requiref(_L, "coroutine", luaopen_coroutine, 0);
    luaL_requiref(_L, "string", luaopen_string, 0);
    luaL_requiref(_L, "table", luaopen_table, 0);
    luaL_requiref(_L, "math", luaopen_moremath, 0);
    luaL_requiref(_L, "bit32", luaopen_bit32, 0);
    luaL_requiref(_L, "io", luaopen_io, 0);
    luaL_requiref(_L, "os", luaopen_os, 0);
    luaL_requiref(_L, "debug", luaopen_debug, 0);
    
    lua_settop (_L, 0);

    techne = [[Techne alloc] initWithArgc: argc andArgv: argv];

    t_print_message ("Entering the main loop.\n");

    [techne iterate];
    
    t_print_message("Bye!\n");
    exit (EXIT_SUCCESS);
}
