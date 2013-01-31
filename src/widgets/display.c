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

#include "gl.h"

#include "algebra.h"
#include "techne.h"
#include "display.h"

@implementation Display

-(void) arrange
{
    Widget *child;
    int v[4];
	
    glGetIntegerv (GL_VIEWPORT, v);

    self->allocation[0] = (double)v[2] / v[3];
    self->allocation[1] = 1;
    
    /* The display widget should expand to fill up all space
     * available. */
    
    self->content[0] = self->allocation[0] -
	self->padding[0] - self->padding[1];
    self->content[1] = self->allocation[1] -
	self->padding[2] - self->padding[3];

    for (child = (Widget *)self->down;
	 child;
	 child = (Widget *)child->right) {
	if ([child isKindOf:[Widget class]]) {
	    [child measure];
	
	    child->allocation[0] = self->content[0];
	    child->allocation[1] = self->content[1];

	    child->offset[0] = 0;
	    child->offset[1] = 0;
	}
    }

    [super arrange];
}

-(void) draw: (int)frame
{
    int v[4];
    float M[16];

    [self arrange];
    
    glGetIntegerv (GL_VIEWPORT, v);
    t_load_orthographic(M,
			-(double)v[2] / v[3] * 0.5,
			(double)v[2] / v[3] * 0.5,
			-0.5, 0.5,
			0, 1);

    t_push_projection(M);

    [super draw: frame];
    
    t_pop_projection();
}

@end
