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

#ifndef _SHADER_H_
#define _SHADER_H_

#include <lua.h>
#include "node.h"

typedef struct {
    unsigned int block;
    int type, size, offset, arraystride, matrixstride;
} shader_Uniform;

typedef enum {
    VERTEX_STAGE,
    GEOMETRY_STAGE,
    FRAGMENT_STAGE
}  shader_Stage;

@interface Shader: Node {
@public
    unsigned int *blocks, name;
    shader_Uniform *uniforms;
    int blocks_n, ismold;
}


+(int) addUniformBlockNamed: (const char *)name
                   forStage: (shader_Stage)stage
                 withSource: (const char *)declaration;
-(void) addSource: (const char *) source for: (shader_Stage)stage;
-(void) link;
-(id)initFrom: (Shader *) mold;

@end

#endif
