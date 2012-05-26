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

#include <gdk/gdk.h>
#include <GL/gl.h>

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

	/* We're only interested in color values.
	   Updating the stencil would screw up
	   the picking mechanism. */
	
	glDepthMask (GL_FALSE);
	glStencilMask (0);
	
	glMatrixMode (GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(v[0], v[2], v[3], v[1], 0, 1);

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glTranslated (pointer[0], pointer[1], 0);
	
	glUseProgramObjectARB(0);

	[super traverse];
    
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix();
    
	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	
	glDepthMask (GL_TRUE);
	glStencilMask (~0);	
    }
}

@end
