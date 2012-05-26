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
#include "box.h"

@implementation Box

-(Box *) init
{
    self->geom = dCreateBox (NULL, 1, 1, 1);
    dGeomSetData (self->geom, self);
    
    self->size[0] = 1;
    self->size[1] = 1;
    self->size[2] = 1;

    self = [super init];

    return self;
}

-(int) _get_size
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber (_L, self->size[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(void) _set_size
{
    int i;
    
    if(lua_istable (_L, 3)) {
	for(i = 0 ; i < 3 ; i += 1) {
	    lua_rawgeti (_L, 3, i + 1);
	    self->size[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }

    dGeomBoxSetLengths (self->geom,
			self->size[0],
			self->size[1],
			self->size[2]);
}

-(void) traverse
{
    dReal *l;

    if (self->debug) {
	glUseProgramObjectARB(0);

	l = self->size;

	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixd (self->matrix);

	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glEnable (GL_DEPTH_TEST);
	glDepthMask (GL_FALSE);

	glColor3f (1, 0, 0);
	glLineWidth (1);

	glBegin (GL_QUAD_STRIP);
	
	glVertex3f(-0.5 * l[0], -0.5 * l[1], -0.5 * l[2]);
	glVertex3f(-0.5 * l[0], -0.5 * l[1], 0.5 * l[2]);
	glVertex3f(-0.5 * l[0], 0.5 * l[1], -0.5 * l[2]);
	glVertex3f(-0.5 * l[0], 0.5 * l[1], 0.5 * l[2]);

	glVertex3f(0.5 * l[0], 0.5 * l[1], -0.5 * l[2]);
	glVertex3f(0.5 * l[0], 0.5 * l[1], 0.5 * l[2]);
 
	glVertex3f(0.5 * l[0], -0.5 * l[1], -0.5 * l[2]);
	glVertex3f(0.5 * l[0], -0.5 * l[1], 0.5 * l[2]);
       
	glVertex3f(-0.5 * l[0], -0.5 * l[1], -0.5 * l[2]);
	glVertex3f(-0.5 * l[0], -0.5 * l[1], 0.5 * l[2]);

	glEnd();
	
	glDisable (GL_DEPTH_TEST);
	glDepthMask (GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	glPopMatrix();
    }
    
    [super traverse];
}

@end
