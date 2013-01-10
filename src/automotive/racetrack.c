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
#include "gl.h"

#include "techne.h"
#include "racetrack.h"
#include "ground.h"
#include "wheel.h"

static int constructbody(lua_State *L)
{
    Racetrack *track;
    RacetrackBody *body;
    
    track = (Racetrack *)t_tonode (L, lua_upvalueindex(1));
    body = [RacetrackBody alloc];
    [body initWith: track->segments_n
     segments: track->segments
     andTolerance: track->tolerance];

    t_configurenode(L, 1);

    return 1;
}

static int constructshape(lua_State *L)
{
    Racetrack *track;
    RacetrackShape *shape;
    
    track = (Racetrack *)t_tonode (L, lua_upvalueindex(1));
    shape = [RacetrackShape alloc];
    [shape initWith: track->segments_n
     segments: track->segments
     andTolerance: track->tolerance];

    t_configurenode(L, 1);

    return 1;
}

static void update_track_segments(double *segments, int segments_n,
                                  double tolerance)
                   
{
    double theta_0, p_0[3], delta_wl, delta_wr, w_l0, w_r0;
    double S, kappa_0, delta_kappa, g_0, delta_g, e_0, delta_e;
    int i, j;

    theta_0 = 0;
    p_0[0] = 0;
    p_0[1] = 0;
    p_0[2] = 0;

    w_l0 = segments[1];
    w_r0 = segments[2];
    kappa_0 = segments[3];
    g_0 = segments[4];
    e_0 = segments[5];
    
    /* printf ("%f, %f, %f\n", 0, w_r0, e_0 * w_r0); */
    /* printf ("%f, %f, %f\n", 0, -w_l0, -e_0 * w_l0); */

    for (i = 0 ; i < segments_n ; i += 1) {
        /* Fill in the initial position and angle for each segment. */
        
	segments[10 * i + 6] = p_0[0];
	segments[10 * i + 7] = p_0[1];
	segments[10 * i + 8] = p_0[2];
	segments[10 * i + 9] = theta_0;

        /* printf ("%f, %f, %f, %f\n", */
        /*         segments[10 * i + 6], */
        /*         segments[10 * i + 7], */
        /*         segments[10 * i + 8], */
        /*         segments[10 * i + 9]); */
        
	S = segments[10 * i];
	delta_wl = segments[10 * i + 1] - w_l0;
	delta_wr = segments[10 * i + 2] - w_r0;
	delta_kappa = segments[10 * i + 3] - kappa_0;
	delta_g = segments[10 * i + 4] - g_0;
	delta_e = segments[10 * i + 5] - e_0;
        
        /* Figure out what the next segment is based on current and
         * previous curvature. */
        
	if (fabs(kappa_0) < 1e-6 && fabs(delta_kappa) < 1e-6) {
	    /* A tangent. */

            e_0 += delta_e;
            w_l0 += delta_wl;
            w_r0 += delta_wr;
		    
	    p_0[0] += S * cos(theta_0);
	    p_0[1] += S * sin(theta_0);
	    p_0[2] += S * g_0 + 0.5 * delta_g * S;
	} else if (fabs(kappa_0) > 1e-6 && fabs(delta_kappa) < 1e-6) {
	    double r, c[2];

            /* A circular segment. */
            
	    r = 1 / kappa_0;
		
	    c[0] = p_0[0] - r * sin(theta_0);
	    c[1] = p_0[1] + r * cos(theta_0);
		    
            e_0 += delta_e;
            w_l0 += delta_wl;
            w_r0 += delta_wr;
		
	    theta_0 += S / r;
            
	    p_0[0] = c[0] + r * sin(theta_0);
	    p_0[1] = c[1] - r * cos(theta_0);
	    p_0[2] += S * g_0 + 0.5 * delta_g * S;
	} else {
	    double r, c[2];
	    int m;

            /* A transitional curve. */
            
	    m = (int)fmax(ceil(fabs(delta_kappa) / tolerance), 1);

	    /* printf ("* %d, %d, %d\n", i, m, n); */
		
	    for (j = 0; j < m ; j += 1) {

		kappa_0 += delta_kappa / (m + 1);
		r = 1 / kappa_0;
		
		c[0] = p_0[0] - r * sin(theta_0);
		c[1] = p_0[1] + r * cos(theta_0);

                e_0 += delta_e / m;
                w_l0 += delta_wl / m;
                w_r0 += delta_wr / m;

		theta_0 += S / r / m;
		
		p_0[0] = c[0] + r * sin(theta_0);
		p_0[1] = c[1] - r * cos(theta_0);
	    }

	    p_0[2] += S * g_0 + 0.5 * delta_g * S;
	}

	w_l0 = segments[10 * i + 1];
	w_r0 = segments[10 * i + 2];
	kappa_0 = segments[10 * i + 3];
	g_0 = segments[10 * i + 4];
	e_0 = segments[10 * i + 5];
    }
}

static void update_track_shape(double *segments, int segments_n, double tolerance,
                               double *tessellation, double *scale,
                               float **vertices, float **normals, float **uv,
                               int *vertices_n)
                   
{
    float *V, *N, *UV;
    double theta_0, p_0[3], delta_wl, delta_wr, w_l0, w_r0, u_0;
    double S, kappa_0, delta_kappa, g_0, delta_g, e_0, delta_e;
    double phi, psi;
    int i, j, n;

    u_0 = 0;

    theta_0 = 0;
    p_0[0] = 0;
    p_0[1] = 0;
    p_0[2] = 0;

    w_l0 = segments[1];
    w_r0 = segments[2];
    kappa_0 = segments[3];
    g_0 = segments[4];
    e_0 = segments[5];
    
    phi = atan(e_0);
    psi = -atan(g_0);

    V = malloc (2 * 3 * sizeof (float));
    N = malloc (2 * 3 * sizeof (float));
    UV = malloc (2 * 2 * sizeof (float));
    n = 0;

    /* Kickstart the strip. */

    V[0] = 0;
    V[1] = w_r0;
    V[2] = e_0 * w_r0;

    V[3] = 0;
    V[4] = -w_l0;
    V[5] = -e_0 * w_l0;

    N[0] = cos(phi) * sin (psi);
    N[1] = -sin(phi);
    N[2] = cos(phi) * cos(psi);
    
    N[3] = N[0];
    N[4] = N[1];
    N[5] = N[2];

    UV[0] = 0;
    UV[1] = w_r0 / scale[1];
    
    UV[2] = 0;
    UV[3] = -w_l0 / scale[1];
    
    n += 2;
    
    /* printf ("%f, %f, %f\n", 0, w_r0, e_0 * w_r0); */
    /* printf ("%f, %f, %f\n", 0, -w_l0, -e_0 * w_l0); */

    for (i = 0 ; i < segments_n ; i += 1) {
	segments[10 * i + 6] = p_0[0];
	segments[10 * i + 7] = p_0[1];
	segments[10 * i + 8] = p_0[2];
	segments[10 * i + 9] = theta_0;

        /* printf ("%f, %f, %f, %f\n", */
        /*         segments[10 * i + 6], */
        /*         segments[10 * i + 7], */
        /*         segments[10 * i + 8], */
        /*         segments[10 * i + 9]); */
        
	S = segments[10 * i];
	delta_wl = segments[10 * i + 1] - w_l0;
	delta_wr = segments[10 * i + 2] - w_r0;
	delta_kappa = segments[10 * i + 3] - kappa_0;
	delta_g = segments[10 * i + 4] - g_0;
	delta_e = segments[10 * i + 5] - e_0;

	if (fabs(kappa_0) < 1e-6 && fabs(delta_kappa) < 1e-6) {
	    double dv[2], u;
	    int m;

            m = fmax(ceil (sqrt(fabs (delta_g) * S / 8 / tessellation[1])), 1);

	    /* printf ("++ %d, %f, %f\n", i, e_0, delta_e); */
		
	    /* A tangent. */

	    dv[0] = -sin(theta_0);
	    dv[1] = cos(theta_0);

	    for (j = 1 ; j <= m ; j += 1) {
		double h, p[2];

		u = S * j / m;
		h = u * g_0 + 0.5 * delta_g * u * u / S;
		
		p[0] = p_0[0] + S * j / m * cos(theta_0);
		p[1] = p_0[1] + S * j / m * sin(theta_0);

		e_0 += delta_e / m;
		w_l0 += delta_wl / m;
		w_r0 += delta_wr / m;

		phi = atan(e_0);
		psi = -atan(g_0 + delta_g * u / S);

                V = realloc (V, (n + 2) * 3 * sizeof (float));
		
                N = realloc (N, (n + 2) * 3 * sizeof (float));
		
                UV = realloc (UV, (n + 2) * 2 * sizeof (float));
		
                V[3 * n] = p[0] + w_r0 * dv[0];
                V[3 * n + 1] = p[1] + w_r0 * dv[1];
                V[3 * n + 2] = p_0[2] + h + e_0 * w_r0;

                V[3 * n + 3] = p[0] - w_l0 * dv[0];
                V[3 * n + 4] = p[1] - w_l0 * dv[1];
                V[3 * n + 5] = p_0[2] + h - e_0 * w_l0;

                N[3 * n + 0] =
                    sin(theta_0) * sin(phi) +
                    cos(phi) * sin (psi) * cos (theta_0);
                N[3 * n + 1] =
                    -cos(theta_0) * sin(phi) +
                    cos(phi) * sin (psi) * sin (theta_0);
                N[3 * n + 2] =
                    cos(phi) * cos(psi);

                N[3 * n + 3] = N[3 * n + 0];
                N[3 * n + 4] = N[3 * n + 1];
                N[3 * n + 5] = N[3 * n + 2];

                UV[2 * n] = (u_0 + u) / scale[0];
                UV[2 * n + 1] = w_r0 / scale[1];
    
                UV[2 * n + 2] = (u_0 + u) / scale[0];
                UV[2 * n + 3] = -w_l0 / scale[1];

                n += 2;
	    }
		    
	    p_0[0] += S * cos(theta_0);
	    p_0[1] += S * sin(theta_0);
	    p_0[2] += S * g_0 + 0.5 * delta_g * S;
	} else if (fabs(kappa_0) > 1e-6 && fabs(delta_kappa) < 1e-6) {
	    double u, r, c[2];
	    int m;

            /* A circular segment. */
            
	    r = 1 / kappa_0;

            m = fmax(ceil(S / tessellation[0]),
                     ceil (sqrt(fabs (delta_g) * S / 8 / tessellation[1])));
            m = m > 0 ? m : 1;
		
	    c[0] = p_0[0] - r * sin(theta_0);
	    c[1] = p_0[1] + r * cos(theta_0);

	    for (j = 0, u = S / m ; j < m ; j += 1, u += S / m) {
		double h, theta;
		    
		theta = theta_0 + S * (j + 1) / m / r;
		    
		e_0 += delta_e / m;
		w_l0 += delta_wl / m;
		w_r0 += delta_wr / m;
		    
		h = u * g_0 + 0.5 * delta_g * u * u / S;

		phi = atan(e_0);
		psi = -atan(g_0 + delta_g * u / S);

                V = realloc (V, (n + 2) * 3 * sizeof (float));
		
                N = realloc (N, (n + 2) * 3 * sizeof (float));
		
                UV = realloc (UV, (n + 2) * 2 * sizeof (float));
		
                V[3 * n] = c[0] + (r - w_r0) * sin(theta);
                V[3 * n + 1] = c[1] - (r - w_r0) * cos(theta);
                V[3 * n + 2] = p_0[2] + h + w_r0 * e_0;

                V[3 * n + 3] = c[0] + (r + w_l0) * sin(theta);
                V[3 * n + 4] = c[1] - (r + w_l0) * cos(theta);
                V[3 * n + 5] = p_0[2] + h - w_l0 * e_0;

                N[3 * n + 0] =
                    sin(theta) * sin(phi) +
                    cos(phi) * sin (psi) * cos (theta);
                N[3 * n + 1] =
                    -cos(theta) * sin(phi) +
                    cos(phi) * sin (psi) * sin (theta);
                N[3 * n + 2] =
                    cos(phi) * cos(psi);

                N[3 * n + 3] = N[3 * n + 0];
                N[3 * n + 4] = N[3 * n + 1];
                N[3 * n + 5] = N[3 * n + 2];

                UV[2 * n] = (u_0 + u) / scale[0];
                UV[2 * n + 1] = w_r0 / scale[1];
    
                UV[2 * n + 2] = (u_0 + u) / scale[0];
                UV[2 * n + 3] = -w_l0 / scale[1];

                n += 2;
                
		/* printf ("%f, %f, %f\n", c[0] + (r - w_r0) * sin(theta), */
		/* 	    c[1] - (r - w_r0) * cos(theta), */
		/* 	    p_0[2] + h + w_r0 * e_0); */
		/* printf ("%f, %f, %f\n", c[0] + (r + w_l0) * sin(theta), */
		/* 	    c[1] - (r + w_l0) * cos(theta), */
		/* 	    p_0[2] + h - w_l0 * e_0); */
	    }
		
	    theta_0 += S / r;
	    p_0[0] = c[0] + r * sin(theta_0);
	    p_0[1] = c[1] - r * cos(theta_0);
	    p_0[2] += S * g_0 + 0.5 * delta_g * S;
	} else {
	    double r, c[2], u;
	    int m, l, k;

            /* A transitional curve. */

	    m = (int)fmax(ceil(fabs(delta_kappa) / tolerance), 1);

            l = fmax(ceil(S / m / tessellation[0]),
                     ceil (sqrt(fabs (delta_g * S) / m / m / 8 / tessellation[1])));
            l = l > 0 ? l : 1;
	    /* printf ("* %d, %d, %d\n", i, m, n); */
		
	    for (j = 0, u = 0; j < m ; j += 1) {
		double h;

		kappa_0 += delta_kappa / (m + 1);
		r = 1 / kappa_0;
		
		c[0] = p_0[0] - r * sin(theta_0);
		c[1] = p_0[1] + r * cos(theta_0);

		for (k = 0 ; k < l ; k += 1) {
		    double theta;
		    
		    e_0 += delta_e / m / l;
		    w_l0 += delta_wl / m / l;
		    w_r0 += delta_wr / m / l;
		    u += S / m / l;
		    
		    theta = theta_0 + S * (k + 1) / l / r / m;
		    h = u * g_0 + 0.5 * delta_g * u * u / S;
	
		    phi = atan(e_0);
		    psi = -atan(g_0 + delta_g * u / S);
	
                    V = realloc (V, (n + 2) * 3 * sizeof (float));

                    N = realloc (N, (n + 2) * 3 * sizeof (float));

                    UV = realloc (UV, (n + 2) * 2 * sizeof (float));
		
                    V[3 * n] = c[0] + (r - w_r0) * sin(theta);
                    V[3 * n + 1] = c[1] - (r - w_r0) * cos(theta);
                    V[3 * n + 2] = p_0[2] + h + w_r0 * e_0;

                    V[3 * n + 3] = c[0] + (r + w_l0) * sin(theta);
                    V[3 * n + 4] = c[1] - (r + w_l0) * cos(theta);
                    V[3 * n + 5] = p_0[2] + h - w_l0 * e_0;

                    N[3 * n + 0] =
                        sin(theta) * sin(phi) +
                        cos(phi) * sin (psi) * cos (theta);
                    N[3 * n + 1] =
                        -cos(theta) * sin(phi) +
                        cos(phi) * sin (psi) * sin (theta);
                    N[3 * n + 2] =
                        cos(phi) * cos(psi);

                    N[3 * n + 3] = N[3 * n + 0];
                    N[3 * n + 4] = N[3 * n + 1];
                    N[3 * n + 5] = N[3 * n + 2];

                    UV[2 * n] = (u_0 + u) / scale[0];
                    UV[2 * n + 1] = w_r0 / scale[1];
    
                    UV[2 * n + 2] = (u_0 + u) / scale[0];
                    UV[2 * n + 3] = -w_l0 / scale[1];
		    
                    n += 2;

		    /* printf ("%f, %f, %f\n", c[0] + (r - w_r0) * sin(theta), */
		    /* 		c[1] - (r - w_r0) * cos(theta), */
		    /* 		p_0[2] + h + w_r0 * e_0); */
		    /* printf ("%f, %f, %f\n", c[0] + (r + w_l0) * sin(theta), */
		    /* 		c[1] - (r + w_l0) * cos(theta), */
		    /* 		p_0[2] + h - w_l0 * e_0); */
		}

		theta_0 += S / r / m;
		
		p_0[0] = c[0] + r * sin(theta_0);
		p_0[1] = c[1] - r * cos(theta_0);
	    }

	    p_0[2] += S * g_0 + 0.5 * delta_g * S;
	}

	u_0 += S;
	w_l0 = segments[10 * i + 1];
	w_r0 = segments[10 * i + 2];
	kappa_0 = segments[10 * i + 3];
	g_0 = segments[10 * i + 4];
	e_0 = segments[10 * i + 5];
    }

    *vertices = V;
    *normals = N;
    *uv = UV;
    *vertices_n = n;

    /* printf ("!!! %f, %f, %f,  %f\n", p_0[0], p_0[1], p_0[2], theta_0 / M_PI); */
}

static int test (double *segments, int index, int size, double tolerance,
		 const dReal *q, dReal *n, dReal *d,
		 double *longitude, double *latitude)
{
    double theta_0, p_0[3], delta_wl, delta_wr, w_l0, w_r0, u, v;
    double S, kappa_0, delta_kappa, g_0, delta_g, e_0, delta_e;
    int i, j, s, t;

    for (s = 0, t = 0 ;
    	 t < size ;
    	 t += 1, s >= 0 ? (s = -(s + 1)) : (s *= -1)) {
    	/* if (t > 0) */
    	/*     printf ("%d, %d\n", index, t); */
	
    	i = (index + size + s) % size;
	j = i > 0 ? i - 1 : i;
	/* printf ("*** %d, %f\n", i, segments[10 * i]); */

	/* Initalize the lot. */
   
	w_l0 = segments[10 * j + 1];
	w_r0 = segments[10 * j + 2];
	kappa_0 = segments[10 * j + 3];
	g_0 = segments[10 * j + 4];
	e_0 = segments[10 * j + 5];

	p_0[0] = segments[10 * i + 6];
	p_0[1] = segments[10 * i + 7];
	p_0[2] = segments[10 * i + 8];
	theta_0 = segments[10 * i + 9];	

	S = segments[10 * i];
	delta_wl = segments[10 * i + 1] - w_l0;
	delta_wr = segments[10 * i + 2] - w_r0;
	delta_kappa = segments[10 * i + 3] - kappa_0;
	delta_g = segments[10 * i + 4] - g_0;
	delta_e = segments[10 * i + 5] - e_0;
	    
	if (fabs(kappa_0) < 1e-6 && fabs(delta_kappa) < 1e-6) {
	    double c, s;

	    /* This is a tangent. */
	    
	    c = cos(theta_0);
	    s = sin(theta_0);
	    
	    u = (q[0] - p_0[0]) * c + (q[1] - p_0[1]) * s;
	    v = -(q[0] - p_0[0] - u * c) * s + (q[1] - p_0[1] - u * s) * c;

	    if (u > 0 && u < S &&
		v > -(w_l0 + u / S * delta_wl) &&
		v < w_r0 + u / S * delta_wr) {
		double e, g, phi, psi;

		e = e_0 + delta_e * u / S;
		g = g_0 + delta_g * u / S;
		
		phi = atan(e);
		psi = -atan(g);

		n[0] = sin(theta_0) * sin(phi) +
		    cos(phi) * sin (psi) * cos (theta_0);
		n[1] = -cos(theta_0) * sin(phi) +
		    cos(phi) * sin (psi) * sin (theta_0);
		n[2] = cos(phi) * cos(psi);

		/* Project vertical depth onto normal. */
		
		*d = (p_0[2] +
		      u * g_0 + 0.5 * delta_g * u * u / S +
		      v * e - q[2]) * n[2];

		if (longitude) {
		    double u_0;
		    int ii;

		    for (ii = 0, u_0 = 0 ; ii < i; u_0 += segments[10 * ii], ii += 1);
		    *longitude = u_0 + u;
		}

		if (latitude) {
		    *latitude = v;
		}
		/* printf ("%d, %d, %f, %f\n", i, j, g_0, delta_g); */
		/* printf ("%d: %f, %f, %f, %f, %f, %f, %f\n", i, u, v, e, phi, n[1], n[2], *d); */

		return i;
	    }
	} else if (fabs(kappa_0) > 1e-6 && fabs(delta_kappa) < 1e-6) {
	    double m, s, r, theta, c[2];

	    /* This is a circular segment. */
	    
	    s = kappa_0 < 0 ? -1 : 1;
	    r = fabs(1 / kappa_0);
		
	    c[0] = p_0[0] - s * r * sin(theta_0);
	    c[1] = p_0[1] + s * r * cos(theta_0);

	    m = sqrt ((q[0] - c[0]) * (q[0] - c[0]) +
		      (q[1] - c[1]) * (q[1] - c[1]));

	    theta = atan2(-s * (c[0] - q[0]), s * (c[1] - q[1]));

	    u = s * (theta - theta_0) * r;

	    if (u < 0) {
		u += 2 * M_PI * r;
	    } else if (u > 2 * M_PI * r) {
		u -= 2 * M_PI * r;
	    }
	    
	    v = s * (r - m);
	    
	    if (u > 0 && u < S &&
		v > -(w_l0 + u / S * delta_wl) &&
		v < w_r0 + u / S * delta_wr) {
		double e, g, phi, psi;

		e = e_0 + delta_e * u / S;
		g = g_0 + delta_g * u / S;
		
		phi = atan(e);
		psi = -atan(g);
		
		n[0] = sin(theta) * sin(phi) +
		    cos(phi) * sin (psi) * cos (theta);
		n[1] = -cos(theta) * sin(phi) +
		    cos(phi) * sin (psi) * sin (theta);
		n[2] = cos(phi) * cos(psi);

		/* Project vertical depth onto normal. */
		
		*d = (p_0[2] +
		      u * g_0 + 0.5 * delta_g * u * u / S +
		      v * e - q[2]) * n[2];

		if (longitude) {
		    double u_0;
		    int ii;

		    for (ii = 0, u_0 = 0 ; ii < i; u_0 += segments[10 * ii], ii += 1);
		    *longitude = u_0 + u;
		}

		if (latitude) {
		    *latitude = v;
		}

		return i;
	    }
	} else {
	    double s, r, c[2], m, theta;
	    int k;

	    /* This is acutally a tranistional curve, i.e.
	       a spiral but we'll just pretend its a bunch
	       of circular sub-segments of varying radius
	       pasted together. */
	    
	    k = (int)fmax(ceil(fabs(delta_kappa) / tolerance), 1);

	    for (j = 0 ; j < k ; j += 1) {
		kappa_0 += delta_kappa / (k + 1);
	    
		s = kappa_0 < 0 ? -1 : 1;
		r = fabs(1 / kappa_0);
		
		c[0] = p_0[0] - s * r * sin(theta_0);
		c[1] = p_0[1] + s * r * cos(theta_0);

		m = sqrt ((q[0] - c[0]) * (q[0] - c[0]) +
			  (q[1] - c[1]) * (q[1] - c[1]));
		
		theta = atan2(-s * (c[0] - q[0]), s * (c[1] - q[1]));
	    
		u = s * (theta - theta_0) * r;
		
		if (u < 0) {
		    u += 2 * M_PI * r;
		} else if (u > 2 * M_PI * r) {
		    u -= 2 * M_PI * r;
		}
		
		v = s * (r - m);

		if (u > 0 && u < S / k &&
		    v > -(w_l0 + u / S * delta_wl) &&
		    v < w_r0 + u / S * delta_wr) {
		    double e, g, phi, psi;
		    
		    e = e_0 + delta_e * u / S;
		    g = g_0 + delta_g * u / S;

		    /* { */
		    /* 	static void *foo; */

		    /* 	if (!foo) foo = q; */

		    /* 	if (foo == q) { */
		    /* 	    printf ("%d, %f, %f, %f, %f\n", i, p_0[0], p_0[1], */
		    /* 		    c[0] + s * r * sin(theta_0+ s * S / k / r), */
		    /* 		    c[1] - s * r * cos(theta_0+ s * S / k / r)); */
		    /* 	} */
		    /* } */
		
		    phi = atan(e);
		    psi = -atan(g);
		
		    n[0] = sin(theta) * sin(phi) +
			cos(phi) * sin (psi) * cos (theta);
		    n[1] = -cos(theta) * sin(phi) +
			cos(phi) * sin (psi) * sin (theta);
		    n[2] = cos(phi) * cos(psi);

		    /* Project vertical depth onto normal. */
		
		    *d = (p_0[2] +
			  u * g_0 + 0.5 * delta_g * u * u / S +
			  v * e - q[2]) * n[2];

		    if (longitude) {
			double u_0;
			int ii;

			for (ii = 0, u_0 = 0 ; ii < i; u_0 += segments[10 * ii], ii += 1);
			*longitude = u_0 + S * j / k + u;
		    }

		    if (latitude) {
			*latitude = v;
		    }

		    return i;
		}

		/* Update for the next segment. */

		e_0 += delta_e / k;
		theta_0 += s * S / k / r;
		w_l0 += delta_wl / k;
		w_r0 += delta_wr / k;
		
		p_0[0] = c[0] + s * r * sin(theta_0);
		p_0[1] = c[1] - s * r * cos(theta_0);
		p_0[2] += S / k * g_0 + 0.5 * delta_g * S / k / k;
		
		g_0 += delta_g / k;
	    }
	}
    }
    
    return -1;
}

static int collideTrackWithWheel (dGeomID track,
				  dGeomID wheel,
				  dContactGeom *contact)
{
    struct wheeldata *wheeldata;
    struct trackdata *trackdata;

    const dReal *R_t, *r_t, *R_w, *r_w;
    dVector3 n = {0, 0, 1}, r, rprime;
    dMatrix3 R;

    int segment;
    
    trackdata = dGeomGetClassData (track);
    wheeldata = dGeomGetClassData (wheel);
    
    r_w = dGeomGetPosition (wheel);
    R_w = dGeomGetRotation (wheel);
	
    r_t = dGeomGetPosition (track);
    R_t = dGeomGetRotation (track);

    /* Transform into track space. */
    
    dOP (rprime, -, r_w, r_t);
    dMULTIPLY1_331 (r, R_t, rprime);
    dMULTIPLY1_333 (R, R_t, R_w);

    /* Get an estimate of the surface normal by
       testing the wheel center location. */

    segment = test (trackdata->segments,
		    trackdata->last,
		    trackdata->segments_n,
		    trackdata->tolerance,
		    r, n, &wheeldata->contact.depth,
		    NULL, NULL);
	
    if (segment < 0) {
	return -1;
    }
   
    wheeldata->contact.g1 = wheel;
    wheeldata->contact.g2 = track;

    wheeldata->axial[0] = R[1];
    wheeldata->axial[1] = R[5];
    wheeldata->axial[2] = R[9];
	
    dCROSS(wheeldata->longitudinal, =, wheeldata->axial, n);
    dCROSS(wheeldata->radial, =, wheeldata->axial, wheeldata->longitudinal);
    dCROSS(wheeldata->lateral, =, n, wheeldata->longitudinal);

    /* Normalize both longitudinal and radial vectors.  (Thanks
       to Simon Fendall for pointing out that the longitudinal
       and lateral vectors need to be normalized as well. */
    
    dSafeNormalize3 (wheeldata->longitudinal);
    dSafeNormalize3 (wheeldata->radial);
    dSafeNormalize3 (wheeldata->lateral);

#if 0
    {
        Node *foo;

        foo = (Node *)dGeomGetData (wheel);

        if (foo->tag == 2) {
	    printf ("%f, %f, %f, %f\n",
		    dLENGTH(n),
		    dLENGTH(wheeldata->axial),
		    dLENGTH(wheeldata->longitudinal),
		    dLENGTH(wheeldata->radial));
        }
    }
#endif

    /* Calculate the contact point. */

    wheeldata->contact.pos[0] =
	r[0] + wheeldata->radial[0] * wheeldata->radii[0] -
	n[0] * wheeldata->radii[1];
	
    wheeldata->contact.pos[1] =
	r[1] + wheeldata->radial[1] * wheeldata->radii[0] -
	n[1] * wheeldata->radii[1];
	
    wheeldata->contact.pos[2] =
	r[2] + wheeldata->radial[2] * wheeldata->radii[0] -
	n[2] * wheeldata->radii[1];

    segment = test (trackdata->segments,
		    trackdata->last,
		    trackdata->segments_n,
		    trackdata->tolerance,
		    wheeldata->contact.pos,
		    wheeldata->contact.normal,
		    &wheeldata->contact.depth,
		    NULL, NULL);
	
    if (segment < 0) {
	return -1;
    }

    trackdata->last = segment;
    wheeldata->airborne = (wheeldata->contact.depth < 0);

    /* Transform back into the world. */
    
    dMULTIPLY0_331 (rprime, R_t, wheeldata->contact.pos);
    dOP (wheeldata->contact.pos, +, rprime, r_t);

    dOPE (rprime, =, wheeldata->axial);
    dMULTIPLY0_331 (wheeldata->axial, R_t, rprime);

    dOPE (rprime, =, wheeldata->lateral);
    dMULTIPLY0_331 (wheeldata->lateral, R_t, rprime);

    dOPE (rprime, =, wheeldata->longitudinal);
    dMULTIPLY0_331 (wheeldata->longitudinal, R_t, rprime);

    dOPE (rprime, =, wheeldata->radial);
    dMULTIPLY0_331 (wheeldata->radial, R_t, rprime);

    dOPE (rprime, =, wheeldata->contact.normal);
    dMULTIPLY0_331 (wheeldata->contact.normal, R_t, rprime);
 	
    /* { */
    /*     Node *foo; */

    /*     foo = (Node *)dGeomGetData (wheel); */
    /*     if (foo->tag == 1) { */
    /* 	    fprintf (stderr, "L: %f, %f, %f\n", wheeldata->longitudinal[0], wheeldata->longitudinal[1], wheeldata->longitudinal[2]); */
    /* 	    fprintf (stderr, "Lat: %f, %f, %f\n", wheeldata->lateral[0], wheeldata->lateral[1], wheeldata->lateral[2]); */
    /* 	    fprintf (stderr, "R: %f, %f, %f\n", wheeldata->radial[0], wheeldata->radial[1], wheeldata->radial[2]); */
    /*     } */
    /* } */
   
    return 0;
}

static int collideHeightfieldWithWheel (dGeomID field,
					dGeomID wheel,
					dContactGeom *contact,
					void (*sampler)(int, int, double *, double *),
					const int *l, const int d,
					const double *s)
{
    struct wheeldata *wheeldata;

    const dReal *R_t, *r_t, *R_w, *r_w;
    dVector3 n = {0, 0, 1}, r, rprime;
    dMatrix3 R;
    double t_f, u_f, t_i, u_i;
    
    const int A[16 * 16] =
	{1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
	 -3, 0, 0, 3, 0, 0, 0, 0,-2, 0, 0,-1, 0, 0, 0, 0,
	 2, 0, 0,-2, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0,
	 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	 0, 0, 0, 0,-3, 0, 0, 3, 0, 0, 0, 0,-2, 0, 0,-1,
	 0, 0, 0, 0, 2, 0, 0,-2, 0, 0, 0, 0, 1, 0, 0, 1,
	 -3, 3, 0, 0,-2,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0,-3, 3, 0, 0,-2,-1, 0, 0,
	 9,-9, 9,-9, 6, 3,-3,-6, 6,-6,-3, 3, 4, 2, 1, 2,
	 -6, 6,-6, 6,-4,-2, 2, 4,-3, 3, 3,-3,-2,-1,-1,-2,
	 2,-2, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	 0, 0, 0, 0, 0, 0, 0, 0, 2,-2, 0, 0, 1, 1, 0, 0,
	 -6, 6,-6, 6,-3,-3, 3, 3,-4, 4, 2,-2,-2,-2,-1,-1,
	 4,-4, 4,-4, 2, 2,-2,-2, 2,-2,-2, 2, 1, 1, 1, 1};
    
    double y[16];
    double x[16], c[16], y_i, y1_i, y2_i;
    int i, j;

    r_w = dGeomGetPosition (wheel);
    R_w = dGeomGetRotation (wheel);
	
    r_t = dGeomGetPosition (field);
    R_t = dGeomGetRotation (field);

    /* Transform into field space. */
    
    dOP (rprime, -, r_w, r_t);
    dMULTIPLY1_331 (r, R_t, rprime);
    dMULTIPLY1_333 (R, R_t, R_w);

    /* Calculate integer indices into the heighfield and
       fractional remainders to interpolate to. Note that
       the indices have to be offset to account for the
       difference in the geoms origin versus the heightfield's
       origin in memory. */
    
    t_f = modf(-r[0] / s[0] + 0.5 * (1 << d) * l[0], &t_i);
    u_f = modf(r[2] / s[1] + 0.5 * (1 << d) * l[1], &u_i);

    for (i = 0 ; i < 4 ; y[i] = NAN, i += 1);
    for (i = 4 ; i < 16 ; y[i] = 0, i += 1);

    /*********************

    13     12     11     10
    o.........o.........o.........o
    .         .         .         .
    .         .         .         .
    .         .         .         .
    .14       .3        .2       9.
    o.........o.........o.........o
    .         .         .         .
    .         .t_f      .         .
    .         .---x     .         .
    .15       .0  |u_f  .1       8.
    o.........o.........o.........o
    .         .(ti, ui) .         .
    .         .         .         .
    .         .         .         .
    .4        .5        .6       7.
    o.........o.........o.........o
    
    **********************/

    sampler (t_i + 0, u_i + 0, &y[0], NULL);
    sampler (t_i + 1, u_i + 0, &y[1], NULL);
    sampler (t_i + 1, u_i + 1, &y[2], NULL);
    sampler (t_i + 0, u_i + 1, &y[3], NULL);

    /* If any of these is still a NAN it means
       we tried to index past the edge so just
       return nothing. */
	
    for (i = 0 ; i < 4 ; i += 1) {
	if (isnan (y[i])) {
	    return -1;
	}
    }

    /* Now sample the neighborhood. */

    sampler (t_i - 1, u_i - 1, &y[4], NULL);
    sampler (t_i + 0, u_i - 1, &y[5], NULL);
    sampler (t_i + 1, u_i - 1, &y[6], NULL);
    sampler (t_i + 2, u_i - 1, &y[7], NULL);
    sampler (t_i + 2, u_i + 0, &y[8], NULL);
    sampler (t_i + 2, u_i + 1, &y[9], NULL);
    sampler (t_i + 2, u_i + 2, &y[10], NULL);
    sampler (t_i + 1, u_i + 2, &y[11], NULL);
    sampler (t_i + 0, u_i + 2, &y[12], NULL);
    sampler (t_i - 1, u_i + 2, &y[13], NULL);
    sampler (t_i - 1, u_i + 1, &y[14], NULL);
    sampler (t_i - 1, u_i + 0, &y[15], NULL);

    /* Now compute partial derivatives using Evans' method. */

    x[0] = y[0]; x[1] = y[1];
    x[2] = y[2]; x[3] = y[3];
	
    x[4] = (y[1] + y[2] + y[6] - y[15] - y[14] - y[4]) / 6;
    x[5] = (y[8] + y[9] + y[7] - y[0] - y[3] - y[5]) / 6;
    x[6] = (y[9] + y[10] + y[8] - y[3] - y[12] - y[0]) / 6;
    x[7] = (y[2] + y[11] + y[1] - y[14] - y[13] - y[15]) / 6;

    x[8] = (y[3] + y[14] + y[2] - y[5] - y[4] - y[6]) / 6;
    x[9] = (y[2] + y[3] + y[9] - y[6] - y[5] - y[7]) / 6;
    x[10] = (y[11] + y[12] + y[10] - y[1] - y[0] - y[8]) / 6;
    x[11] = (y[12] + y[13] + y[11] - y[0] - y[15] - y[1]) / 6;

    x[12] = 0.25 * (y[2] + y[4] - y[6] - y[14]);
    x[13] = 0.25 * (y[9] + y[5] - y[7] - y[3]);	
    x[14] = 0.25 * (y[10] + y[0] - y[8] - y[12]);	
    x[15] = 0.25 * (y[11] + y[15] - y[1] - y[13]);

    /* Calculate the coefficients for the bicubic spline
       and interpolate.  This is according to 'Numerical
       recipes, 3rd Edition, pp. 136' . */
    
    for (i = 0 ; i < 16 ; i += 1) {
	c[i] = 0;
	    
	for (j = 0 ; j < 16 ; j += 1) {
	    c[i] += A[i * 16 + j] * x[j];
	}
    }
    
    for (i = 3, y_i = 0, y1_i = 0, y2_i = 0;
	 i >= 0 ;
	 y_i = t_f * y_i + ((c[i * 4 + 3] * u_f + c[i * 4 + 2]) * u_f + c[i * 4 + 1]) * u_f + c[i * 4 + 0],
	     y2_i = t_f * y2_i + (3.0 * c[i * 4 + 3] * u_f + 2.0 * c[i * 4 + 2]) * u_f + c[i * 4 + 1],
	     y1_i = u_f * y1_i + (3.0 * c[3 * 4 + i] * t_f + 2.0 * c[2 * 4 + i]) * t_f + c[1 * 4 + i],
	     i -= 1);

    /* Calculate and normalize the
       contact normal vector. */
    
    n[0] = -y1_i / s[0];
    n[1] = 1;
    n[2] = -y2_i / s[1];

    dSafeNormalize3(n);

    /* Now proceed as in the racetrack collision
       handling routine. */
    
    wheeldata = dGeomGetClassData (wheel);
    wheeldata->contact.g1 = wheel;
    wheeldata->contact.g2 = field;
    
    wheeldata->axial[0] = R[1];
    wheeldata->axial[1] = R[5];
    wheeldata->axial[2] = R[9];

    dCROSS(wheeldata->longitudinal, =, wheeldata->axial, n);
    dCROSS(wheeldata->radial, =, wheeldata->axial, wheeldata->longitudinal);
    dCROSS(wheeldata->lateral, =, n, wheeldata->longitudinal);

    dSafeNormalize3 (wheeldata->radial);

    wheeldata->contact.pos[0] =
	r[0] + wheeldata->radial[0] * wheeldata->radii[0] -
	n[0] * wheeldata->radii[1];
	
    wheeldata->contact.pos[1] =
	r[1] + wheeldata->radial[1] * wheeldata->radii[0] -
	n[1] * wheeldata->radii[1];
	
    wheeldata->contact.pos[2] =
	r[2] + wheeldata->radial[2] * wheeldata->radii[0] -
	n[2] * wheeldata->radii[1];

    wheeldata->contact.normal[0] = n[0];
    wheeldata->contact.normal[1] = n[1];
    wheeldata->contact.normal[2] = n[2];

    /* Project the vertical penetration depth onto the normal
       to get the depth along the normal. */
    
    wheeldata->contact.depth = (y_i - wheeldata->contact.pos[1]) * n[1];
    wheeldata->airborne = (wheeldata->contact.depth < 0);

    /* Transform back into the world. */
    
    dMULTIPLY0_331 (rprime, R_t, wheeldata->contact.pos);
    dOP (wheeldata->contact.pos, +, rprime, r_t);

    dOPE (rprime, =, wheeldata->axial);
    dMULTIPLY0_331 (wheeldata->axial, R_t, rprime);

    dOPE (rprime, =, wheeldata->lateral);
    dMULTIPLY0_331 (wheeldata->lateral, R_t, rprime);

    dOPE (rprime, =, wheeldata->longitudinal);
    dMULTIPLY0_331 (wheeldata->longitudinal, R_t, rprime);

    dOPE (rprime, =, wheeldata->radial);
    dMULTIPLY0_331 (wheeldata->radial, R_t, rprime);

    dOPE (rprime, =, wheeldata->contact.normal);
    dMULTIPLY0_331 (wheeldata->contact.normal, R_t, rprime);

#if 0
    {
        Node *foo;

        foo = (Node *)dGeomGetData (wheel);

        if (foo->tag == 2) {
    	    /* { */
    	    /* 	double y_il = ((1 - t_f) * y[0] + t_f * y[1]) * (1 - u_f) + */
    	    /* 	    ((1 - t_f) * y[3] + t_f * y[2]) * u_f; */
		
    	    /* 	printf ("%f, %f\n", fabs(y_i - y_il), wheeldata->contact.depth); */
    	    /* } */
	    
    	    /* printf ("nn %.5f, %.5f, %.5f\n", */
    	    /* 	    wheeldata->contact.normal[0], */
    	    /* 	    wheeldata->contact.normal[1], */
    	    /* 	    wheeldata->contact.normal[2]); */
	
    	    /* printf ("pp %.5f, %.5f, %.5f\n", */
    	    /* 	    wheeldata->contact.pos[0], */
    	    /* 	    wheeldata->contact.pos[1], */
    	    /* 	    wheeldata->contact.pos[2]); */
    	    /* printf ("N %f\n", dLENGTH(wheeldata->contact.normal)); */
    	    /* printf ("dd %f\n", wheeldata->contact.depth); */
        }
    }
#endif
  
    return 0;
}

static int collideWithWheel (dGeomID track,
			     dGeomID wheel,
			     int flags,
			     dContactGeom *contact,
			     int skip)
{
    int n;

    n = collideTrackWithWheel (track, wheel, contact);
    
    if (n < 0) {
        struct trackdata *trackdata;
        struct wheeldata *wheeldata;

        trackdata = dGeomGetClassData (track);
        wheeldata = dGeomGetClassData (wheel);
    
	if (!trackdata->field ||
	    collideHeightfieldWithWheel (trackdata->field,
					 wheel, contact,
					 trackdata->sampler,
					 trackdata->tiles,
					 trackdata->depth,
					 trackdata->resolution) < 0) {
	    wheeldata->airborne = 1;
	}
    }

    return 0;
}

static dColliderFn * getCollider (int num)
{
    if (num == dWheelClass) {
	return collideWithWheel;
    } else {
	return 0;
    }
}

static int sampler_tostring(lua_State *L)
{
    lua_pushstring(L, "Sampler");
   
    return 1;
}

static int sampler_index(lua_State *L)
{
    struct trackdata *data;
    
    if (lua_istable (L, 2)) {
	double n[3], d, u, v;
	double p[3] = {0, 0, 0};
	int s;

	/* Get the track data. */
	
	data = lua_touserdata (L, lua_upvalueindex(1));

	/* Get the sampling point. */
	
	lua_rawgeti (L, 2, 1);
	p[0] = lua_tonumber (L, -1);

	lua_rawgeti (L, 2, 2);
	p[1] = lua_tonumber (L, -1);

	lua_pop (L, 2);

	/* Sample the track. */
	
	s = test (data->segments, 0,
		  data->segments_n, data->tolerance,
		  p, n, &d, &u, &v);
	
	if (s >= 0) {
	    lua_newtable (L);
	    lua_pushinteger (L, s);
	    lua_rawseti (L, -2, 1);
	    
	    lua_pushnumber (L, u);
	    lua_rawseti (L, -2, 2);
	    
	    lua_pushnumber (L, v);
	    lua_rawseti (L, -2, 3);
	    
	    lua_pushnumber (L, d);
	    lua_rawseti (L, -2, 4);	    
	} else {
	    lua_pushboolean (L, 0);
	}
    } else {
	lua_pushnil (L);
    }

    return 1;
}


@implementation Racetrack

-(void) init
{
    [super init];
    
    self->segments = NULL;
    self->segments_n = 0;
    self->tolerance = 0.01 / 10;
}

-(int) _get_segments
{
    int i, j;    

    lua_createtable(_L, self->segments_n, 0);
    
    for (j = 0 ; j < self->segments_n ; j += 1) {
        lua_createtable (_L, 6, 0);

        for(i = 0; i < 6; i += 1) {
            lua_pushnumber (_L, self->segments[10 * j + i]);
            lua_rawseti (_L, -2, i + 1);
        }

        lua_rawseti (_L, -2, j + 1);
    }

    return 1;
}

-(void) _set_segments
{
    int i, j;    

    if(!lua_isnil (_L, 3)) {
        lua_len (_L, 3);
        
        self->segments_n = lua_tointeger(_L, -1);
        self->segments = realloc (self->segments,
                                  10 * self->segments_n * sizeof(double));
        
        lua_pop (_L, 1);

        for (j = 0 ; j < self->segments_n ; j += 1) {
            lua_pushinteger (_L, j + 1);
            lua_gettable (_L, 3);
            
            for(i = 0 ; i < 6 ; i += 1) {
                lua_pushinteger (_L, i + 1);
                lua_gettable (_L, -2);
                self->segments[10 * j + i] = lua_tonumber (_L, -1);
                lua_pop (_L, 1);
            }

            lua_pop (_L, 1);
        }
    }
}

-(int) _get_tolerance
{
    lua_pushnumber (_L, self->tolerance);

    return 1;
}

-(void) _set_tolerance
{
    self->tolerance = lua_tonumber (_L, 3);
}

-(int) _get_body
{
    t_pushuserdata(_L, 1, self);
    lua_pushcclosure(_L, constructbody, 1);

    return 1;
}

-(void) _set_body
{
}

-(int) _get_shape
{
    t_pushuserdata(_L, 1, self);
    lua_pushcclosure(_L, constructshape, 1);

    return 1;
}

-(void) _set_shape
{
}

@end

@implementation RacetrackBody

-(void)initWith: (int)n segments: (double *)s andTolerance: (double)t
{
    struct trackdata *data;
    
    if (!dTrackClass) {
	struct dGeomClass class = {
	    sizeof (struct trackdata),
	    getCollider,
	    dInfiniteAABB,
	    0, 0
	};
      
	dTrackClass = dCreateGeomClass (&class);
    }

    self->geom = dCreateGeom (dTrackClass);
    dGeomSetData (self->geom, self);

    data = dGeomGetClassData (self->geom);

    data->segments = malloc(10 * n * sizeof(double));
    memcpy (data->segments, s, 10 * n * sizeof(double));
    data->segments_n = n;
    data->tolerance = t;
    data->last = 0;
    
    data->tiles = NULL;
    data->depth = 0;
    data->resolution = NULL;
    data->sampler = NULL;

    update_track_segments(data->segments, data->segments_n, data->tolerance);

    [super init];
}

-(void) meetSibling: (id)sibling
{
    struct trackdata *data;

    if ([sibling isKindOf: [Ground class]]) {
	Ground *ground;

	ground = (Ground *)sibling;
	data = dGeomGetClassData (self->geom);

	data->sampler = ground->sampler;
	data->field = ground->geom;
	data->tiles = ground->size;
	data->depth = ground->depth;
	data->resolution = ground->resolution;
    }
}

-(void) missSibling: (id)sibling
{
    struct trackdata *data;

    if ([sibling isKindOf: [Ground class]]) {
	data = dGeomGetClassData (self->geom);

	data->sampler = NULL;
	data->field = NULL;
	data->tiles = NULL;
	data->depth = 0;
	data->resolution = NULL;
    }
}
	
-(int) _get_sampler
{
    struct trackdata *data;
    
    data = dGeomGetClassData (self->geom);

    lua_newtable (_L);

    lua_newtable (_L);
    lua_pushstring(_L, "__track");
    lua_pushvalue (_L, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__index");
    lua_pushlightuserdata (_L, data);
    lua_pushcclosure(_L, (lua_CFunction)sampler_index, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__tostring");
    lua_pushcfunction(_L, (lua_CFunction)sampler_tostring);
    lua_settable(_L, -3);
    lua_setmetatable(_L, -2);	

    return 1;
}

-(void) _set_sampler
{
    T_WARN_READONLY;
}

-(void) free
{
    struct trackdata *data;

    data = dGeomGetClassData (self->geom);

    if (data->segments) {
	free (data->segments);
    }

    [super free];
}

@end

@implementation RacetrackShape

-(void)initWith: (int)n segments: (double *)s andTolerance: (double)t
{
    self->dirty = 1;
    
    self->scale[0] = 1;
    self->scale[1] = 1;
    
    self->tessellation[0] = 2 * M_PI / 32;
    self->tessellation[1] = 0.1 / 10;

    self->tolerance = t;
    self->segments_n = n;
    self->segments = malloc(10 * n * sizeof(double));
    memcpy (self->segments, s, 10 * n * sizeof(double));
    
    [super initWithMode: GL_TRIANGLE_STRIP];
}

-(int) _get_scale
{
    lua_newtable (_L);
	
    lua_pushnumber (_L, self->scale[0]);
    lua_rawseti (_L, 3, 1);
	
    lua_pushnumber (_L, self->scale[1]);
    lua_rawseti (_L, 3, 2);

    return 1;
}

-(int) _get_tessellation
{
    lua_newtable (_L);
    
    lua_pushnumber (_L, self->tessellation[0]);
    lua_rawseti (_L, 3, 1);
	
    lua_pushnumber (_L, self->tessellation[1]);
    lua_rawseti (_L, 3, 2);

    return 1;
}

-(void) _set_scale
{
    if (!lua_isnil(_L, 3)) {
        lua_rawgeti (_L, 3, 1);
        self->scale[0] = lua_tonumber (_L, -1);
	
        lua_rawgeti (_L, 3, 2);
        self->scale[1] = lua_tonumber (_L, -1);

        self->dirty = 1;
    }    
}

-(void) _set_tessellation
{
    if (!lua_isnil(_L, 3)) {
        lua_rawgeti (_L, 3, 1);
        self->tessellation[0] = lua_tonumber (_L, -1);
	
        lua_rawgeti (_L, 3, 2);
        self->tessellation[1] = lua_tonumber (_L, -1);

        lua_pop (_L, 2);

        self->dirty = 1;
    }    
}

-(void) draw
{
    if (self->dirty) {
        float *vertices, *normals, *uv;
        int size;

        update_track_shape(self->segments, self->segments_n, self->tolerance,
                           self->tessellation, self->scale,
                           &vertices, &normals, &uv, &size);
        
        t_pushuserdata(_L, 1, self);
    
        lua_pushliteral(_L, "positions");
        array_createarray(_L, ARRAY_TFLOAT, vertices, 2, size, 3);
        lua_settable (_L, -3);

        lua_pushliteral(_L, "mapping");
        array_createarray(_L, ARRAY_TFLOAT, uv, 2, size, 2);
        lua_settable (_L, -3);

        lua_pushliteral(_L, "normals");
        array_createarray(_L, ARRAY_TFLOAT, normals, 2, size, 3);
        lua_settable (_L, -3);

        lua_pop(_L, 1);
        
        free(vertices);
        free(uv);
        free(normals);

        self->dirty = 0;
    }

    [super draw];
}

@end
