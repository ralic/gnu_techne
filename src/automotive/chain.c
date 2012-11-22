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
#include "gl.h"

#include "../array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "body.h"
#include "chain.h"

@implementation Chain

-(void) init
{
    dMass m;

    self->joint = dJointCreateAMotor (_WORLD, NULL);

    /* Initialize the object. */
    
    self->radii[0] = 0.041;
    self->radii[1] = 0.104;
 
    self->direction = 1;

    /* Create the chain run body and initalize the contact joints. */

    dMassSetParameters (&m, 1e-6, 0, 0, 0, 1e-6, 1e-6, 1e-6, 0, 0, 0);
	
    self->run = dBodyCreate (_WORLD);

    dBodySetData (self->run, self);
    dBodySetMass (self->run, &m);
    dBodySetGravityMode (self->run, 0);
	
    self->contacts[0].surface.mode = dContactFDir1;
    self->contacts[0].surface.mu = dInfinity;

    self->contacts[1].surface.mode = dContactFDir1;
    self->contacts[1].surface.mu = dInfinity;
        
    [super init];
}

-(void) update
{
    [super update];

    self->sprockets[0] = dJointGetBody (self->joint, 0);
    self->sprockets[1] = dJointGetBody (self->joint, 1);

    dJointAttach (self->joint, 0, 0);
}

-(void) free
{
    dBodyDestroy (self->run);
    
    [super free];
}

-(void) stepBy: (double) h at: (double) t
{
    dJointID j;
    dReal R, r, d, s, S, w = 1;
    const dReal *q, *p;
    dVector3 qprime, u, v;

    if (self->sprockets[0] && self->sprockets[1]) {
	p = dBodyGetPosition (self->sprockets[0]);
	q = dBodyGetPosition (self->sprockets[1]);

	w = (dDOT(self->feedbacks[0].f1, self->contacts[1].fdir1) >=
	     dDOT(self->feedbacks[1].f1, self->contacts[1].fdir1)) ? -1 : 1;

	self->direction = w;
    
	/* Calculate q - p in the frame of the front sprocket. */
    
	dBodyGetPosRelPoint(self->sprockets[0], q[0], q[1], q[2], qprime);
    
	r = self->radii[0];
	R = self->radii[1];

	d = R - r;
	s = qprime[0] * qprime[0] + qprime[2] * qprime[2];
	S = sqrt(s - d * d);

	/* Solve for the vectors from the center of each sprocket to the
	   point of contact with the chain in the frame of reference of the
	   front sprocket and then transform the result into the world
	   frame. */
    
	dBodyVectorToWorld (self->sprockets[0],
			    (-w * qprime[2] * r * S + qprime[0] * r * d) / s,
			    0,
			    (w * qprime[0] * r * S + qprime[2] * r * d) / s,
			    u);

	dBodyVectorToWorld (self->sprockets[0],
			    (-w * qprime[2] * R * S + qprime[0] * R * d) / s,
			    0,
			    (w * qprime[0] * R * S + qprime[2] * R * d) / s,
			    v);

	/* Add the position of the center of the sprocket to each calculated
	   vector to get the point of contact with the chain in the world
	   frame. */

	dOP (self->contacts[0].geom.pos, -, p, u);
	dOP (self->contacts[1].geom.pos, -, q, v);

	/* Set the rest of the contact geometry. */
    
	if (self->direction < 0) {
	    dOP (self->contacts[0].fdir1, -,
		 self->contacts[0].geom.pos,
		 self->contacts[1].geom.pos);
	} else {
	    dOP (self->contacts[0].fdir1, -,
		 self->contacts[1].geom.pos,
		 self->contacts[0].geom.pos);
	}
	
	dOPE (self->contacts[0].geom.normal, =, u);

	dSafeNormalize3 (self->contacts[0].fdir1);
	dSafeNormalize3 (self->contacts[0].geom.normal);

	dOPE (self->contacts[1].fdir1, =, self->contacts[0].fdir1);
	dOPE (self->contacts[1].geom.normal, =, self->contacts[0].geom.normal);
	dOPEC (self->contacts[1].geom.normal, *=, -1);
	
	self->contacts[0].geom.depth = 0;
	self->contacts[0].geom.g1 = 0;
	self->contacts[0].geom.g2 = 0;

	self->contacts[1].geom.depth = 0;
	self->contacts[1].geom.g1 = 0;
	self->contacts[1].geom.g2 = 0;

	assert (dBodyGetNumJoints (self->run) == 0);
	
	j = dJointCreateContact (_WORLD, _GROUP, &self->contacts[0]);

	dJointSetFeedback (j, &self->feedbacks[0]);
	dJointAttach (j, self->run, self->sprockets[0]);

	j = dJointCreateContact (_WORLD, _GROUP, &self->contacts[1]);

	dJointSetFeedback (j, &self->feedbacks[1]);
	dJointAttach (j, self->run, self->sprockets[1]);
	
	/* If the active chain branch has changed we need
	   to flip the chain velocity. */
    
	if (w * self->direction < 1) {
	    const dReal *v;
	    dVector3 l, u;
	    dReal V_l;
	
	    v = dBodyGetLinearVel(self->run);
	    dOP (l, -, p, q);
	    V_l = -2 * dDOT(v, l) / dDOT (l, l);
	    dOPC (u, *, l, V_l);
	    dOPE (u, +=, v);

	    dBodySetLinearVel (self->run, u[0], u[1], u[2]);
	}

	/* Constrain the motion of the chain run properly. */
		    
	{
	    const dReal *v, *l;
	    dMatrix3 R;
	    dReal vdotl;

	    v = dBodyGetLinearVel(self->run);
	    l = self->contacts[0].fdir1;
	    vdotl = dDOT (v, l);

	    dBodySetAngularVel (self->run, 0, 0, 0);
	    dBodySetLinearVel (self->run,
			       vdotl * l[0],
			       vdotl * l[1],
			       vdotl * l[2]);
	
	    dBodySetPosition (self->run,
			      self->contacts[0].geom.pos[0],
			      self->contacts[0].geom.pos[1],
			      self->contacts[0].geom.pos[2]);

	    dRSetIdentity (R);

	    dBodySetRotation (self->run, R);
	}
    }

    [super stepBy: h at: t];
}

-(int) _get_radii
{
    int i;
    
    lua_newtable (_L);
        
    for(i = 0; i < 2; i += 1) {
	lua_pushnumber (_L, self->radii[i]);
	lua_rawseti (_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_velocity
{
    const dReal *v;
	
    v = dBodyGetLinearVel(self->run);
    array_createarray (_L, ARRAY_TDOUBLE, (double *)v, 1, 3);

    return 1;
}

-(int) _get_forces
{
    lua_newtable (_L);

    /* The force applied on the front sprocket. */

    array_createarray (_L, ARRAY_TDOUBLE,
		       self->feedbacks[0].f2, 1, 3);

    lua_rawseti (_L, -2, 1);

    /* The force applied on the rear sprocket. */
	
    array_createarray (_L, ARRAY_TDOUBLE,
		       self->feedbacks[1].f2, 1, 3);

    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_torques
{
    lua_newtable (_L);

    /* The torque applied on the front sprocket. */

    array_createarray (_L, ARRAY_TDOUBLE,
		       self->feedbacks[0].t2, 1, 3);

    lua_rawseti (_L, -2, 1);

    /* The torque applied on the rear sprocket. */
	
    array_createarray (_L, ARRAY_TDOUBLE,
		       self->feedbacks[1].t2, 1, 3);

    lua_rawseti (_L, -2, 2);	

    return 1;
}

-(int) _get_contacts
{
    int j;

    if (self->sprockets[0] && self->sprockets[1]) {
	lua_newtable (_L);
        
	for(j = 0 ; j < 2 ; j += 1) {
	    array_createarray (_L, ARRAY_TDOUBLE,
			       self->contacts[j].geom.pos, 1, 3);

	    lua_rawseti (_L, -2, j + 1);
	}
    }
    
    return 1;
}

-(void) _set_radii
{
    int i;
    
    if(!lua_isnil (_L, 3)) {
	for(i = 0 ; i < 2 ; i += 1) {
	    lua_pushinteger (_L, i + 1);
	    lua_gettable (_L, 3);
	    self->radii[i] = lua_tonumber (_L, -1);
                
	    lua_pop (_L, 1);
	}
    }
}

-(void) _set_velocity
{
    T_WARN_READONLY;
}

-(void) _set_contacts
{
    T_WARN_READONLY;
}

-(void) traverse
{
    if (self->debug && self->sprockets[0] && self->sprockets[1]) {
	dBodyID b;
	const dReal *v;
	int i, j;
	
	glUseProgramObjectARB(0);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_BLEND);
	glDepthMask (GL_FALSE);

	/* Draw the sprockets. */

	for (j = 0 ; j < 2 ; j += 1) {
	    const dReal *p, *R;
	    double T[16];
	    
	    glMatrixMode (GL_MODELVIEW);
	    glPushMatrix();

	    b = self->sprockets[j];
	    p = dBodyGetPosition (b);
	    R = dBodyGetRotation (b);

	    T[0] = R[0]; T[1] = R[4]; T[2] = R[8]; T[3] = 0;
	    T[4] = R[1]; T[5] = R[5]; T[6] = R[9]; T[7] = 0;
	    T[8] = R[2]; T[9] = R[6]; T[10] = R[10]; T[11] = 0;
	    T[12] = p[0]; T[13] = p[1]; T[14] = p[2]; T[15] = 1;
	    
	    glMultMatrixd(T);

	    glColor3f (1, 1, 0);
	    glLineWidth (1);

	    glBegin (GL_LINE_STRIP);

	    for (i = 0 ; i < 25 ; i += 1) {
		glVertex3f (self->radii[j] * cos(i * 2 * M_PI / 24),
			    0,
			    self->radii[j] * sin(i * 2 * M_PI / 24));
	    }
	
	    glVertex3f (0, 0, 0);

	    glEnd();

	    glPopMatrix();
	}
 
	/* Draw the contact geometry. */
	
	glColor3f (1, 0, 0);
	glPointSize (3);
	
	glBegin (GL_POINTS);

	glVertex3d (self->contacts[0].geom.pos[0],
		    self->contacts[0].geom.pos[1],
		    self->contacts[0].geom.pos[2]);
	
	glVertex3d (self->contacts[1].geom.pos[0],
		    self->contacts[1].geom.pos[1],
		    self->contacts[1].geom.pos[2]);
	
	glEnd();

	/* Draw the active chain branch. */
	
	glColor3f (0, 1, 0);

	glBegin (GL_LINES);

	glVertex3d (self->contacts[0].geom.pos[0],
		    self->contacts[0].geom.pos[1],
		    self->contacts[0].geom.pos[2]);

	glVertex3d (self->contacts[1].geom.pos[0],
		    self->contacts[1].geom.pos[1],
		    self->contacts[1].geom.pos[2]);
	glEnd();

	/* And the chain velocity as well. */
	
	v = dBodyGetLinearVel (self->run);

	/* { */
	/*     dReal *w, *w1; */

	/*     w = dBodyGetAngularVel (self->sprockets[0]); */
	/*     w1 = dBodyGetAngularVel (self->sprockets[1]); */
	/*     printf ("%f, %f, %f, %f, %f\n", */
	/* 	    dLENGTH (v), dLENGTH (w) * self->radii[0], */
	/* 	    dLENGTH (w1) * self->radii[1], */
	/* 	    dLENGTH (v) / dLENGTH (w) / self->radii[0], */
	/* 	    dLENGTH (v) / dLENGTH (w1) / self->radii[1]); */
	/* } */

	glColor3f (1, 0, 0);
	glLineWidth (2);
	
	glBegin (GL_LINES);

	glVertex3d (self->contacts[0].geom.pos[0],
		    self->contacts[0].geom.pos[1],
		    self->contacts[0].geom.pos[2]);

	glVertex3d (self->contacts[0].geom.pos[0] + v[0] / 100,
		    self->contacts[0].geom.pos[1] + v[1] / 100,
		    self->contacts[0].geom.pos[2] + v[2] / 100);
	glEnd();

	glDepthMask (GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_DEPTH_TEST);
    }
    
    [super traverse];
}

@end
