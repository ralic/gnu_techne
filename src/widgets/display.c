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
#include "display.h"

@implementation Display

-(void) transform
{
    Widget *child;
    
    double zero[3] = {0, 0, 0};
    int v[4];
	
    glGetIntegerv (GL_VIEWPORT, v);

    self->allocated[0] = (double)v[2] / v[3];
    self->allocated[1] = 1;
    
    /* Calculate our own allocation based on our children. */
    
    self->minimum[0] = self->allocated[0] -
	self->padding[0] - self->padding[1];
    self->minimum[1] = self->allocated[1] -
	self->padding[2] - self->padding[3];

    for (child = (Widget *)self->down;
	 child && ![child isKindOf:[Widget class]];
	 child = (Widget *)child->right);
    
    if (child) {
	[child measureWidth];
	[child measureHeight];
	
	child->allocated[0] = self->minimum[0];
	child->allocated[1] = self->minimum[1];
    }

    [super transformRelativeTo: zero];
}

-(void) traverse
{
    int v[4];
	
    glMatrixMode (GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glGetIntegerv (GL_VIEWPORT, v);
    glOrtho(-(double)v[2] / v[3] * 0.5,
	    (double)v[2] / v[3] * 0.5,
	    -0.5, 0.5,
	    0, 1);

    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
	
    glUseProgramObjectARB(0);

    [self place];
    [super traverse];
    
    glMatrixMode (GL_MODELVIEW);
    glPopMatrix();
    
    glMatrixMode (GL_PROJECTION);
    glPopMatrix();
}

@end
