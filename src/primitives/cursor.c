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

#include <gdk/gdk.h>

#include "gl.h"

#include "techne.h"
#include "cursor.h"

static int uptodate;

@implementation Cursor

-(void) inputWithEvent: (GdkEvent *) event
{
    assert (event);

    if (event->type == GDK_MOTION_NOTIFY) {
	self->position[0] = (int)(((GdkEventMotion *)event)->x);
	self->position[1] = (int)(((GdkEventMotion *)event)->y);

	uptodate = 1;
    }
}

-(void) toggle
{
    [super toggle];

    if (self->linked) {
	uptodate = 0;
    }
}

-(void) transform
{
    double zero[3] = {0, 0, 0};

    [super transformRelativeTo: zero];

    /* _TRACEM(4, 4, ".5f", self->matrix); */
    /* _TRACE ("%f, %f\n", self->position[0], self->position[1]); */
}

-(void) traverse
{
    if (uptodate) {
	float M[16];
	int v[4];

	glGetIntegerv (GL_VIEWPORT, v);
	/* _TRACEV(4, "d", v); */
	
	/* We're only interested in color values.
	   Updating the stencil would screw up
	   the picking mechanism. */
	
	glDepthMask (GL_FALSE);
	glStencilMask (0);

	/* Set an orthographic projection matrix. */
	
	memset (M, 0, sizeof (float[16]));

	M[0] = 2 / (float)v[2];
	M[3] = -(1 + 2 * (float)v[0] / v[2]);
	M[5] = -2 / (float)v[3];
	M[7] = 1 + 2 * (float)v[1] / v[3];
	M[10] = -1;
	M[11] = 0;
	M[15] = 1;

	/* _TRACEM(4, 4, ".5f", M); */

	t_push_projection(M);
	
	[super traverse];
	
	t_pop_projection(M);

	glDepthMask (GL_TRUE);
	glStencilMask (~0);	
    }
}

@end
