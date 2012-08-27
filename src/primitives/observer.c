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
#include <AL/al.h>

#include "gl.h"

#include "techne.h"
#include "observer.h"

@implementation Observer

-(void) transform
{
    [super transform];

    {
	double *R, *r;
    
	R = self->rotation;
	r = self->translation;

	/*  Compute a matrix as in:
    
	    gluLookAt (r[0], r[1], r[2],
	    r[0] - R[2], r[1] - R[5], r[2] - R[8],
	    -R[0], -R[3], -R[6]);

	    and leave it on the top of the matrix stack. */

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt (r[0], r[1], r[2],
		   r[0] - R[2], r[1] - R[5], r[2] - R[8],
		   -R[0], -R[3], -R[6]);
    }
    
    /* Update the OpenAL listener object to reflect
       this transform. */
    
    {
	float T[16], R[6], r[3];

	glGetFloatv (GL_MODELVIEW_MATRIX, T);

	R[0] = -T[8]; R[1] = -T[9]; R[2] = -T[10];
	R[3] = T[4]; R[4] = T[5]; R[5] = T[6];

	r[0] = T[12]; r[1] = T[13]; r[2] = T[14]; 

	alListenerfv (AL_ORIENTATION, R);
	alListenerfv (AL_POSITION, r);
    }
}

@end
