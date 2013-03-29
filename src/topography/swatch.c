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

#include <lua.h>
#include <lauxlib.h>

#include "techne.h"
#include "vegetation.h"
#include "swatch.h"

@implementation Swatch

-(void) init
{
    int i;

    [super init];
    
    for (i = 0 ; i < 3 ; i += 1) {
        self->values[i] = 1;
        self->weights[i] = 1;
    }
}

-(int) _get_reference
{
    int j;
    
    lua_createtable (_L, 3, 0);

    for (j = 0; j < 3 ; j += 1) {
        if (self->weights[j] > 0) {
            lua_pushnumber(_L, self->values[j]);
        } else {
            lua_pushnil(_L);
        }
                
        lua_rawseti(_L, -2, j + 1);
    }

    return 1;
}

-(void) _set_reference
{
    int i;

    if (lua_istable (_L, 3)) {
        for (i = 0 ; i < 3 ; i += 1) {
            lua_pushinteger (_L, i + 1);
            lua_gettable (_L, 3);

            if (lua_isnumber(_L, -1)) {
                self->values[i] = lua_tonumber (_L, -1);
                self->weights[i] = 1;
            } else {
                self->values[i] = 0;
                self->weights[i] = 0;
            }			    
			
            lua_pop(_L, 1);
        }

        /* Update the palette's uniform if needed. */
        
        if (self->up) {
            Vegetation *parent = (Vegetation *)self->up;

            assert (parent->name);
            
            glUseProgram (parent->name);
            glUniform3fv(self->locations.references, 1, self->values);
            glUniform3fv(self->locations.weights, 1, self->weights);
        }
    }
}

-(void) meetParent: (Shader *)parent
{
    if (![parent isKindOf: [Vegetation class]]) {
	t_print_warning("%s node has no vegetation parent.\n",
			[self name]);
	
	return;
    }
}

-(void) updateWithIndex: (int)i
{
    unsigned int name;
    
    name = ((Shader *)self->up)->name;
    self->locations.references = glGetUniformLocation (name, "references") + i;
    self->locations.weights = glGetUniformLocation (name, "weights") + i;

    glUniform3fv(self->locations.references, 1, self->values);
    glUniform3fv(self->locations.weights, 1, self->weights);
}

@end
