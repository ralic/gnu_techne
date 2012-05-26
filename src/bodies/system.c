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
#include <lualib.h>
#include <lauxlib.h>

#include <ode/ode.h>

#include "techne.h"
#include "system.h"
#include "body.h"

static void insert (Node *root, dSpaceID space)
{
    Node *child;
    
    /* Don't kidnap other system nodes' children. */
	
    if (![root isKindOf: [System class]]) {
	if ([root isKindOf: [Body class]] && ((Body *)root)->geom) {
	    [((Body *)root) insertInto: space];
	}

	for (child = root->down ; child ; child = child->right) {
	    insert (child, space);
	}
    }
}

@implementation System

-(System *) init
{
    [super init];

    self->space = dSimpleSpaceCreate (NULL);
    dSpaceSetCleanup (self->space, 0);
    
    return self;
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

-(void) adopt: (id) child named: (char *) name
{
    insert (child, self->space);
    
    [super adopt: child];
}

@end
