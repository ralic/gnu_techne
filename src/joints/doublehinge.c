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

#include "array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "doublehinge.h"
#include "body.h"

@implementation Doublehinge

-(Joint *) init
{
    self->joint = dJointCreateDHinge (_WORLD, NULL);
    
    self->anchors[0][0] = 0;
    self->anchors[0][1] = 0;
    self->anchors[0][2] = 0;
    
    self->anchors[1][0] = 0;
    self->anchors[1][1] = 0;
    self->anchors[1][2] = 0;

    self->axis[0] = 0;
    self->axis[1] = 0;
    self->axis[2] = 1;

    self = [super init];

    return self;
}

-(void) update
{
    if (dJointGetBody (self->joint, 0) ||
	dJointGetBody (self->joint, 1)) {
	dJointGetDHingeAxis (self->joint, self->axis);
	dJointGetDHingeAnchor1 (self->joint, self->anchors[0]);
	dJointGetDHingeAnchor2 (self->joint, self->anchors[1]);
    }
    
    [super update];
  
    /* Axis and anchor should be set after the
       joint has been attached. */

    if (self->linked) {
	dJointSetDHingeAxis (self->joint,
			     self->axis[0],
			     self->axis[1],
			     self->axis[2]);
    
	dJointSetDHingeAnchor1 (self->joint,
				self->anchors[0][0],
				self->anchors[0][1],
				self->anchors[0][2]);
    
	dJointSetDHingeAnchor2 (self->joint,
				self->anchors[1][0],
				self->anchors[1][1],
				self->anchors[1][2]);
    }
}

-(int) _get_anchors
{
    int j;
    
    dJointGetDHingeAnchor1 (self->joint, self->anchors[0]);
    dJointGetDHingeAnchor2 (self->joint, self->anchors[1]);
    
    lua_newtable (_L);

    for(j = 0 ; j < 2 ; j += 1) {
	array_createarray (_L, ARRAY_TDOUBLE, self->anchors[j], 1, 3);
	lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(int) _get_axis
{
    dJointGetDHingeAxis (self->joint, self->axis);
    array_createarray (_L, ARRAY_TDOUBLE, self->axis, 1, 3);

    return 1;
}

-(int) _get_tolerance
{
    lua_pushnumber (_L, self->tolerance);

    return 1;
}

-(void) _set_axis
{
    array_Array *array;
    int i;
    
    array = array_checkcompatible (_L, 3, ARRAY_TDOUBLE, 1, 3);

    dSafeNormalize3 (array->values.doubles);

    for (i = 0 ; i < 3 ; i += 1) {
	self->axis[i] = array->values.doubles[i];
    }

    if (self->bodies[0] || self->bodies[1]) {
        dJointSetDHingeAxis (self->joint,
			     array->values.doubles[0],
			     array->values.doubles[1],
			     array->values.doubles[2]);
    }
}

-(void) _set_anchors
{
    array_Array *array;
    int i;
    
    if(!lua_isnil (_L, 3)) {
	/* Set the first anchor. */
	
	lua_pushinteger (_L, 1);
	lua_gettable (_L, 3);
	
	array = array_checkcompatible (_L, -1, ARRAY_TDOUBLE, 1, 3);

	dJointSetDHingeAnchor1 (self->joint,
			       array->values.doubles[0],
			       array->values.doubles[1],
			       array->values.doubles[2]);

	for (i = 0 ; i < 3 ; i += 1) {
	    self->anchors[0][i] = array->values.doubles[i];
	}
	
	/* Set the second anchor. */
	
	lua_pushinteger (_L, 2);
	lua_gettable (_L, 3);
	
	array = array_checkcompatible (_L, -1, ARRAY_TDOUBLE, 1, 3);

	dJointSetDHingeAnchor2 (self->joint,
				array->values.doubles[0],
				array->values.doubles[1],
				array->values.doubles[2]);

	for (i = 0 ; i < 3 ; i += 1) {
	    self->anchors[1][i] = array->values.doubles[i];
	}

	lua_pop (_L, 2);
    }
}

-(void) _set_tolerance
{
    self->tolerance = lua_tonumber (_L, 3);

    dJointSetDHingeParam (self->joint, dParamCFM, self->tolerance);
}

-(void) traverse
{
    if (self->debug) {
	dBodyID a, b;
	const dReal *q;
	dVector3 p[2], x;
	int i;

	a = dJointGetBody (self->joint, 0);
	b = dJointGetBody (self->joint, 1);
	
	assert (a || b);

	dJointGetDHingeAnchor1 (self->joint, p[0]);
	dJointGetDHingeAnchor2 (self->joint, p[1]);
	dJointGetDHingeAxis (self->joint, x);

	glUseProgramObjectARB(0);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glDepthMask (GL_FALSE);

	glMatrixMode (GL_MODELVIEW);

	for (i = 0 ; i < 2 ; i += 1) {
	    glPushMatrix();
	    glTranslatef(p[i][0], p[i][1], p[i][2]);
	
	    /* Draw the dots. */

	    glPointSize (3);

	    glColor3f(0, 0, 1);

	    glBegin (GL_POINTS);
	    glVertex3f (x[0], x[1], x[2]);
	    glEnd ();	
	
	    /* Draw the lines. */

	    glLineWidth (1);
	    
	    glColor3f(0, 0, 1);

	    glBegin (GL_LINES);
	    glVertex3f (0, 0, 0);
	    glVertex3f (x[0], x[1], x[2]);
	    
	    glEnd ();	
	    glPopMatrix ();
	}

	glColor3f(0, 1, 0);
	glLineWidth (3);
	glPointSize (8);

	glBegin (GL_LINES);
	glVertex3f (p[0][0], p[0][1], p[0][2]);
	glVertex3f (p[1][0], p[1][1], p[1][2]);
	glEnd();
	
	if (b) {
	    q = dBodyGetPosition (b);

	    /* Draw one arm. */
	    
	    glBegin (GL_LINES);
	    glVertex3f (p[1][0], p[1][1], p[1][2]);
	    glVertex3f (q[0], q[1], q[2]);
	    glEnd();
	    
	    glBegin(GL_POINTS);
	    glVertex3f (p[1][0], p[1][1], p[1][2]);
	    glEnd();
	}

	if (a) {
	    q = dBodyGetPosition (a);

	    /* Draw the other arm and anchor. */
	
	    glBegin (GL_LINES);
	    glVertex3f (p[0][0], p[0][1], p[0][2]);
	    glVertex3f (q[0], q[1], q[2]);
	    glEnd();
	    
	    glBegin(GL_POINTS);
	    glVertex3f (p[0][0], p[0][1], p[0][2]);
	    glEnd();
	}
	
	glDepthMask (GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_DEPTH_TEST);
    }
    
    [super traverse];
}

@end
