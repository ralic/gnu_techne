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

#ifndef _SWATCH_H_
#define _SWATCH_H_

#include <lua.h>
#include "graphic.h"
#include "shader.h"

@interface Swatch: Graphic {
@public
    float values[3], weights[3], resolutions[2];
    const char *sources[T_STAGES_N];

    Texture *detail;
    int reference;
}

-(void) configureVegetationShader: (Shader *)shader;
-(void) addSourceToVegetationShader: (ShaderMold *)shader for: (t_Enumerated)stage;
-(int) _get_reference;
-(void) _set_reference;
-(int) _get_resolution;
-(void) _set_resolution;
-(int) _get_detail;
-(void) _set_detail;

@end

#endif
