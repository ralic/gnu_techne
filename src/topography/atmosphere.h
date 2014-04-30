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

#ifndef _ATMOSPHERE_H_
#define _ATMOSPHERE_H_

#include "shader.h"
#include "shape.h"

@interface Atmosphere: Shader {
@public
    unsigned int skylight;

    int size[2], explicit;
    float azimuth, elevation, turbidity, intensity[3], direction[3], direction_w[3];
    float rayleigh[3], mie;
}

+(Atmosphere *) instance;
-(void) toggle;

@end

@interface AtmosphereShape: Shape {
    unsigned int positions, mapping;
    double radius;
}

@end

#endif
