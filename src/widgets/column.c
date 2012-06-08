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

#include <GL/gl.h>

#include "techne.h"
#include "row.h"
#include "column.h"

@implementation Column

-(Column *) init
{
    self = [super init];
    self->rows = 0;

    return self;
}

-(void) measure
{
    Widget *child;

    self->content[0] = 0;
    self->content[1] = 0;
    
    /* Measure all children and set our content width to the max of
     * theirs and our content height to the sum. */
    
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
	
	child->allocation[1] = h;
	self->content[1] += h;

	if (w > self->content[0]) {
	    self->content[0] = w;
	}
    }

    /* If we're a table we need to readjust our grandchildren. */
    
    if (self->rows > 0) {
	Widget *grandchild;
	int i, j, done = 0;
	double max;
	
	for (i = 0 ; !done ; i += 1) {
	    done = 1;
	    max = 0;

	    /* Find the largest width for the ith column of each row
	     * child. */
	    
	    for(child = (Widget *)self->down;
		child;
		child = (Widget *)child->right) {
		
		if ([child isKindOf: [Row class]]) {
		    /* Find the ith widget child of the row (if it
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
			if (max < grandchild->allocation[0]) {
			    max = grandchild->allocation[0];
			}
			
			done = 0;
		    }
		}		
	    }

	    /* We're done when we're past the maximum column for all
	     * rows. */
	    
	    if (done) {
		continue;
	    }

	    /* Now that we've found the column's width set the
	     * allocation of all widgets in the column to that. */
	    
	    for(child = (Widget *)self->down;
		child;
		child = (Widget *)child->right) {
		
		if ([child isKindOf: [Row class]]) {
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
			/* If there is an ith row cell expand its
			 * width to the column width. */
			
			child->content[0] += max - grandchild->allocation[0];
			grandchild->allocation[0] = max;
		    } else if (last) {
			/* Otherwise if the row is too short (in terms
			 * of cells) just accumlate the current
			 * column's width on its last cell. */

			child->content[0] += max;
			last->allocation[0] += max;
		    }
		} else if ([child isKindOf: [Widget class]]) {
		    /* If the child is not a row then treat it as a
		     * row with one cell and accumulate all width on
		     * that. */
		    
		    child->allocation[0] += max;
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

	    if (self->content[0] < child->content[0]) {
		self->content[0] = child->content[0];
	    }
	}
    }

    /* Allocate our content width to each child. */
    
    for(child = (Widget *)self->down;
	child;
	child = (Widget *)child->right) {

	if (![child isKindOf: [Widget class]]) {
	    continue;
	}

	child->allocation[0] = self->content[0];
    }
}

-(void) adopt: (Node *)child
{
    [super adopt: child];
    
    if ([child isKindOf: [Row class]]) {
	self->rows += 1;
    }
}

-(void) renounce: (Node *)child
{
    [super renounce: child];
    
    if ([child isKindOf: [Row class]]) {
	self->rows -= 1;
    }
}

-(void) prepare
{
    Widget *child;
    double delta;
	
    /* Now distribute the children in the allocated space. */

    for(child = (Widget *)self->down, delta = -0.5 * self->content[1];
	child;
	child = (Widget *)child->right) {

	if (![child isKindOf: [Widget class]]) {
	    continue;
	}

	delta += 0.5 * child->allocation[1];

	child->offset[0] = 0;
	child->offset[1] = delta;

	delta += 0.5 * child->allocation[1];
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

