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
#include <math.h>

#include <lua.h>
#include <lauxlib.h>

#include "gl.h"

#include "techne.h"
#include "shader.h"
#include "atmosphere.h"

static Atmosphere *instance;
static ShaderMold *handle;
static int first[32], count[32];

static double lambda[] = {0.38, 0.39, 0.40, 0.41, 0.42, 0.43, 0.44, 0.45, 0.46,
			  0.47, 0.48, 0.49, 0.50, 0.51, 0.52, 0.53, 0.54, 0.55,
			  0.56, 0.57, 0.58, 0.59, 0.60, 0.61, 0.62, 0.63, 0.64,
			  0.65, 0.66, 0.67, 0.68, 0.69, 0.70, 0.71, 0.72, 0.73,
			  0.74, 0.75, 0.76, 0.77, 0.78};

static double S[] = {1655.9, 1623.37, 2112.75, 2588.82, 2582.91, 2423.23,
		     2676.05, 2965.83, 3054.54, 3005.75, 3066.37, 2883.04,
		     2871.21, 2782.5, 2710.06, 2723.36, 2636.13, 2550.38,
		     2506.02, 2531.16, 2535.59, 2513.42, 2463.15, 2417.32,
		     2368.53, 2321.21, 2282.77, 2233.98, 2197.02, 2152.67,
		     2109.79, 2072.83, 2024.04, 1987.08, 1942.72, 1907.24,
		     1862.89, 1825.92, 0, 0, 0};

static double k_o[] = {0, 0, 0, 0, 0, 0, 0, 0.003, 0.006, 0.009, 0.014, 0.021,
		       0.03, 0.04, 0.048, 0.063, 0.075, 0.085, 0.103, 0.12,
		       0.12, 0.115, 0.125, 0.12, 0.105, 0.09, 0.079, 0.067,
		       0.057, 0.048, 0.036, 0.028, 0.023, 0.018, 0.014, 0.011,
		       0.01, 0.009, 0.007, 0.004, 0};

static double k_wa[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.016, 0.024,
			0.0125, 1, 0.87, 0.061, 0.001, 1e-05, 1e-05, 0.0006};

static double k_g[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		       3.0, 0.21, 0};

static double w_X[] = {0.001368, 0.004243, 0.014310, 0.043510, 0.134380,
		       0.283900, 0.348280, 0.336200, 0.290800, 0.195360,
		       0.095640, 0.032010, 0.004900, 0.009300, 0.063270,
		       0.165500, 0.290400, 0.433450, 0.594500, 0.762100,
		       0.916300, 1.026300, 1.062200, 1.002600, 0.854450,
		       0.642400, 0.447900, 0.283500, 0.164900, 0.087400,
		       0.046770, 0.022700, 0.011359, 0.005790, 0.002899,
		       0.001440, 0.000690, 0.000332, 0.000166, 0.000083,
		       0.000042};

static double w_Y[] = {0.000039, 0.000120, 0.000396, 0.001210, 0.004000,
		       0.011600, 0.023000, 0.038000, 0.060000, 0.090980,
		       0.139020, 0.208020, 0.323000, 0.503000, 0.710000,
		       0.862000, 0.954000, 0.994950, 0.995000, 0.952000,
		       0.870000, 0.757000, 0.631000, 0.503000, 0.381000,
		       0.265000, 0.175000, 0.107000, 0.061000, 0.032000,
		       0.017000, 0.008210, 0.004102, 0.002091, 0.001047,
		       0.000520, 0.000249, 0.000120, 0.000060, 0.000030,
		       0.000015};

static double w_Z[] = {0.006450, 0.020050, 0.067850, 0.207400, 0.645600,
		       1.385600, 1.747060, 1.772110, 1.669200, 1.287640,
		       0.812950, 0.465180, 0.272000, 0.158200, 0.078250,
		       0.042160, 0.020300, 0.008750, 0.003900, 0.002100,
		       0.001650, 0.001100, 0.000800, 0.000340, 0.000190,
		       0.000050, 0.000020, 0.000000, 0.000000, 0.000000,
		       0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
		       0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
		       0.000000};

static double alpha = 1.3, l = 0.35, w = 2;

static void calculate_sun_color(double elevation,
				double turbidity,
				float *color)
{
    double S_1[41], tau_r[41], tau_alpha[41], tau_o[41], tau_g[41], tau_wa[41];
    double beta, m, X, Y, Z;
    int i;
    
    beta = 0.04608 * turbidity - 0.04586;
    m = 1 / (cos(M_PI_2 - elevation) +
	     0.15 * pow(93.885 - elevation + M_PI_2 * 180 / M_PI, -1.253));

    for (i = 0 ; i < 41 ; i += 1) {
	tau_r[i] = exp (-0.008735 * m * pow(lambda[i], -4.08));
	tau_alpha[i] = exp (-beta * m * pow(lambda[i], -alpha));
	tau_o[i] = exp (-k_o[i] * l * m);
	tau_g[i] = exp (-1.41 * k_g[i] * m /
			pow(1 + 118.93 * k_g[i] * m, 0.45));
	tau_wa[i] = exp (-0.2385 * k_wa[i] * w * m /
		      pow(1 + 20.07 * k_wa[i] * w * m, 0.45));
    }
    
    for (i = 0 ; i < 41 ; i += 1) {
	S_1[i] = S[i] * tau_r[i] * tau_alpha[i] *
	         tau_o[i] * tau_g[i] * tau_wa[i];
    }

    for (X = 0, Y = 0, Z = 0, i = 0;
	 i < 41;
	 X += S_1[i] * w_X[i],
	 Y += S_1[i] * w_Y[i],
	 Z += S_1[i] * w_Z[i],
	 i += 1);
    
    color[0] =  1.4511e-04 * X - 7.3584e-05 * Y - 2.2423e-05 * Z;
    color[1] = -4.4470e-05 * X + 8.7259e-05 * Y + 1.6177e-06 * Z;
    color[2] =  2.6497e-06 * X - 9.3143e-06 * Y + 5.4090e-05 * Z;
    
    /* color[0] = color[1] = color[2] = Y / 2.1915e04; */
}

static double perez(double *c,
		    double theta, double gamma,
		    double costheta, double cosgamma) {
    return (1.0 + c[0] * exp(c[1] / costheta)) *
	(1.0 + c[2] * exp(c[3] * gamma) + c[4] * cosgamma * cosgamma);
}

static void calculate_sky_color(double azimuth, double elevation,
				double turbidity,
				int width, int height, unsigned char *pixels)
{
    double T, T2, theta_s, theta2_s, theta3_s, phi_s, theta, phi, gamma, chi;
    double costheta, sintheta, cosphi, sinphi, cosgamma;
    double costheta_s, sintheta_s, cosphi_s, sinphi_s;
    double c_Y[5], c_x[5], c_y[5];
    double Y_z, x_z, y_z, Y_n, Y_0, *Y, *x, *y, X, Z;
    double F_x0, F_y0, F_Y0;
    int i, j;

    T = turbidity;
    phi_s = azimuth;
    theta_s = M_PI_2 - elevation;

    costheta_s = cos(theta_s);
    sintheta_s = sin(theta_s);
    cosphi_s = cos(phi_s);
    sinphi_s = sin(phi_s);

    T2 = T * T;
    theta2_s = theta_s * theta_s;
    theta3_s = theta2_s * theta_s;
    chi = (4.0 / 9.0 - T / 120.0) * (M_PI - 2.0 * theta_s);

    Y_z = (4.0453 * T - 4.9710) * tan(chi) - 0.2155 * T + 2.4192;

    x_z = T2 * (0.00166 * theta3_s - 0.00375 * theta2_s + 0.00209 * theta_s) +
	T * (-0.02903 * theta3_s + 0.06377 * theta2_s - 
	     0.03202 * theta_s + 0.00394) +
	0.11693 * theta3_s - 0.21196 * theta2_s +0.06052 * theta_s + 0.25886;


    y_z = T2 * (0.00275 * theta3_s - 0.0061 * theta2_s + 0.00317 * theta_s) +
	T * (-0.04214 * theta3_s + 0.0897 * theta2_s
	     -0.04153 * theta_s + 0.00516) +
	0.15346 * theta3_s - 0.26756 * theta2_s + 0.0667 * theta_s + 0.26688;

    c_Y[0] =  0.1787 * T - 1.4630;
    c_Y[1] = -0.3554 * T + 0.4275;
    c_Y[2] = -0.0227 * T + 5.3251;
    c_Y[3] =  0.1206 * T - 2.5771;
    c_Y[4] = -0.0670 * T + 0.3703;

    c_x[0] = -0.0193 * T - 0.2592;
    c_x[1] = -0.0665 * T + 0.0008;
    c_x[2] = -0.0004 * T + 0.2125;
    c_x[3] = -0.0641 * T - 0.8989;
    c_x[4] = -0.0033 * T + 0.0452;

    c_y[0] = -0.0167 * T - 0.2608;
    c_y[1] = -0.0950 * T + 0.0092;
    c_y[2] = -0.0079 * T + 0.2102;
    c_y[3] = -0.0441 * T - 1.6537;
    c_y[4] = -0.0109 * T + 0.0529;

    F_Y0 = perez(c_Y, 0.0, theta_s, 1.0, costheta_s);
    F_x0 = perez(c_x, 0.0, theta_s, 1.0, costheta_s);
    F_y0 = perez(c_y, 0.0, theta_s, 1.0, costheta_s);

    Y = (double *)calloc(height * width, sizeof(double));
    x = (double *)calloc(height * width, sizeof(double));
    y = (double *)calloc(height * width, sizeof(double));

    Y_0 = 0;

    for(i = 0 ; i < height ; i += 1) {
	for(j = 0 ; j < width ; j += 1) {
	    theta = ((double)i / height) * M_PI_2;
	    phi = 2 * ((double)j / width) * M_PI;

	    costheta = cos(theta);
	    sintheta = sin(theta);
	    cosphi = cos(phi);
	    sinphi = sin(phi);

	    cosgamma = sintheta * cosphi * sintheta_s * cosphi_s +
		sintheta * sinphi * sintheta_s * sinphi_s +
		costheta * costheta_s;
	    gamma = acos(cosgamma);

	    Y[i * width + j] = Y_z *
		perez(c_Y, theta, gamma, costheta, cosgamma) / F_Y0;

	    x[i * width + j] = x_z *
		perez(c_x, theta, gamma, costheta, cosgamma) / F_x0;
	    
	    y[i * width + j] = y_z *
		perez(c_y, theta, gamma, costheta, cosgamma) / F_y0;

	    if (Y_0 < Y[i * width + j]) {
		Y_0 = Y[i * width + j];
	    }
	}
    }

    Y_0 = 1 / log(1 + Y_0);
    
    for(i = 0 ; i < width * height ; i += 1) {
	double R, G, B;

	/* Y_n = Y_0 * log(1 + Y[i]); */
	Y_n = 1 - exp((0.05 * sintheta_s - 0.5) * Y[i]);
	X = x[i] / y[i] * Y_n;
	Z = (1 - x[i] - y[i]) / y[i] * Y_n;

	R =  3.177035 * X - 1.611037 * Y_n - 0.490932 * Z;
	G = -0.973620 * X + 1.910416 * Y_n + 0.035418 * Z;
	B =  0.058011 * X - 0.203926 * Y_n + 1.184237 * Z;

	pixels[3 * i + 0] = R > 1 ? 255 : lrint(R * 255.0);
	pixels[3 * i + 1] = G > 1 ? 255 : lrint(G * 255.0);
	pixels[3 * i + 2] = B > 1 ? 255 : lrint(B * 255.0);
    }
}

@implementation Atmosphere

+(Atmosphere *) instance
{
    return instance;
}

-(void) init
{
#include "glsl/textured_vertex.h"	
#include "glsl/textured_fragment.h"	

    const char *private[1] = {"texture"};
    int i;

    [super init];
    
    /* If this is the first instance create the program. */

    if (!handle) {
        ShaderMold *shader;
        
	shader = [ShaderMold alloc];
        
        [shader initWithHandle: &handle];
        [shader declare: 1 privateUniforms: private];
	[shader addSource: glsl_textured_vertex for: T_VERTEX_STAGE];
	[shader addSource: glsl_textured_fragment for: T_FRAGMENT_STAGE];
	[shader link];
    } else {
        t_pushuserdata(_L, 1, handle);
    }
    
    [self load];

    /* Initialize the object. */

    self->index = 1;
    
    self->size[0] = 0;
    self->size[1] = 0;

    self->azimuth = 0;
    self->elevation = M_PI / 4;

    self->turbidity = 3;

    self->rayleigh[0] = 0.012 * 6.95e-06;
    self->rayleigh[1] = 0.012 * 1.18e-05;
    self->rayleigh[2] = 0.012 * 2.44e-05;

    self->mie = 5e-5;
    self->dirty = 0;

    glGenTextures(1, &self->skylight);

    /* Bind the sky texture to the first unit. */
    
    i = glGetUniformLocation (self->name, "texture");

    glUseProgram (self->name);
    glUniform1i (i, 0);

    /* Create the shape node. */
    
    lua_pushstring(_L, "shape");
    [[AtmosphereShape alloc] init];
    lua_settable (_L, -3);
}

-(void) toggle
{
    [super toggle];

    if (self->linked) {
        if (instance) {
            t_print_error ("Only one Atmosphere node should be linked to "
                           "the scene.\n");
            abort();
        }

        instance = self;
    } else {
        assert (instance == self);
        instance = NULL;
    }
}

-(int) _get_sun
{
    lua_newtable(_L);
        
    lua_pushnumber(_L, self->azimuth);
    lua_rawseti(_L, -2, 1);
        
    lua_pushnumber(_L, self->elevation);
    lua_rawseti(_L, -2, 2);

    return 1;
}

-(int) _get_intensity
{
    int i;
    
    lua_newtable(_L);
        
    for(i = 0; i < 3; i += 1) {
        lua_pushnumber(_L, self->intensity[i]);
        lua_rawseti(_L, -2, i + 1);
    }	

    return 1;
}

-(int) _get_size
{
    int i;
    
    lua_newtable(_L);
        
    for(i = 0; i < 2; i += 1) {
        lua_pushnumber(_L, self->size[i]);
        lua_rawseti(_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_rayleigh
{
    int i;
    
    lua_newtable(_L);
        
    for(i = 0; i < 3; i += 1) {
        lua_pushnumber(_L, self->rayleigh[i]);
        lua_rawseti(_L, -2, i + 1);
    }

    return 1;
}

-(int) _get_mie
{
    lua_pushnumber(_L, self->mie);

    return 1;
}

-(int) _get_turbidity
{
    lua_pushnumber (_L, self->turbidity);

    return 1;
}

-(void) _set_sun
{
    if(lua_istable(_L, 3)) {
        lua_rawgeti(_L, 3, 1);
        self->azimuth = lua_tonumber(_L, -1);
        lua_pop(_L, 1);

        lua_rawgeti(_L, 3, 2);
        self->elevation = lua_tonumber(_L, -1);
        lua_pop(_L, 1);

        self->dirty = 1;
    }
}

-(void) _set_intensity
{
    int i;
    
    if (lua_istable(_L, 3)) {
        for(i = 0 ; i < 3 ; i += 1) {
            lua_rawgeti(_L, 3, i + 1);
            self->intensity[i] = lua_tonumber(_L, -1);
                
            lua_pop(_L, 1);
        }

        self->explicit = 1;
    } else {
        self->explicit = 0;
    }
}

-(void) _set_size
{
    int i;
    
    if(lua_istable(_L, 3)) {
        for(i = 0 ; i < 2 ; i += 1) {
            lua_rawgeti(_L, 3, i + 1);
            self->size[i] = lua_tonumber(_L, -1);
                
            lua_pop(_L, 1);
        }

        self->dirty = 1;
    }
}

-(void) _set_rayleigh
{
    int i;
    
    if(lua_istable(_L, 3)) {
        for(i = 0 ; i < 3 ; i += 1) {
            lua_rawgeti(_L, 3, i + 1);
            self->rayleigh[i] = lua_tonumber(_L, -1);
                
            lua_pop(_L, 1);
        }

        self->dirty = 1;
    }
}

-(void) _set_mie
{
    self->mie = lua_tonumber(_L, 3);
    self->dirty = 1;
}

-(void) _set_turbidity
{
    self->turbidity = lua_tonumber (_L, -1);
    self->dirty = 1;
}

-(void) free
{
    glDeleteTextures (1, &self->skylight);

    [super free];
}

-(void) draw: (int)frame
{
    float M[16];
    float phi, theta_0, x, y, z;

    /* Update the texture if needed. */
    
    if (self->dirty) {
        unsigned char *pixels;

        pixels = (unsigned char *)calloc(self->size[0] * self->size[1],
                                         3 * sizeof(unsigned char));
    
        calculate_sky_color(self->azimuth, self->elevation,
                            self->turbidity, self->size[0], self->size[1], pixels);

        if (!self->explicit) {
            calculate_sun_color(self->elevation, self->turbidity, self->intensity);
        }

        /* Load the texture.*/
    
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, self->skylight);

        glTexImage2D(GL_TEXTURE_2D, 0,
                     GL_RGB,
                     self->size[0],
                     self->size[1],
                     0, GL_RGB,
                     GL_UNSIGNED_BYTE,
                     pixels);
	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
        free(pixels);
        
        self->dirty = 0;
    }
    
    /* Calculate the sunlight direction. */
    
    t_push_modelview (self->matrix, T_MULTIPLY);
    t_copy_modelview (M);

    phi = self->azimuth;
    theta_0 = M_PI_2 - self->elevation;

    x = cos (phi) * sin (theta_0);
    y = sin (phi) * sin (theta_0);
    z = cos (theta_0);
		
    self->direction[0] = x * M[0] + y * M[4] + z * M[8];
    self->direction[1] = x * M[1] + y * M[5] + z * M[9];
    self->direction[2] = x * M[2] + y * M[6] + z * M[10];

    t_pop_modelview ();

    /* Bind resources and draw. */
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, self->skylight);    
    
    glUseProgram(self->name);

    glStencilMask(0);
    glDepthMask(GL_FALSE);
    glBlendFunc (GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
    
    glEnable(GL_CULL_FACE);
    glEnable (GL_BLEND);

    [super draw: frame];

    glDisable(GL_CULL_FACE);
    glDisable (GL_BLEND);

    glDepthMask(GL_TRUE);
    glStencilMask(~0);
}

@end

@implementation AtmosphereShape

-(void) init
{
    double theta_0, theta_1, phi;
    double dtheta, dphi;
    float *vertices, *uv;
    int i, j, l, k;

    [super initWithMode: GL_TRIANGLE_STRIP];

    l = 32 * 33 * 2 * sizeof(float);

    vertices = (float *)malloc (3 * l);
    uv = (float *)malloc (2 * l);
    
    /* Draw a hemisphere.  The radius is quite arbitrary,
       as long as the sphere lies completely inside the
       frustum. The best choice therefore is the far plane. */

    dtheta = 1.0 / 32.0;
    dphi = 1.0 / 32.0;
   
    for(j = 0 ; j < 32 ; j += 1) {
        first[j] = 33 * 2 * j;
        count[j] = 33 * 2;
	
        theta_0 = j * M_PI * dtheta;
        theta_1 = (j + 1) * M_PI * dtheta;

        for(i = 0 ; i <= 32 ; i += 1) {
            k = j * 33 + i;
            phi = -2 * i * M_PI * dphi;

            uv[4 * k] = i * dphi + 0.5;
            uv[4 * k + 1] = 2 * j * dtheta;

            vertices[6 * k] = cos(phi) * sin(theta_0);
            vertices[6 * k + 1] = sin(phi) * sin(theta_0);
            vertices[6 * k + 2] = cos(theta_0);

            uv[4 * k + 2] = i * dphi + 0.5;
            uv[4 * k + 3] = 2 * (j + 1) * dtheta;

            vertices[6 * k + 3] = cos(phi) * sin(theta_1);
            vertices[6 * k + 4] = sin(phi) * sin(theta_1);
            vertices[6 * k + 5] = cos(theta_1);
        }
    }
    
    /* Create the VBOs.  Positions. */
    
    glGenBuffers(1, &self->positions);
    glBindBuffer (GL_ARRAY_BUFFER, self->positions);
    glBufferData (GL_ARRAY_BUFFER, 3 * l, vertices, GL_STATIC_DRAW);

    /* Texture coordinates. */
    
    glGenBuffers(1, &self->mapping);
    glBindBuffer (GL_ARRAY_BUFFER, self->mapping);
    glBufferData (GL_ARRAY_BUFFER, 2 * l, uv, GL_STATIC_DRAW);

    free (vertices);
    free (uv);
}

-(void) free
{
    glDeleteBuffers (1, &self->positions);
    glDeleteBuffers (1, &self->mapping);

    [super free];
}

-(void) meetParent: (Shader *)parent
{
    int i;

    [super meetParent: parent];

    /* Bind the VBOs into the VAO. */
    
    glBindVertexArray(self->name);

    i = glGetAttribLocation(parent->name, "positions");
    glBindBuffer(GL_ARRAY_BUFFER, self->positions);
    glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(i);

    i = glGetAttribLocation(parent->name, "mapping");
    glBindBuffer(GL_ARRAY_BUFFER, self->mapping);
    glVertexAttribPointer(i, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(i);
}

-(void) draw: (int)frame
{
    float M[16], P[16];
    double rho;
    
    [super draw: frame];
    
    t_copy_projection (P);
    rho = 0.9 * P[14] / (P[10] + 1.0);

    t_push_modelview (self->matrix, T_MULTIPLY);
    t_copy_modelview (M);

    /* Make sure the sky dome is centered at {0, 0, 0}. */

    M[0] *= rho; M[1] *= rho; M[2] *= rho;
    M[4] *= rho; M[5] *= rho; M[6] *= rho;
    M[8] *= rho; M[9] *= rho; M[10] *= rho;
    M[12] = M[13] = M[14] = 0.0;
    t_load_modelview (M, T_LOAD);

    glBindVertexArray(self->name);
    glMultiDrawArrays (self->mode, first, count, 32);

    t_pop_modelview ();
}

@end
