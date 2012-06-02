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
#include <lualib.h>
#include <lauxlib.h>

#include <ode/ode.h>
#include <GL/gl.h>

#include "../array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "gearing.h"
#include "body.h"

@implementation Gearing

-(Joint *) init
{
    self = [super init];
    	
    self->contact.surface.mode = 0;
    self->contact.geom.depth = 0;
    self->contact.geom.g1 = 0;
    self->contact.geom.g2 = 0;

    return self;
}

-(void) update
{
    [super update];

    self->hinge = NULL;
}

-(void) stepBy: (double) h at: (double) t
{
    dReal m;
    dVector3 r;
    dJointID j;

    if (self->engaged && self->bodies[0] && self->bodies[1]) {
	if (!self->hinge) {
	    int i, n;

	    n = dBodyGetNumJoints (self->bodies[0]);

	    for (i = 0 ; i < n ; i += 1) {
		self->hinge = dBodyGetJoint (self->bodies[0], i);

		if (dJointGetType (self->hinge) == dJointTypeHinge) {
		    break;
		} else {
		    self->hinge = NULL;
		}
	    }
	}
	
	dJointGetHingeAxis (self->hinge, self->normal);
	
	dOPE (self->points[0], =, dBodyGetPosition (self->bodies[0]));
	dOPE (self->points[1], =, dBodyGetPosition (self->bodies[1]));
	
	dOP (r, -, self->points[1], self->points[0]);
	m = dDOT (r, self->normal);

	r[0] -= self->normal[0] * m;
	r[1] -= self->normal[1] * m;
	r[2] -= self->normal[2] * m;

	m = 1.0 / (1.0 + self->ratio);
	
	dOPEC (r, *=, m);
	dOP (self->contact.geom.pos, +, self->points[0], r);

	dSafeNormalize3 (r);
	dCROSS (self->contact.geom.normal, =, self->normal, r);
	
	j = dJointCreateContact (_WORLD, _GROUP, &self->contact);
	dJointAttach (j, self->bodies[0], self->bodies[1]);
	dJointSetFeedback (j, &self->feedback);

	j = dJointCreateContact (_WORLD, _GROUP, &self->contact);
	dJointAttach (j, self->bodies[1], self->bodies[0]);
	dJointSetFeedback (j, &self->morefeedback);
	
	/* printf ("* %f, %f\n", */
	/* 	dDISTANCE (self->contact.geom.pos, self->points[0]) / */
	/* 	dDISTANCE (self->contact.geom.pos, self->points[1]), */
	/* 	dLENGTH(dBodyGetAngularVel (self->bodies[1]))/ */
	/* 	dLENGTH(dBodyGetAngularVel (self->bodies[0]))); */
	
	/* printf ("self->normal: %f, %f, %f\n", */
	/* 	self->normal[0], self->normal[1], self->normal[2]); */
	/* printf ("n: %f, %f, %f\n", */
	/* 	self->contact.geom.normal[0], */
	/* 	self->contact.geom.normal[1], */
	/* 	self->contact.geom.normal[2]); */
	
	/* printf ("F: %f, %f, %f\n", */
	/* 	self->contact.fdir1[0], */
	/* 	self->contact.fdir1[1], */
	/* 	self->contact.fdir1[2]); */	
    }
    
    [super stepBy: h at: t];
}

-(int) _get_ratio
{
    if (self->engaged) {
	lua_pushnumber (_L, self->ratio);
    } else {
	lua_pushnil (_L);
    }

    return 1;
}
-(int) _get_forces
{
    double d_1[3], d_2[3];
    int i;

    for(i = 0; i < 3; i += 1) {
	d_1[i] = self->feedback.f1[i] - self->morefeedback.f1[i];
	d_2[i] = self->feedback.f2[i] - self->morefeedback.f2[i];
    }
    
    lua_newtable (_L);

    /* The force applied on the first body. */
        
    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE, d_2, 1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE, d_1, 1, 3);
    }

    lua_rawseti (_L, -2, 1);

    /* The force applied on the second body. */
	
    lua_newtable (_L);
        
    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE, d_1, 1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE, d_2, 1, 3);
    }

    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_torques
{
    double d_1[3], d_2[3];
    int i;

    for(i = 0; i < 3; i += 1) {
	d_1[i] = self->feedback.t1[i] - self->morefeedback.t1[i];
	d_2[i] = self->feedback.t2[i] - self->morefeedback.t2[i];
    }
    
    lua_newtable (_L);

    /* The torque applied on the first body. */
        
    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE, d_2, 1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE, d_1, 1, 3);
    }

    lua_rawseti (_L, -2, 1);

    /* The torque applied on the second body. */
	
    lua_newtable (_L);
        
    if (self->inverted) {
	array_createarray (_L, ARRAY_TDOUBLE, d_1, 1, 3);
    } else {
	array_createarray (_L, ARRAY_TDOUBLE, d_2, 1, 3);
    }

    lua_rawseti (_L, -2, 2);

    return 1;
}

-(void) _set_ratio
{
    if (!lua_isnil(_L, 3)) {
	self->ratio = lua_tonumber (_L, 3);
	self->engaged = 1;
    } else {
	self->ratio = 0;
	self->engaged = 0;
    }
}

-(void) traverse
{
    if (self->engaged && self->debug &&
	self->bodies[0] && self->bodies[1]) {
	int i, j;
	
	glUseProgramObjectARB(0);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glDepthMask (GL_FALSE);

	glPointSize (3);
	glLineWidth (1);

	glEnable (GL_DEPTH_TEST);
	glColor3f (1, 0, 1);
	
	/* Draw the first gear. */

	for (j = 0; j < 2 ; j += 1) {
	    dReal *p;
	    dVector3 r, t;
	    dReal R;

	    p = self->points[j];
	    
	    dOP(r, -, self->contact.geom.pos, p);
	    R = dLENGTH (r);
	    dOPEC(r, /=, R);
	    dCROSS(t, =, r, self->normal);

	    glBegin (GL_LINE_STRIP);

	    for (i = 0 ; i < 25 ; i += 1) {
		glVertex3f (p[0] + R * (cos(i * 2 * M_PI / 24) * r[0] +
					sin(i * 2 * M_PI / 24) * t[0]),
			    p[1] + R * (cos(i * 2 * M_PI / 24) * r[1] +
					sin(i * 2 * M_PI / 24) * t[1]),
			    p[2] + R * (cos(i * 2 * M_PI / 24) * r[2] +
					sin(i * 2 * M_PI / 24) * t[2]));
	    }
	
	    glVertex3f (p[0], p[1],p[2]);

	    glEnd();
	}
	
	glDepthMask (GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable (GL_DEPTH_TEST);

	glPopMatrix();
    }
    
    [super traverse];
}

@end
