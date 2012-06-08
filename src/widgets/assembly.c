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

#include <math.h>
#include <lua.h>
#include <lauxlib.h>

#include <GL/gl.h>

#include "techne.h"
#include "row.h"
#include "assembly.h"

@implementation Assembly

-(Assembly *) init
{
    self = [super init];

    return self;
}

-(void) measure
{
    Widget *child;
    double w_2, h_2;

    /* Calculate the bounding box of all children. */
    
    for(child = (Widget *)self->down, w_2 = 0, h_2 = 0;
	child;
	child = (Widget *)child->right) {
	double w, h;

	if (![child isKindOf: [Widget class]]) {
	    continue;
	}

	[child measure];

	w = child->content[0] + child->padding[0] + child->padding[1];
	h = child->content[1] + child->padding[2] + child->padding[3];

	/* Width. */
	
	if (fabs(child->offset[0] - 0.5 * w) > w_2) {
	    w_2 = fabs(child->offset[0] - 0.5 * w);
	}

	if (fabs(child->offset[0] + 0.5 * w) > w_2) {
	    w_2 = fabs(child->offset[0] + 0.5 * w);
	}

	child->allocation[0] = w;
	
	/* Height. */
	
	if (fabs(child->offset[1] - 0.5 * h) > h_2) {
	    h_2 = fabs(child->offset[1] - 0.5 * h);
	}

	if (fabs(child->offset[1] + 0.5 * h) > h_2) {
	    h_2 = fabs(child->offset[1] + 0.5 * h);
	}

	child->allocation[1] = h;
    }

    self->content[0] = 2 * w_2;
    self->content[1] = 2 * h_2;
}

-(void) traverse
{
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
	
    [self place];    
    [super traverse];

    glPopMatrix();
}

@end

