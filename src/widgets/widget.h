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

#include "graphic.h"

@interface Widget: Graphic {
    double padding[4], allocation[2], content[2], align[2], offset[2];
    int debug;
}

-(void) arrange;
-(void) measure;

-(int) _get_align;
-(int) _get_padding;
-(int) _get_offset;
-(int) _get_content;
-(void) _set_align;
-(void) _set_padding;
-(void) _set_offset;
-(void) _set_content;

@end

#endif
