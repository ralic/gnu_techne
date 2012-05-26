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
#include <GL/glu.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"
#include "capsule.h"

@implementation Capsule

-(Capsule *) init
{
    self->quadric = gluNewQuadric();
    gluQuadricDrawStyle(self->quadric, GLU_LINE);
    gluQuadricNormals(self->quadric, GLU_NONE);

    self->geom = dCreateCapsule (NULL, 1, 1);
    dGeomSetData (self->geom, self);

    self->radius = 1;
    self->height = 1;
    
    self = [super init];

    return self;
}

-(int) _get_radius
{
    lua_pushnumber (_L, self->radius);
    
    return 1;
}

-(int) _get_length
{
    lua_pushnumber (_L, self->height);

    return 1;
}

-(void) _set_radius
{
    radius = lua_tonumber (_L, 3);

    dGeomCapsuleSetParams (self->geom, self->radius, self->height);
}

-(void) _set_length
{
    height = lua_tonumber (_L, 3);

    dGeomCapsuleSetParams (self->geom, self->radius, self->height);
}

-(void) traverse
{
    if (self->debug) {
	glUseProgramObjectARB(0);

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixd (self->matrix);

	glLineWidth (1);

	glEnable (GL_DEPTH_TEST);

	glColor3f (0, 1, 0);
	glBegin (GL_LINES);
	glVertex3f (0, 0, -0.5 * self->height - self->radius);
	glVertex3f (0, 0, 0.5 * self->height + self->radius);
	glEnd();
	
	glTranslatef (0, 0, -0.5 * self->height);

	glColor3f (1, 0, 0);
	gluCylinder (self->quadric, self->radius, self->radius,
		     self->height, 16, 1);
    
	glDisable (GL_DEPTH_TEST);

	glPopMatrix();
    }
    
    [super traverse];
}

@end
