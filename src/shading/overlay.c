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

#include <string.h>
#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "algebra.h"
#include "techne.h"
#include "overlay.h"

@implementation Overlay

-(void) init
{
    [super init];

    self->normalized = 0;
}

-(void) draw: (int)frame
{
    double M[16];
    int v[4];

    glGetIntegerv (GL_VIEWPORT, v);
    /* _TRACEV(4, "d", v); */
	
    /* Set an orthographic projection matrix. */

    if (self->normalized) {
        t_load_orthographic(M,
                            -(double)v[2] / v[3] * 0.5,
                            (double)v[2] / v[3] * 0.5,
                            -0.5, 0.5,
                            0, 1);
    } else {
        t_load_orthographic(M, v[0], v[2], v[3], v[1], 0, 1);
    }
        
    /* _TRACEM(4, 4, ".5f", M); */

    t_push_projection(M);

    t_load_identity_4 (M);
    t_push_modelview (M, T_LOAD);
	
    [super draw: frame];
	
    t_pop_modelview();
    t_pop_projection();
}

-(int) _get_normalized
{
    lua_pushboolean(_L, self->normalized);

    return 1;
}

-(void) _set_normalized
{
    self->normalized = lua_toboolean(_L, 3);
}

@end
