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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"
#include "dynamics.h"
#include "environment.h"
#include "body.h"

@implementation Environment

-(void) init
{
    [super init];

    self->space = dSimpleSpaceCreate (NULL);
    dSpaceSetCleanup (self->space, 0);
}

-(void) toggle
{
    if (!linked) {
	dSpaceAdd(_SPACE, (dGeomID)self->space);
    } else {
	dSpaceRemove(_SPACE, (dGeomID)self->space);
    }

    [super toggle];
}

-(void) adopt: (id) child
{
    if ([child isKindOf: [Body class]]) {
	[child fasten];
	[child insertInto: self->space];
    }

    [super adopt: child];
}

-(void) renounce: (id) child
{
    if ([child isKindOf: [Body class]]) {
	[child release];
	[child insertInto: NULL];
    }

    [super renounce: child];
}

@end
