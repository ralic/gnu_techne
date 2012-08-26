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

#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

#include "node.h"

@interface Transform: Node {
    double position[3], orientation[9];
    double translation[3], rotation[9];
    float matrix[16];

    int transform;
}

-(id) parentTransform;
-(void) transform;
-(void) transformCustom;
-(void) transformAsRoot;
-(void) transformRelativeTo: (double *) p;
-(void) transformToTranslation: (double *) r
                   andRotation: (double *) R;

-(int) _get_position;
-(int) _get_orientation;
-(int) _get_transform;

-(void) _set_position;
-(void) _set_orientation;
-(void) _set_transform;

@end

#endif
