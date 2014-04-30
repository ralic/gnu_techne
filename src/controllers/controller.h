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

#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

#include "device.h"
#include <linux/input.h>

@interface Controller: Device {
    int device, has_force;
    struct ff_effect effect;
    double force[2];
}

-(void) initWithDevice: (const char *)name;
-(int) _get_force;
-(void) _set_force;

-(int) _get_buttons;
-(void) _set_buttons;

-(int) _get_axes;
-(void) _set_axes;

@end

#endif
