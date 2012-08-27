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

static int pointer[2], uptodate;

@implementation Cursor

-(void) inputWithEvent: (GdkEvent *) event
{
    assert (event);

    if (event->type == GDK_MOTION_NOTIFY) {
	pointer[0] = (int)(((GdkEventMotion *)event)->x);
	pointer[1] = (int)(((GdkEventMotion *)event)->y);

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
}

-(void) traverse
{
    if (uptodate) {
	int v[4];

	glGetIntegerv (GL_VIEWPORT, v);
	/* _TRACEV(4, "d", v); */
	
	/* We're only interested in color values.
	   Updating the stencil would screw up
	   the picking mechanism. */
	
	glDepthMask (GL_FALSE);
	glStencilMask (0);

	{
	    static float M[16];

	    /* glMatrixMode (GL_PROJECTION); */
	    /* glLoadIdentity(); */
	    /* glOrtho(0, 1024, 0, 640, -1, 1); */
	    /* glGetFloatv(GL_PROJECTION_MATRIX, M); */

	    /* _TRACEM(4, 4, ".5f", M); */
	    
	    memset (M, 0, sizeof (float[16]));

	    M[0] = 2 / (float)v[2];
	    M[3] = (float)(v[2] + 2 * v[0]) / v[2];
	    M[5] = 2 / (float)v[3];
	    M[7] = (float)(v[3] + 2 * v[1]) / v[3];
	    M[10] = -1;
	    M[11] = 0;
	    M[15] = 1;

	    /* _TRACEM(4, 4, ".5f", M); */

	    t_set_projection(M);
	}

	
	/* { */
	/*     static float M[16]; */

	/*     memset (M, 0, sizeof (float[16])); */

	/*     M[3] = pointer[0]; */
	/*     M[7] = pointer[1]; */

	/*     t_set_modelview(M); */
	/* } */
	
	[super traverse];
    
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix();
	
	glDepthMask (GL_TRUE);
	glStencilMask (~0);	
    }
}

@end
