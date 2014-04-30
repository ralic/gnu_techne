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

#ifndef _CHAIN_H_
#define _CHAIN_H_

#include <lua.h>
#include <ode/ode.h>
#include "joint.h"

@interface Chain: Joint {
    dContact contacts[2];
    dBodyID sprockets[2], run;
    dJointFeedback feedbacks[2];

    double radii[2], direction;
}

-(int) _get_radii;
-(int) _get_velocity;
-(void) _set_radii;
-(void) _set_velocity;
-(void) _set_contacts;
-(int) _get_contacts;

@end

#endif
