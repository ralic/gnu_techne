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
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "row.h"
#include "column.h"

@implementation Row

-(Row *) init
{
    self = [super init];
    self->columns = 0;

    return self;
}

-(void) measure
{
    Widget *child;

    self->content[0] = 0;
    self->content[1] = 0;
    
    /* Measure all children and set out content height to the max of
     * theirs and our width to thei sum. */
    
    for(child = (Widget *)self->down;
	child;
	child = (Widget *)child->right) {
	double w, h;

	if (![child isKindOf: [Widget class]]) {
	    continue;
	}
	
	[child measure];

	w = child->content[0] + child->padding[0] + child->padding[1];
	h = child->content[1] + child->padding[2] + child->padding[3];
	
	child->allocation[0] = w;
	self->content[0] += w;

	if (h > self->content[1]) {
	    self->content[1] = h;
	}
    }
    
    /* If we're a table we need to readjust our grandchildren. */
    
    if (self->columns > 0) {
	Widget *grandchild;
	int i, j, done = 0;
	double max;
	
	for (i = 0 ; !done ; i += 1) {
	    done = 1;
	    max = 0;

	    /* Find the largest height for the ith row of each column
	     * child. */
	    
	    for(child = (Widget *)self->down;
		child;
		child = (Widget *)child->right) {
		
		if ([child isKindOf: [Column class]]) {
		    /* Find the ith widget child of the column (if it
		     * exists). */
		    
		    for(grandchild = (Widget *)child->down, j = 0;
			grandchild;
			grandchild = (Widget *)grandchild->right) {
			if ([grandchild isKindOf: [Widget class]]) {
			    if (j == i) {
				break;
			    }

			    j += 1;
			}
		    }

		    if (grandchild) {
			if (max < grandchild->allocation[1]) {
			    max = grandchild->allocation[1];
			}
			
			done = 0;
		    }
		}
	    }

	    /* We're done when we're past the maximum row for all
	     * colmuns. */
	    
	    if (done) {
		continue;
	    }

	    /* Now that we've found the row's height set the
	     * allocation of all widgets in the row to that. */
	    
	    for(child = (Widget *)self->down;
		child;
		child = (Widget *)child->right) {
		
		if ([child isKindOf: [Column class]]) {
		    Widget *last = NULL;
		    
		    for(grandchild = (Widget *)child->down, j = 0;
			grandchild;
			grandchild = (Widget *)grandchild->right) {
			if ([grandchild isKindOf: [Widget class]]) {
			    last = grandchild;

			    if (j == i) {
				break;
			    }

			    j += 1;
			}
		    }

		    if (grandchild) {
			/* If there is an ith column cell expand its
			 * height to the row height. */
			
			child->content[1] += max - grandchild->allocation[1];
			grandchild->allocation[1] = max;
		    } else if (last) {
			/* Otherwise if the column is too short (in
			 * terms of cells) just accumlate the current
			 * row's height on its last cell. */

			child->content[1] += max;
			last->allocation[1] += max;
		    }
		} else if ([child isKindOf: [Widget class]]) {
		    /* If the child is not a column then treat it as a
		     * row with one cell and accumulate all width on
		     * that. */
		    
		    child->allocation[1] += max;
		}
	    }
	}

	/* Now update the column's width to the new max width of all
	 * elements. */
	
	for(child = (Widget *)self->down;
	    child;
	    child = (Widget *)child->right) {

	    if (![child isKindOf: [Widget class]]) {
		continue;
	    }

	    if (self->content[1] < child->content[1]) {
		self->content[1] = child->content[1];
	    }
	}
    }

    /* Allocate our content height to each child. */
    
    for(child = (Widget *)self->down;
	child;
	child = (Widget *)child->right) {

	if (![child isKindOf: [Widget class]]) {
	    continue;
	}

	child->allocation[1] = self->content[1];
    }
}

-(void) adopt: (Node *)child
{
    [super adopt: child];
    
    if ([child isKindOf: [Column class]]) {
	self->columns += 1;
    }
}

-(void) renounce: (Node *)child
{
    [super renounce: child];
    
    if ([child isKindOf: [Column class]]) {
	self->columns -= 1;
    }
}

-(void) prepare
{
    Widget *child;
    double delta;
	
    /* Now distribute the children in the allocated space. */

    for(child = (Widget *)self->down, delta = -0.5 * self->content[0];
	child;
	child = (Widget *)child->right) {

	if (![child isKindOf: [Widget class]]) {
	    continue;
	}

	delta += 0.5 * child->allocation[0];

	child->offset[0] = delta;
	child->offset[1] = 0;

	delta += 0.5 * child->allocation[0];
    }
    
    [super prepare];
}

-(void) traverse
{
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
	
    [self place];    
    [super traverse];

    glPopMatrix();
}

@end

