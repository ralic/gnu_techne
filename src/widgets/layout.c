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

#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>

#include "techne.h"
#include "layout.h"

static int poweroftwo;
static PangoContext *context;

@implementation Layout

+(void) initialize
{
    PangoFontMap *fontmap;

    fontmap = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(fontmap);
}

-(Layout *)init
{
    /* Get the configuration. */
    
    lua_getglobal (_L, "options");

    lua_getfield (_L, -1, "poweroftwo");
    poweroftwo = lua_toboolean (_L, -1);
    lua_pop (_L, 2);

    /* Initialize the node. */
    
    self = [super init];

    self->layout = pango_layout_new (context);

    self->texels[0] = 0;
    self->texels[1] = 0;

    self->allocated[0] = 0;
    self->allocated[1] = 0;

    self->opacity = 1;

    self->wrap = 0;
    self->gravity = -1;
    self->justify = 0;
    self->indent = 0;
    self->spacing = 0;
    self->scale = -1;
    
    glGenTextures(1, &self->texture);

    return self;
}

-(void) free
{
    glDeleteTextures(1, &self->texture);

    [super free];
}

-(double) measureWidth
{
    GLint v[4];

    if (self->scale > 0) {
	self->minimum[0] = self->scale * (double)self->texels[0] /
	                                 (double)self->texels[1];
    } else {
	glGetIntegerv(GL_VIEWPORT, v);
	self->minimum[0] = (double)self->texels[0] / v[3];
    }
    
    return self->minimum[0] + self->padding[0] + self->padding[1];
}

-(double) measureHeight
{
    GLint v[4];

    glGetIntegerv(GL_VIEWPORT, v);
    
    if (self->scale > 0) {
	self->minimum[1] = self->scale;
    } else {
	self->minimum[1] = (double)self->texels[1] / v[3];
    }
    
    return self->minimum[1] + self->padding[2] + self->padding[3];
}

-(void) update
{
    cairo_t *cairo;
    cairo_surface_t *surface;
	
    unsigned char *pixels, *source;
    int i, j, n;

    /* Lay out the text. */
	
    pango_layout_set_markup (layout, self->text, -1);
    pango_layout_get_pixel_size (self->layout,
				 &self->texels[0],
				 &self->texels[1]);
	
    /* Create a Cairo image surface. */
	
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					  self->texels[0],
					  self->texels[1]);

    cairo = cairo_create (surface);
    cairo_set_source_rgba (cairo, 0, 0, 0, 0);
    cairo_paint (cairo);

    /* Draw the layout. */
	
    pango_cairo_update_context (cairo, context);
    pango_cairo_show_layout (cairo, layout);

    /* Convert from pre-multiplied alpha BGRA to RGBA. */
	
    source = cairo_image_surface_get_data (surface);
    n = cairo_image_surface_get_stride (surface);
    pixels = (unsigned char *) malloc (self->texels[0] * self->texels[1] *
				       4 * sizeof (unsigned char));
	
    for (j = 0 ; j < self->texels[1] ; j += 1) {
	for (i = 0 ; i < self->texels[0] ; i += 1) {
	    unsigned char *p, *q;

	    p = source + (self->texels[1] - j - 1) * n + 4 * i;
	    q = pixels + 4 * (j * self->texels[0] + i);

	    q[0] = p[3] > 0 ? ((double)p[2] / p[3]) * 255 : 0;
	    q[1] = p[3] > 0 ? ((double)p[1] / p[3]) * 255 : 0;
	    q[2] = p[3] > 0 ? ((double)p[0] / p[3]) * 255 : 0;
	    q[3] = p[3];
	}
    }
	
    /* Load the texture. */
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, self->texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, self->texels[0]);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (poweroftwo /* && _WINDOW */) {
	gluBuild2DMipmaps (GL_TEXTURE_2D,
			   GL_RGBA,
			   self->texels[0],
			   self->texels[1],
			   GL_RGBA,
			   GL_UNSIGNED_BYTE,
			   pixels);
    } else {
	glTexImage2D(GL_TEXTURE_2D, 0,
		     GL_RGBA,
		     self->texels[0],
		     self->texels[1],
		     0,
		     GL_RGBA,
		     GL_UNSIGNED_BYTE,
		     pixels);
    }
	
    /* Clean up. */
	
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);

    free (pixels);
}

-(void) traverse
{
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
    glMultMatrixd (self->matrix);
	
    glColor4d (1, 1, 1, self->opacity);
    glUseProgramObjectARB(0);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, self->texture);

    glDepthMask (GL_FALSE);

    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    [self place];
    
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex2f(-0.5 * self->minimum[0], -0.5 * self->minimum[1]);
    glTexCoord2f(1, 0);
    glVertex2f(0.5 * self->minimum[0], -0.5 * self->minimum[1]);
    glTexCoord2f(1, 1);
    glVertex2f(0.5 * self->minimum[0], 0.5 * self->minimum[1]);
    glTexCoord2f(0, 1);
    glVertex2f(-0.5 * self->minimum[0], 0.5 * self->minimum[1]);
    glEnd();

    glDepthMask (GL_TRUE);

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    [super traverse];
    
    glMatrixMode (GL_MODELVIEW);
    glPopMatrix();
}

-(int) _get_text
{
    lua_pushstring (_L, self->text);

    return 1;
}

-(int) _get_width
{
    if (self->wrap) {
	lua_pushnumber (_L, self->width);
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_justify
{
    lua_pushboolean (_L, self->justify);

    return 1;
}

-(int) _get_gravity
{
    lua_pushnumber (_L, self->gravity);

    return 1;
}

-(int) _get_indent
{
    lua_pushnumber (_L, self->indent);

    return 1;
}

-(int) _get_spacing
{
    lua_pushnumber (_L, self->spacing);	

    return 1;
}

-(int) _get_tabs
{
    lua_getmetatable (_L, 1);
    lua_replace (_L, 1);
    lua_gettable (_L, 1);

    return 1;
}

-(int) _get_scale
{
    if (self->scale > 0) {
	lua_pushnumber (_L, self->scale);
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(void) _set_text
{
    self->text = lua_tostring (_L, 3);

    [self update];
}

-(void) _set_width
{
    if (lua_isnumber (_L, 3)) {
	self->width = lua_tonumber (_L, 3);
	
	pango_layout_set_width (self->layout, self->width * PANGO_SCALE);
	/* pango_layout_set_wrap (self->layout, PANGO_WRAP_WORD_CHAR); */
	pango_layout_set_ellipsize (self->layout, PANGO_ELLIPSIZE_START);
	    
	self->wrap = 1;
    } else {
	pango_layout_set_width (self->layout, -1);
	    
	self->wrap = 0;
    }

    [self update];
}

-(void) _set_justify
{
    self->justify = lua_toboolean (_L, 3);

    pango_layout_set_justify (self->layout, self->justify ? TRUE : FALSE);

    [self update];
}

-(void) _set_gravity
{
    self->gravity = lua_tonumber (_L, 3);

    if (self->gravity > 0) {
	pango_layout_set_alignment (self->layout, PANGO_ALIGN_RIGHT);
    } else if (self->gravity < 0) {
	pango_layout_set_alignment (self->layout, PANGO_ALIGN_LEFT);
    } else {
	pango_layout_set_alignment (self->layout, PANGO_ALIGN_CENTER);
    }

    [self update];
}

-(void) _set_indent
{
    self->indent = lua_tonumber (_L, 3);

    pango_layout_set_indent (self->layout, self->indent * PANGO_SCALE);

    [self update];
}

-(void) _set_spacing
{
    self->spacing = lua_tonumber (_L, 3);

    pango_layout_set_spacing (self->layout, self->spacing * PANGO_SCALE);

    [self update];
}

-(void) _set_tabs
{
    PangoTabArray *array;
    int i, n;
	
    if(lua_istable (_L, 3)) {
	lua_len (_L, 3);
	n = lua_tointeger (_L, -1);
	lua_pop (_L, 1);
	    
	array = pango_tab_array_new (n, TRUE);
	    
	for(i = 0 ; i < n ; i += 1) {
	    lua_rawgeti (_L, 3, i + 1);
	    pango_tab_array_set_tab (array, i, PANGO_TAB_LEFT,
				     lua_tointeger (_L, -1));
                
	    lua_pop (_L, 1);
	}

	pango_layout_set_tabs (self->layout, array);
	pango_tab_array_free (array);
    } else {
	pango_layout_set_tabs (self->layout, NULL);
    }

    [self update];
	
    lua_getmetatable (_L, 1);
    lua_replace (_L, 1);
    lua_settable (_L, 1);
}

-(void) _set_scale
{
    if (lua_isnumber(_L, 3)) {
	self->scale = lua_tonumber (_L, 3);
    } else {
	self->scale = -1;
    }
}

-(int) _get_opacity
{
    lua_pushnumber (_L, self->opacity);

    return 1;
}

-(void) _set_opacity
{
    self->opacity = lua_tonumber (_L, 3);
}

@end


