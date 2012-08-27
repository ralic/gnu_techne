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
#include "plane.h"

@implementation Plane

-(Plane *) init
{
    self->geom = dCreatePlane (NULL, 0, 0, 1, 0);
    dGeomSetData (self->geom, self);
    
    self = [super init];

    return self;
}

-(void) fasten
{
    /* We're unplacable so do nothing here. */
}

-(void) release
{
    /* We're unplacable so do nothing here. */
}

-(void) recalculate
{
    double *r, *R;

    r = self->position;
    R = self->orientation;

/* 	printf ("%f, %f, %f\n", R[2], R[5], R[8]); */
/* 	printf ("%f, %f, %f\n", r[0], r[1], r[2]); */
    dGeomPlaneSetParams (self->geom,
			 R[2], R[5], R[8],
			 r[0] * R[2] +
			 r[1] * R[5] +
			 r[2] * R[8]);
}

-(void) _set_position
{
    [super _set_position];
    [self recalculate];
}

-(void) _set_orientation
{
    [super _set_orientation];
    [self recalculate];
}

-(void) traverse
{
    int i, j;
    double T[16];

    if (self->debug) {
	glUseProgramObjectARB(0);
	glMatrixMode (GL_MODELVIEW);
	glPushMatrix();
	glGetDoublev (GL_MODELVIEW_MATRIX, T);

	T[12] *= 0;
	T[13] *= 0;
	T[14] *= 0;

	glLoadMatrixd (T);
	glMultMatrixf (self->matrix);

	glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	glEnable (GL_DEPTH_TEST);

	glColor3f (1, 0, 0);
	glLineWidth (1);

	glBegin (GL_QUADS);
	
	for (i = -30 ; i < 30 ; i += 1) {
	    for (j = -30 ; j < 30 ; j += 1) {
		glVertex3f(0.5 * i, 0.5 * j, 0);
		glVertex3f(0.5 * (i + 1), 0.5 * j, 0);
		glVertex3f(0.5 * (i + 1), 0.5 * (j + 1), 0);
		glVertex3f(0.5 * i, 0.5 * (j + 1), 0);
	    }
	}

	glEnd();

	glDisable (GL_DEPTH_TEST);
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	glPopMatrix();
    }
    
    [super traverse];
}

@end
