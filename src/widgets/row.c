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

#include <GL/gl.h>

#include "techne.h"
#include "row.h"

@implementation Row

-(double) measureWidth
{
    Widget *child;

    self->minimum[0] = 0;
    
    for(child = (Widget *)self->down;
	child;
	child = (Widget *)child->right) {
	double w;

	w = [child measureWidth];

	child->allocated[0] = w;
	self->minimum[0] += w;
    }

    return self->minimum[0] + self->padding[0] + self->padding[1];
}

-(double) measureHeight
{
    Widget *child;

    self->minimum[1] = 0;
    
    for(child = (Widget *)self->down;
	child;
	child = (Widget *)child->right) {
	double h;

	h = [child measureHeight];	
	self->minimum[1] = h > self->minimum[1] ? h : self->minimum[1];
    }

    for(child = (Widget *)self->down;
	child;
	child = (Widget *)child->right) {
	child->allocated[1] = self->minimum[1];
    }

    return self->minimum[1] + self->padding[2] + self->padding[3];
}

-(void) transform
{
    Widget *child;
    double delta;
	
    /* Now distribute the children in the allocated space. */

    for(child = (Widget *)self->down, delta = -0.5 * self->minimum[0];
	child;
	child = (Widget *)child->right) {
	delta += 0.5 * child->allocated[0];

	child->offset[0] = delta;
	child->offset[1] = 0;

	delta += 0.5 * child->allocated[0];
    }
    
    [super transform];
}

-(void) traverse
{
    [self place];    
    [super traverse];
}

@end

