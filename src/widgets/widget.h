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

#ifndef _WIDGET_H_
#define _WIDGET_H_

#include "transform.h"

@interface Widget: Transform {
    double padding[4], allocated[2], minimum[2], align[2], offset[2];
    int debug;
}

-(double) measureWidth;
-(double) measureHeight;
-(void) place;

-(int) _get_align;
-(int) _get_padding;
-(void) _set_align;
-(void) _set_padding;

@end

#endif
