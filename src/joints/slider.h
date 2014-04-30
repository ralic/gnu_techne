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

#ifndef _SLIDER_H_
#define _SLIDER_H_

#include <lua.h>
#include <ode/ode.h>
#include "joint.h"

@interface Slider: Joint {
@public
    double axis[3], motor[2], stops[2], hardness[2];
    double tolerance, bounce;
}

-(int) _get_axis;
-(int) _get_motor;
-(int) _get_stops;
-(int) _get_tolerance;

-(void) _set_axis;
-(void) _set_motor;
-(void) _set_stops;
-(void) _set_tolerance;

-(int) _get_position;
-(int) _get_rate;

-(void) _set_position;
-(void) _set_rate;

@end

#endif
