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

#ifndef _LAYOUT_H_
#define _LAYOUT_H_

#include <gdk/gdk.h>
#include "widget.h"
#include "shape.h"
#include "shader.h"

@interface LayoutShader: Shader {
@public
    int location, texture, reference_1;
}

@end

@interface Layout: Widget {
@public
    PangoLayout *layout;

    const char *text;
    unsigned int texture;
    double width, scale;
    int wrap, justify, gravity, indent, spacing, texels[2];

    Shape *shape;
    LayoutShader *shader;
}

-(void) redraw;
-(void) update;

-(int) _get_text;
-(int) _get_width;
-(int) _get_justify;
-(int) _get_gravity;
-(int) _get_indent;
-(int) _get_spacing;
-(int) _get_tabs;
-(int) _get_scale;

-(void) _set_text;
-(void) _set_width;
-(void) _set_justify;
-(void) _set_gravity;
-(void) _set_indent;
-(void) _set_spacing;
-(void) _set_tabs;
-(void) _set_scale;

@end

#endif
