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

#include "polygon.h"

GLenum array_to_gl_type[] = {
    GL_DOUBLE, GL_FLOAT, GL_FALSE, GL_FALSE, GL_UNSIGNED_INT, GL_INT,
    GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_BYTE, GL_BYTE
};

@implementation Polygon

-(void) traverse
{
    if (self->vertices) {
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixd (self->matrix);

	glUseProgramObjectARB(0);

	if (self->color[3] < 1) {
	    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	    glEnable(GL_BLEND);
	}
	
	glEnable(GL_DEPTH_TEST);

	glColor4dv(self->color);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, array_to_gl_type[self->vertices->type],
			0, self->vertices->values.any);
	glDrawArrays(GL_POLYGON, 0, self->vertices->size[0]);
	glDisableClientState(GL_VERTEX_ARRAY);
    
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
    
	glMatrixMode (GL_MODELVIEW);
	glPopMatrix();
    }
    
    [super traverse];
}

@end
