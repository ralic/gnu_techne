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

#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <lua.h>
#include "transform.h"

@interface Graphics: Node {
}

-(id) initWithName: (char *)name andClass: (char *)class;
-(void) iterate: (Transform *)root;

-(int) _get_window;
-(int) _get_hide;
-(int) _get_title;
-(int) _get_decorate;
-(int) _get_grabinput;
-(int) _get_cursor;
-(int) _get_pointer;
-(int) _get_perspective;
-(int) _get_orthographic;	
-(int) _get_canvas;
-(int) _get_configure;	
-(int) _get_focus;	
-(int) _get_defocus;	
-(int) _get_close;
-(int) _get_renderer;
-(int) _get_screen;

-(void) _set_window;
-(void) _set_hide;
-(void) _set_title;
-(void) _set_decorate;
-(void) _set_grabinput;
-(void) _set_cursor;
-(void) _set_pointer;
-(void) _set_perspective;
-(void) _set_orthographic;
-(void) _set_canvas;
-(void) _set_configure;
-(void) _set_focus;
-(void) _set_defocus;
-(void) _set_close;
-(void) _set_renderer;
-(void) _set_screen;

@end

void t_set_projection (float *matrix);
void t_set_modelview (float *matrix);

#endif
