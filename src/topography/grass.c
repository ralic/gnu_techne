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

#include <stdlib.h>
#include <math.h>

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "shader.h"
#include "grass.h"

#define N_K 32               /* Number of stiffness samples. */
#define N_S 32               /* Number of blade segments. */
#define K_0 2.1              /* Lower bound of stiffness parameter. */
#define K_1 2.5              /* Upper bound of stiffness parameter. */

@implementation Grass

-(void)init
{
    float *texels, x, y, k, theta_0, phi, l_i, A, Delta;
    int i, j;

#include "glsl/grass_tesselation_control.h"
#include "glsl/grass_tesselation_evaluation.h"
#include "glsl/grass_geometry.h"

    self->sources[T_TESSELATION_CONTROL_STAGE] = glsl_grass_tesselation_control;
    self->sources[T_TESSELATION_EVALUATION_STAGE] = glsl_grass_tesselation_evaluation;
    self->sources[T_GEOMETRY_STAGE] = glsl_grass_geometry;

    /* Calculate the deflection texture. */

    texels = malloc(N_K * N_S * 3 * sizeof(float));

    for (j = 0 ; j < N_K ; j += 1) {
        x = y = theta_0 = 0;
        k = K_0 + (K_1 - K_0) * (float)j / (N_K - 1);
        
        for (i = 0 ; i < N_S ; i += 1) {
            texels[3 * ((N_S * j) + i) + 0] = x;
            texels[3 * ((N_S * j) + i) + 1] = y;
            texels[3 * ((N_S * j) + i) + 2] = theta_0;

            if (i == N_S) {
                break;
            }
            
            l_i = (1 - (float)i / (N_S - 1));
            A = l_i * l_i / k;
            Delta = 1 - 4 / 2.42 * A * (M_PI / 2 - theta_0 - A);
            phi = (1 - sqrt(Delta)) / (2 / 2.42 * A);

            if (phi > M_PI / 2) {
                phi = M_PI - phi;
            }
            
            theta_0 += M_PI / 2 - theta_0 - phi;
            x += sin(theta_0) / (N_S - 1);
            y += cos(theta_0) / (N_S - 1);
        }
    }
    
    /* for (i = N_K / 2, j = 0 ; j < N_S ; j += 1) { */
    /*     _TRACE ("%f, %f, %f\n", texels[3 * ((N_S * i) + j) + 0], texels[3 * ((N_S * i) + j) + 1], texels[3 * ((N_S * i) + j) + 2]); */
    /* } */
    
    [super init];

    glGenTextures (1, &self->deflections);
    glBindTexture(GL_TEXTURE_2D, self->deflections);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, N_S, N_K, 0, GL_RGB,
                 GL_FLOAT, texels);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    free (texels);
}

-(void)free
{
    glDeleteTextures (1, &self->deflections);

    [super free];
}

-(void) addSourceToVegetationShader: (ShaderMold *)shader for: (t_Enumerated)stage
{
    [super addSourceToVegetationShader: shader for: stage];
    [shader addSourceString: self->sources[stage] for: stage];
}

-(void) configureVegetationShader: (Shader *)shader
{
    [super configureVegetationShader: shader];
    [shader setSamplerUniform: "deflections" to: self->deflections];
}

@end
