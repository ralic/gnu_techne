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

#include <ode/ode.h>
#include <GL/gl.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"
#include "point.h"

@implementation Point

-(Point *) init
{
    self->geom = NULL;
    
    self = [super init];

    return self;
}

-(void) traverse
{
    if (self->debug) {
	glUseProgramObjectARB(0);

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixd (self->matrix);

	glPointSize (5);
	glColor3f (1, 1, 0);

	glBegin (GL_POINTS);
	glVertex3f (0, 0, 0);
	glEnd();

	glPopMatrix();
    }
    
    [super traverse];
}

@end
