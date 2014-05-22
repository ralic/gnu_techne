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
#include <string.h>
#include <math.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "gl.h"
#include "glx.h"

#include "array/array.h"
#include "techne.h"
#include "algebra.h"
#include "graphics.h"
#include "input.h"
#include "shader.h"
#include "transform.h"
#include "root.h"

static GdkWindow *window;
static GLXPbuffer pbuffer;
GLXFBConfig configuration;
static GdkDisplay *display;
static GdkScreen *screen;

static Graphics* instance;
static int have_context;

#define PROJECTION_OFFSET (0)
#define PROJECTION_SIZE (sizeof(float[16]))
#define MODELVIEW_OFFSET (PROJECTION_OFFSET + PROJECTION_SIZE)
#define MODELVIEW_SIZE (sizeof(float[16]))
#define TRANSFORM_OFFSET (MODELVIEW_OFFSET + MODELVIEW_SIZE)
#define TRANSFORM_SIZE (sizeof(float[16]))
#define UNIFORM_BUFFER_SIZE (TRANSFORM_OFFSET + TRANSFORM_SIZE)
#define PROJECTION_STACK_DEPTH 8
#define MODELVIEW_STACK_DEPTH 8

static enum {
    ORTHOGRAPHIC, PERSPECTIVE, FRUSTUM
} projection;

static int width, height;
static int hide = 1, cursor = 1, decorate = 1, offscreen = 0;
static int redbits = 1, greenbits = 1, bluebits = 1, alphabits = 1;
static int depthbits = 1, stencilbits = 8, samples = 4;

static int frames;
static double planes[6], frustum[3];
static int focus = LUA_REFNIL, defocus = LUA_REFNIL;
static int configure = LUA_REFNIL, delete = LUA_REFNIL;
static char *title;

static unsigned int buffer;

static double modelviews[MODELVIEW_STACK_DEPTH][16];
static double projections[PROJECTION_STACK_DEPTH][16];
static double transform[16];
static int modelviews_n, projections_n;

/* Context flags. */

static int debug, reportonce, interrupt, arbcontext;

static unsigned int *keys;
static int keys_n;

static void APIENTRY debug_callback (GLenum source,
                                     GLenum type,
                                     GLuint id,
                                     GLenum severity,
                                     GLsizei length,
                                     const GLchar *message,
                                     const void *userParam)
{
    int c;
    const char *s;

    switch (type) {
    case GL_DEBUG_TYPE_ERROR_ARB:
        c = 31;
        s = "error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        c = 35;
        s = "deprecated behavior";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        c = 33;
        s = "undefined behavior";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        c = 36;
        s = "performance warning";
        break;
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        c = 34;
        s = "portability warning";
        break;
    case GL_DEBUG_TYPE_OTHER_ARB:default:
        c = 37;
        s = "";
        break;
    }

    t_print_message("%sGL %s %d:%s %s\n",
                    t_ansi_color(c, 1),
                    s, id,
                    t_ansi_color(0, 0),
                    message);

    /* Ignore this kind of message from now on. */

    if (reportonce) {
        glDebugMessageControlARB (source, type, GL_DONT_CARE,
                                  1, &id, GL_FALSE);
    }

    if (interrupt) {
        raise(2);
    }
}

static void update_projection()
{
    double P[16];
    double a;

    memset (P, 0, sizeof (double[16]));

    switch (projection) {
    case FRUSTUM:
        a = (double)width / height;

        /* Set planes based on frustum.  This will result in the same
         * matrix as:
         *     gluPerspective (frustum[0], a, frustum[1], frustum[2]);
         *
         * Note: intentional absence of break at the end.
         */

        planes[4] = frustum[1];
        planes[5] = frustum[2];
        planes[3] = frustum[1] * tan(0.5 * frustum[0]);
        planes[2] = -planes[3];

        planes[0] = planes[2] * a;
        planes[1] = planes[3] * a;
    case PERSPECTIVE:
        P[0] = 2 * planes[4] / (planes[1] - planes[0]);
        P[8] = (planes[1] + planes[0]) / (planes[1] - planes[0]);
        P[5] = 2 * planes[4] / (planes[3] - planes[2]);
        P[9] = (planes[3] + planes[2]) / (planes[3] - planes[2]);
        P[10] = -(planes[5] + planes[4]) / (planes[5] - planes[4]);
        P[14] = -2 * planes[5] * planes[4] / (planes[5] - planes[4]);
        P[11] = -1;
        break;
    case ORTHOGRAPHIC:
        P[0] = 2 / (planes[1] - planes[0]);
        P[12] = -(planes[1] + planes[0]) / (planes[1] - planes[0]);
        P[5] = 2 / (planes[3] - planes[2]);
        P[13] = -(planes[3] + planes[2]) / (planes[3] - planes[2]);
        P[10] = -2 / (planes[5] - planes[4]);
        P[14] = -(planes[5] + planes[4]) / (planes[5] - planes[4]);
        P[15] = 1;
        break;
    }

    t_load_projection(P);
}

#define SET_PROJECTION(P)						\
    {									\
        float T_f[16], P_f[16];                                         \
        int i;                                                          \
                                                                        \
        t_concatenate_4T(transform, (P), modelviews[modelviews_n]);     \
                                                                        \
        for (i = 0 ; i < 16 ; i += 1) {                                 \
            P_f[i] = P[i];                                              \
            T_f[i] = transform[i];                                      \
        }                                                               \
                                                                        \
        glBindBuffer(GL_UNIFORM_BUFFER, buffer);			\
                                                                        \
        glBufferSubData(GL_UNIFORM_BUFFER, PROJECTION_OFFSET,		\
                        PROJECTION_SIZE, P_f);                          \
        glBufferSubData(GL_UNIFORM_BUFFER, TRANSFORM_OFFSET,		\
                        TRANSFORM_SIZE, T_f);                           \
                                                                        \
        if(0) {								\
            float M[16];						\
                                                                        \
            glGetBufferSubData(GL_UNIFORM_BUFFER, PROJECTION_OFFSET,    \
                               PROJECTION_SIZE, M);                     \
                                                                        \
            _TRACEM(4, 4, ".5f", M);                                    \
        }								\
    }

void t_load_projection (double *matrix)
{
    memcpy(projections[projections_n], matrix, 16 * sizeof(double));

    SET_PROJECTION(matrix);
}

void t_push_projection (double *matrix)
{
    projections_n += 1;
    assert (projections_n < PROJECTION_STACK_DEPTH);

    t_load_projection(matrix);
}

void t_pop_projection ()
{
    projections_n -= 1;
    assert (projections_n >= 0);

    SET_PROJECTION(projections[projections_n]);
}

void t_copy_projection(double *matrix)
{
    memcpy(matrix, projections[projections_n], 16 * sizeof(double));
}

#define SET_MODELVIEW(M)						\
    {									\
        float T_f[16], M_f[16];                                         \
        int i;                                                          \
                                                                        \
        t_concatenate_4T(transform, projections[projections_n], (M));   \
                                                                        \
        for (i = 0 ; i < 16 ; i += 1) {                                 \
            M_f[i] = M[i];                                              \
            T_f[i] = transform[i];                                      \
        }                                                               \
                                                                        \
        glBindBuffer(GL_UNIFORM_BUFFER, buffer);			\
        glBufferSubData(GL_UNIFORM_BUFFER, MODELVIEW_OFFSET,		\
                        MODELVIEW_SIZE, M_f);                           \
        glBufferSubData(GL_UNIFORM_BUFFER, TRANSFORM_OFFSET,		\
                        TRANSFORM_SIZE, T_f);                           \
                                                                        \
        if(0) {								\
            float N[16];						\
                                                                        \
            glGetBufferSubData(GL_UNIFORM_BUFFER, MODELVIEW_OFFSET,     \
                               MODELVIEW_SIZE, N);                      \
                                                                        \
            _TRACEM(4, 4, ".5f", N);                                    \
        }								\
    }

void t_load_modelview (double *matrix, t_MatrixLoadMode mode)
{
    if (mode == T_MULTIPLY) {
        double M[16];

        t_concatenate_4T(M, modelviews[modelviews_n], matrix);
        memcpy(modelviews[modelviews_n], M, 16 * sizeof(double));
    } else {
        memcpy(modelviews[modelviews_n], matrix, 16 * sizeof(double));
    }

    SET_MODELVIEW(modelviews[modelviews_n]);
}

void t_push_modelview (double *matrix, t_MatrixLoadMode mode)
{
    if (mode == T_MULTIPLY) {
        t_concatenate_4T(modelviews[modelviews_n + 1],
                         modelviews[modelviews_n], matrix);
    } else {
        memcpy(modelviews[modelviews_n + 1], matrix, 16 * sizeof(double));
    }

    modelviews_n += 1;
    assert (modelviews_n < MODELVIEW_STACK_DEPTH);

    SET_MODELVIEW(modelviews[modelviews_n]);
}

void t_pop_modelview ()
{
    modelviews_n -= 1;
    assert (modelviews_n >= 0);

    SET_MODELVIEW(modelviews[modelviews_n]);
}

void t_copy_modelview(double *matrix)
{
    memcpy(matrix, modelviews[modelviews_n], 16 * sizeof(double));
}

void t_copy_transform(double *matrix)
{
    memcpy(matrix, transform, 16 * sizeof(double));
}

void t_get_pointer (int *x, int *y)
{
    if (window) {
        GdkModifierType mask;

        gdk_window_get_pointer (window, x, y, &mask);
    } else {
        *x = -1;
        *y = -1;
    }
}

void t_warp_pointer (int x, int y)
{
    if (window) {
        int x_w, y_w;

        gdk_window_get_origin (window, &x_w, &y_w);
        gdk_display_warp_pointer (display, screen, x + x_w, y + y_w);
        gdk_flush();
    }
}

@implementation Graphics

-(void) init
{
    int argc = 0;
    char **argv = NULL;

    self->index = 4;

    [super init];

    gdk_init(&argc, &argv);

    display = gdk_display_get_default ();
    screen = gdk_display_get_default_screen (display);

    lua_pushstring (_L, "graphics");
    lua_setfield (_L, -2, "tag");

    instance = self;
}

+(Builtin *)instance
{
    return instance;
}

-(void) free
{
    GLXContext context;

    context = glXGetCurrentContext ();

    glXMakeContextCurrent(GDK_DISPLAY_XDISPLAY(display), None, None, NULL);
    glXDestroyContext(GDK_DISPLAY_XDISPLAY(display), context);

    if (offscreen) {
        glXDestroyPbuffer(GDK_DISPLAY_XDISPLAY(display), pbuffer);
    } else if (window) {
        gdk_window_destroy(window);
    }

    [super free];
}

-(void) iterate
{
    Node *root;
    GdkEvent *event;

    t_begin_cpu_interval (&self->core);

    if (frames > 0) {
        glXSwapBuffers (GDK_DISPLAY_XDISPLAY(display),
                        offscreen ? pbuffer : GDK_WINDOW_XWINDOW(window));

        t_end_gpu_interval(&self->latency);
    }

    t_begin_gpu_interval(&self->latency);

    while ((event = gdk_event_get()) != NULL) {
        assert(event);

        /* _TRACE ("%d\n", event->type); */
        assert(event);

        /* Ignore double/triple-click events for now.  Individual
         * press/release event sets are generated as expected. */

        if (event->type == GDK_2BUTTON_PRESS ||
            event->type == GDK_3BUTTON_PRESS) {
            gdk_event_free (event);
            continue;
        }

        /* Ignore consecutive keypresses for the same key.  These are
         * always a result of key autorepeat. */

        if (event->type == GDK_KEY_PRESS) {
            unsigned int k;
            int i, j = -1, skip = 0;

            k = ((GdkEventKey *)event)->keyval;

            for (i = 0 ; i < keys_n ; i += 1) {
                /* Find an empty spot in case we need to store the
                 * key. */

                if (keys[i] == 0) {
                    j = i;
                }

                /* Check if it's been pressed already. */

                if (keys[i] == k) {
                    skip = 1;
                    break;
                }
            }

            /* Store the key making space if necesarry. */

            if (skip) {
                gdk_event_free (event);
                continue;
            } else if (j >= 0) {
                keys[j] = k;
            } else {
                if (i == keys_n) {
                    keys_n += 1;
                    keys = realloc (keys, keys_n * sizeof(unsigned int));
                }

                keys[i] = k;
            }
        } else if (event->type == GDK_KEY_RELEASE) {
            unsigned int k;
            int i;

            k = ((GdkEventKey *)event)->keyval;

            /* Remove the key from the list. */

            for (i = 0 ; i < keys_n ; i += 1) {
                if (keys[i] == k) {
                    keys[i] = 0;
                    break;
                }
            }
        }

        switch(event->type) {
        case GDK_CONFIGURE:
            width = ((GdkEventConfigure *)event)->width;
            height = ((GdkEventConfigure *)event)->height;

            glViewport (0, 0, width, height);

            /* If a projection frustum has been specified update the
             * planes as the viewport aspect ratio has probably
             * changed. */

            if (projection == FRUSTUM) {
                /* Set planes based on frustum. */

                update_projection();
            }

            t_pushuserdata (_L, 1, self);
            t_callhook (_L, configure, 1, 0);
            break;
        case GDK_FOCUS_CHANGE:
            t_pushuserdata (_L, 1, self);

            if (((GdkEventFocus *)event)->in == TRUE) {
                t_callhook (_L, focus, 1, 0);
            } else {
                t_callhook (_L, defocus, 1, 0);
            }
            break;
        case GDK_DELETE:
            t_callhook (_L, delete, 0, 0);
            break;
        case GDK_NOTHING:
        case GDK_MAP:
        case GDK_UNMAP:
        case GDK_WINDOW_STATE:
        case GDK_VISIBILITY_NOTIFY:
            break;
        case GDK_KEY_PRESS: case GDK_KEY_RELEASE:
        case GDK_BUTTON_PRESS: case GDK_BUTTON_RELEASE:
        case GDK_SCROLL: case GDK_MOTION_NOTIFY:
            [Input addEvent: event];
            continue;
        default:
            assert(0);
        }

        gdk_event_free (event);
    }

    /* Draw the scene. */

    glClear(GL_DEPTH_BUFFER_BIT |
            GL_COLOR_BUFFER_BIT |
            GL_STENCIL_BUFFER_BIT);

    t_end_cpu_interval (&self->core);

    frames += 1;

    for (root = [Root nodes] ; root ; root = (Root *)root->right) {
        t_draw_subtree (root, frames);
    }
}

-(int) _get_window
{
    if (have_context) {
        lua_newtable (_L);
        lua_pushinteger (_L, width);
        lua_rawseti (_L, -2, 1);
        lua_pushinteger (_L, height);
        lua_rawseti (_L, -2, 2);
    } else {
        lua_pushnil(_L);
    }

    return 1;
}

-(int) _get_hide
{
    lua_pushboolean (_L, hide);

    return 1;
}

-(int) _get_title
{
    lua_pushstring (_L, title);

    return 1;
}

-(int) _get_decorate
{
    lua_pushboolean (_L, decorate);

    return 1;
}

-(int) _get_cursor
{
    lua_pushboolean (_L, cursor);

    return 1;
}

-(int) _get_grabinput
{
    lua_pushboolean (_L, gdk_pointer_is_grabbed ());

    return 1;
}

-(int) _get_canvas
{
    if (have_context) {
        double canvas[4];

        glGetDoublev (GL_COLOR_CLEAR_VALUE, canvas);
        array_createarray (_L, ARRAY_TDOUBLE, canvas, 1, 3);
    } else {
        lua_pushnil(_L);
    }

    return 1;
}

-(int) _get_colorbuffer
{
    if (have_context) {
        array_Array *pixels;
        int v[4];

        glGetIntegerv (GL_VIEWPORT, v);

        /* glReadBuffer (GL_FRONT); */

        pixels = array_createarray(_L, ARRAY_TNUCHAR, NULL, 3, v[3], v[2], 4);
        glReadPixels(v[0], v[1], v[2], v[3], GL_RGBA,
                     GL_UNSIGNED_BYTE, pixels->values.any);
    } else {
        lua_pushnil(_L);
    }

    return 1;
}

-(int) _get_perspective
{
    if (have_context) {
        int i;

        if (projection == PERSPECTIVE) {
            lua_newtable(_L);

            for(i = 0; i < 6; i += 1) {
                lua_pushnumber(_L, planes[i]);
                lua_rawseti(_L, -2, i + 1);
            }
        } else if (projection == FRUSTUM) {
            lua_newtable(_L);

            for(i = 0; i < 3; i += 1) {
                lua_pushnumber(_L, frustum[i]);
                lua_rawseti(_L, -2, i + 1);
            }
        } else {
            lua_pushnil (_L);
        }
    } else {
        lua_pushnil(_L);
    }

    return 1;
}

-(int) _get_orthographic
{
    if (have_context) {
        int i;

        if (projection == ORTHOGRAPHIC) {
            lua_newtable(_L);

            for(i = 0; i < 6; i += 1) {
                lua_pushnumber(_L, planes[i]);
                lua_rawseti(_L, -2, i + 1);
            }
        } else {
            lua_pushnil (_L);
        }
    } else {
        lua_pushnil(_L);
    }

    return 1;
}

-(int) _get_configure
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, configure);

    return 1;
}

-(int) _get_focus
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, focus);

    return 1;
}

-(int) _get_defocus
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, defocus);

    return 1;
}

-(int) _get_close
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, delete);

    return 1;
}

-(int) _get_renderer
{
    if (have_context) {
        lua_pushstring (_L, (const char *)glGetString(GL_RENDERER));
        lua_pushstring (_L, " ");
        lua_pushstring (_L, (const char *)glGetString(GL_VERSION));
        lua_concat (_L, 3);
    } else {
        lua_pushnil(_L);
    }

    return 1;
}

-(int) _get_screen
{
    lua_newtable (_L);
    lua_pushinteger (_L, gdk_screen_width());
    lua_rawseti (_L, -2, 1);
    lua_pushinteger (_L, gdk_screen_height());
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_latency
{
    if (have_context && self->latency.frames > 0) {
        lua_createtable(_L, 2, 0);
        lua_pushnumber(_L, self->latency.interval * 1e-9 / self->latency.frames);
        lua_rawseti(_L, -2, 1);
        lua_pushinteger(_L, self->latency.frames);
        lua_rawseti(_L, -2, 2);
    } else {
        lua_pushnil(_L);
    }

    return 1;
}

-(int) _get_redbits
{
    lua_pushinteger(_L, redbits);

    return 1;
}

-(int) _get_greenbits
{
    lua_pushinteger(_L, greenbits);

    return 1;
}

-(int) _get_bluebits
{
    lua_pushinteger(_L, bluebits);

    return 1;
}

-(int) _get_alphabits
{
    lua_pushinteger(_L, alphabits);

    return 1;
}

-(int) _get_depthbits
{
    lua_pushinteger(_L, depthbits);

    return 1;
}

-(int) _get_stencilbits
{
    lua_pushinteger(_L, stencilbits);

    return 1;
}

-(int) _get_samples
{
    lua_pushinteger(_L, samples);

    return 1;
}

-(int) _get_frames
{
    lua_pushinteger(_L, frames);

    return 1;
}

-(void) _set_window
{
    GdkGeometry geometry;

    if (lua_istable (_L, 3)) {
        lua_pushinteger (_L, 1);
        lua_gettable (_L, 3);
        width = lua_tointeger(_L, -1);

        lua_pushinteger (_L, 2);
        lua_gettable (_L, 3);
        height = lua_tointeger(_L, -1);

        lua_pop (_L, 2);

        if (!have_context) {
            GLXFBConfig *configurations;
            GLXContext context;

            int context_attributes[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, 1,
                GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                GLX_CONTEXT_FLAGS_ARB, 0,
                None
            };

            int visual_attributes[] = {
                GLX_X_RENDERABLE,   True,
                GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
                GLX_RENDER_TYPE,    GLX_RGBA_BIT,
                GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
                GLX_RED_SIZE,       redbits,
                GLX_GREEN_SIZE,     greenbits,
                GLX_BLUE_SIZE,      bluebits,
                GLX_ALPHA_SIZE,     alphabits,
                GLX_DEPTH_SIZE,     depthbits,
                GLX_STENCIL_SIZE,   stencilbits,
                GLX_DOUBLEBUFFER,   True,
                GLX_SAMPLE_BUFFERS, samples > 0,
                GLX_SAMPLES,        samples,
                None
            };

            const char *name, *class;

            int i, j;

            /* Get the configuration. */

            lua_getglobal (_L, "options");

            /* The window manager class. */

            lua_getfield (_L, -1, "wmclass");

            if (lua_istable (_L, -1)) {
                lua_rawgeti (_L, -1, 1);
                name = lua_tostring (_L, -1);

                lua_rawgeti (_L, -1, 2);
                class = lua_tostring (_L, -1);

                lua_pop (_L, 2);
            } else {
                name = "techne";
                class = "Techne";
            }

            lua_pop (_L, 1);

            /* Do we want an on-screen window? */

            lua_getfield (_L, -1, "offscreen");
            offscreen = lua_toboolean (_L, -1);
            lua_pop (_L, 1);

            /* The graphics context options. */

            lua_getfield (_L, -1, "context");

            arbcontext = lua_toboolean (_L, -1);

            if (lua_type(_L, -1) != LUA_TBOOLEAN) {
                int n, i, isnumber;
                const char *s;

                /* Encapsulate into a table if needed. */

                if (lua_type(_L, -1) != LUA_TTABLE) {
                    lua_newtable(_L);
                    lua_insert (_L, -2);
                    lua_rawseti (_L, -2, 1);
                }

                n = lua_rawlen (_L, -1);

                for (i = 0 ; i < n ; i += 1) {
                    lua_rawgeti (_L, -1, i + 1);

                    s = lua_tostring (_L, -1);
                    lua_tonumberx (_L, -1, &isnumber);

                    if (isnumber) {
                        char *dot;

                        dot = strchr (s, '.');

                        if (dot) {
                            *dot = '\0';

                            context_attributes[3] = atoi(dot + 1);
                        }

                        context_attributes[1] = atoi(s);

                        /* Forward-compatible contexts are only defined for
                         * versions 3.0 and later. */

                        if (context_attributes[1] >= 3) {
                            context_attributes[5] |=
                                GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
                        }
                    } else if (!strcmp (s, "debug")) {
                        debug = 1;

                        context_attributes[5] |= GLX_CONTEXT_DEBUG_BIT_ARB;
                    } else if (!strcmp (s, "reportonce")) {
                        reportonce = 1;
                    } else if (!strcmp (s, "break")) {
                        interrupt = 1;
                    }

                    lua_pop (_L, 1);
                }
            }

            lua_pop (_L, 2);

            /* Create the rendering context and associated visual. */
            /* Check the version. */

            if (!glXQueryVersion(GDK_DISPLAY_XDISPLAY(display), &i, &j) ||
                (i == 1 && j < 4) || i < 1) {
                t_print_error ("This GLX version is not supported.\n");
                exit(1);
            }

            /* Query the framebuffer configurations. */

            configurations = glXChooseFBConfig(GDK_DISPLAY_XDISPLAY(display),
                                               GDK_SCREEN_XNUMBER(screen),
                                               visual_attributes, &j);
            if (!configurations) {
                t_print_error("Failed to retrieve any framebuffer configurations.\n");
                exit(1);
            }

            t_print_message("Found %d matching framebuffer configurations.\n", j);

            /* Choose a configuration at random and create a GLX context. */

            configuration = configurations[0];

            if (arbcontext) {
                context = glXCreateContextAttribsARB(GDK_DISPLAY_XDISPLAY(display),
                                                     configuration, 0,
                                                     True, context_attributes);
            } else {
                context = glXCreateNewContext(GDK_DISPLAY_XDISPLAY(display),
                                              configuration, GLX_RGBA_TYPE,
                                              0, True);
            }

            gdk_display_sync(display);

            if (!context) {
                t_print_error ("I could not create a rendering context.\n");
                exit(1);
            }

            /* Create the rendering surface. */

            if (offscreen) {
                int pbuffer_attributes[] = {
                    GLX_PBUFFER_WIDTH,  1920,
                    GLX_PBUFFER_HEIGHT, 1080,
                    None
                };

                pbuffer = glXCreatePbuffer(GDK_DISPLAY_XDISPLAY(display),
                                           configuration,
                                           pbuffer_attributes);

                XSync(GDK_DISPLAY_XDISPLAY(display), False);

                glXMakeContextCurrent(GDK_DISPLAY_XDISPLAY(display),
                                      pbuffer, pbuffer, context);
            } else {
                GdkWindowAttr window_attributes;
                GdkVisual *visual;
                XVisualInfo *info;

                /* Get the visual. */

                info = glXGetVisualFromFBConfig(GDK_DISPLAY_XDISPLAY(display),
                                                configuration);
                visual = gdk_x11_screen_lookup_visual (screen, info->visualid);

                window_attributes.window_type = GDK_WINDOW_TOPLEVEL;
                window_attributes.wclass = GDK_INPUT_OUTPUT;
                window_attributes.wmclass_name = (char *)name;
                window_attributes.wmclass_class = (char *)class;
                window_attributes.colormap = gdk_colormap_new(visual, FALSE);
                window_attributes.visual = visual;
                window_attributes.width = width;
                window_attributes.height = height;
                window_attributes.event_mask = (GDK_STRUCTURE_MASK |
                                                GDK_FOCUS_CHANGE_MASK |
                                                GDK_BUTTON_PRESS_MASK |
                                                GDK_BUTTON_RELEASE_MASK |
                                                GDK_KEY_PRESS_MASK |
                                                GDK_KEY_RELEASE_MASK |
                                                GDK_POINTER_MOTION_MASK |
                                                GDK_SCROLL_MASK);

                window = gdk_window_new(gdk_get_default_root_window(),
                                        &window_attributes,
                                        GDK_WA_COLORMAP | GDK_WA_VISUAL |
                                        GDK_WA_WMCLASS);

                glXMakeContextCurrent(GDK_DISPLAY_XDISPLAY(display),
                                      GDK_WINDOW_XWINDOW(window),
                                      GDK_WINDOW_XWINDOW(window),
                                      context);

                XFree(info);
            }

            /* Query the bound context. */

            if (arbcontext) {
                int major, minor;

                glGetIntegerv(GL_MAJOR_VERSION, &major);
                glGetIntegerv(GL_MINOR_VERSION, &minor);

                t_print_message("A version %d.%d, ARB GLX context was created.\n", major, minor);
            } else {
                t_print_message("An old-style GLX context was created.\n");
            }

            if (debug) {
                glDebugMessageCallbackARB (&debug_callback, NULL);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);

                /* glDebugMessageControlARB (GL_DONT_CARE, */
                /*                        GL_DONT_CARE, */
                /*                        GL_DEBUG_SEVERITY_HIGH_ARB, */
                /*                        0, NULL, */
                /*                        high ? GL_TRUE : GL_FALSE); */

                /* glDebugMessageControlARB (GL_DONT_CARE, */
                /*                        GL_DONT_CARE, */
                /*                        GL_DEBUG_SEVERITY_MEDIUM_ARB, */
                /*                        0, NULL, */
                /*                        medium ? GL_TRUE : GL_FALSE); */

                /* glDebugMessageControlARB (GL_DONT_CARE, */
                /*                        GL_DONT_CARE, */
                /*                        GL_DEBUG_SEVERITY_LOW_ARB, */
                /*                        0, NULL, */
                /*                        low ? GL_TRUE : GL_FALSE); */
            }

            /* Print useful debug information. */

            t_print_message ("The graphics renderer is: '%s %s'.\n",
                             glGetString(GL_RENDERER), glGetString(GL_VERSION));
            t_print_message ("The shading language version is: '%s'.\n",
                             glGetString(GL_SHADING_LANGUAGE_VERSION));

            glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                                  GLX_RED_SIZE, &redbits);

            glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                                  GLX_GREEN_SIZE, &greenbits);

            glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                                  GLX_BLUE_SIZE, &bluebits);

            glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                                  GLX_ALPHA_SIZE, &alphabits);

            glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                                  GLX_DEPTH_SIZE, &depthbits);

            glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                                  GLX_STENCIL_SIZE, &stencilbits);

            glXGetFBConfigAttrib (GDK_DISPLAY_XDISPLAY(display), configuration,
                                  GLX_SAMPLES, &samples);

            t_print_message ("The configuration of the framebuffer is "
                             "[%d, %d, %d, %d, %d, %d, %d].\n",
                             redbits, greenbits, bluebits, alphabits,
                             depthbits, stencilbits, samples);

            t_print_message ("The rendering context is%sdirect.\n",
                             glXIsDirect (GDK_DISPLAY_XDISPLAY(display), context) ?
                             " " : " *not* ");

            XFree(configurations);

            /* Enable multisampling if we have a suitable context. */

            if (samples > 0) {
                glEnable(GL_MULTISAMPLE);
            }

            {
#include "glsl/transform_block.h"

                double I[16];

                /* Register the transform uniform block. */

                buffer = t_add_global_block ("__transform_block", glsl_transform_block);

                /* Create the global shading context buffer object. */

                glBindBuffer(GL_UNIFORM_BUFFER, buffer);
                glBufferData(GL_UNIFORM_BUFFER, UNIFORM_BUFFER_SIZE, NULL,
                             GL_DYNAMIC_DRAW);

                /* Initialize the values. */

                t_load_identity_4 (I);
                t_load_projection (I);
                t_load_modelview (I, T_LOAD);
            }

            have_context = 1;
        }

        if (offscreen) {
            GLXContext context;
            int pbuffer_attributes[] = {
                GLX_PBUFFER_WIDTH,  width,
                GLX_PBUFFER_HEIGHT, height,
                None
            };

            context = glXGetCurrentContext ();
            glXMakeContextCurrent(GDK_DISPLAY_XDISPLAY(display),
                                  None, None, NULL);
            glXDestroyPbuffer(GDK_DISPLAY_XDISPLAY(display), pbuffer);

            pbuffer = glXCreatePbuffer(GDK_DISPLAY_XDISPLAY(display),
                                       configuration,
                                       pbuffer_attributes);

            glXMakeContextCurrent(GDK_DISPLAY_XDISPLAY(display),
                                  pbuffer, pbuffer, context);

            XSync(GDK_DISPLAY_XDISPLAY(display), False);
        } else {
            gdk_window_hide(window);
            gdk_window_unfullscreen (window);
            gdk_window_set_geometry_hints (window, NULL, 0);
            gdk_window_resize (window, width, height);

            geometry.min_width = width;
            geometry.min_height = height;
            geometry.max_width = width;
            geometry.max_height = height;

            gdk_window_set_geometry_hints(window, &geometry,
                                          GDK_HINT_MIN_SIZE |
                                          GDK_HINT_MAX_SIZE);

            if (!hide) {
                gdk_window_clear (window);
                gdk_window_show (window);
                gdk_window_raise (window);
                /* gdk_window_focus (window, GDK_CURRENT_TIME); */

                if (width == gdk_screen_width() &&
                    height == gdk_screen_height()) {
                    gdk_window_fullscreen (window);
                }
            }

            /* Always flush when you're done. */

            gdk_window_flush (window);
            gdk_flush ();
        }
    }

    /* Clear the window's color buffers to the
       canvas color. */

    glDrawBuffer (GL_FRONT_AND_BACK);
    glClear (GL_COLOR_BUFFER_BIT);
    glFlush();
    glDrawBuffer (GL_BACK);
}

-(void) _set_hide
{
    /* Update the window if the state has toggled. */

    if (window) {
        if (!hide && lua_toboolean (_L, 3)) {
            gdk_window_hide (window);
            gdk_flush();
        }

        if (hide && !lua_toboolean (_L, 3)) {
            gdk_window_show (window);
            gdk_window_raise (window);
            /* gdk_window_focus (window, GDK_CURRENT_TIME); */

            if (width == gdk_screen_width() &&
                height == gdk_screen_height()) {
                gdk_window_fullscreen (window);
            }

            gdk_flush();

            /* Clear the window's color buffers to the
               canvas color. */

            glDrawBuffer (GL_FRONT_AND_BACK);
            glClear (GL_COLOR_BUFFER_BIT);
            glFlush();
            glDrawBuffer (GL_BACK);
        }

        gdk_window_flush (window);
    }

    hide = lua_toboolean (_L, 3);
}

-(void) _set_title
{
    if (window) {
        gdk_window_set_title (window, (char *) lua_tostring (_L, 3));
    }
}

-(void) _set_decorate
{
    if (window) {
        decorate = lua_toboolean (_L, 3);
        gdk_window_set_override_redirect (window, !decorate);
    }
}

-(void) _set_grabinput
{
    if (window) {
        if (lua_toboolean (_L, 3)) {
            gdk_pointer_grab (window, TRUE,
                              GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK,
                              NULL, NULL, GDK_CURRENT_TIME);
        } else {
            gdk_pointer_ungrab (GDK_CURRENT_TIME);
        }
    }
}

-(void) _set_cursor
{
    if (window) {
        GdkCursor *new;

        cursor = lua_toboolean (_L, 3);

        new = gdk_cursor_new (cursor ? GDK_ARROW : GDK_BLANK_CURSOR);
        gdk_window_set_cursor (window, new);
        gdk_cursor_destroy(new);
    }
}

-(void) _set_perspective
{
    int i, n;

    n = lua_rawlen(_L, 3);

    if (n == 3) {
        for (i = 0 ; i < 3 ; i += 1) {
            lua_pushinteger (_L, i + 1);
            lua_gettable (_L, 3);
            frustum[i] = lua_tonumber(_L, -1);
            lua_pop (_L, 1);
        }

        projection = FRUSTUM;
    } else {
        for (i = 0 ; i < 6 ; i += 1) {
            lua_pushinteger (_L, i + 1);
            lua_gettable (_L, 3);
            planes[i] = lua_tonumber(_L, -1);
            lua_pop (_L, 1);
        }

        projection = PERSPECTIVE;
    }

    update_projection();
}

-(void) _set_orthographic
{
    int i;

    for (i = 0 ; i < 6 ; i += 1) {
        lua_pushinteger (_L, i + 1);
        lua_gettable (_L, 3);
        planes[i] = lua_tonumber(_L, -1);
        lua_pop (_L, 1);
    }

    projection = ORTHOGRAPHIC;
    update_projection();
}

-(void) _set_canvas
{
    array_Array *array;

    array = array_checkcompatible (_L, 3,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);

    glClearColor (array->values.doubles[0],
                  array->values.doubles[1],
                  array->values.doubles[2],
                  0);
}

-(void) _set_colorbuffer
{
    T_WARN_READONLY;
}

-(void) _set_configure
{
    luaL_unref (_L, LUA_REGISTRYINDEX, configure);
    configure = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_focus
{
    luaL_unref (_L, LUA_REGISTRYINDEX, focus);
    focus = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_defocus
{
    luaL_unref (_L, LUA_REGISTRYINDEX, defocus);
    defocus = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_close
{
    luaL_unref (_L, LUA_REGISTRYINDEX, delete);
    delete = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_renderer
{
    T_WARN_READONLY;
}

-(void) _set_screen
{
    T_WARN_READONLY;
}

-(void) _set_latency
{
    T_WARN_READONLY;
}

-(void) _set_redbits
{
    if (have_context) {
        T_WARN_READONLY;
    } else {
        redbits = lua_tointeger(_L, 3);
    }
}

-(void) _set_greenbits
{
    if (have_context) {
        T_WARN_READONLY;
    } else {
        greenbits = lua_tointeger(_L, 3);
    }
}

-(void) _set_bluebits
{
    if (have_context) {
        T_WARN_READONLY;
    } else {
        bluebits = lua_tointeger(_L, 3);
    }
}

-(void) _set_alphabits
{
    if (have_context) {
        T_WARN_READONLY;
    } else {
        alphabits = lua_tointeger(_L, 3);
    }
}

-(void) _set_depthbits
{
    if (have_context) {
        T_WARN_READONLY;
    } else {
        depthbits = lua_tointeger(_L, 3);
    }
}

-(void) _set_stencilbits
{
    if (have_context) {
        T_WARN_READONLY;
    } else {
        stencilbits = lua_tointeger(_L, 3);
    }
}

-(void) _set_samples
{
    if (have_context) {
        T_WARN_READONLY;
    } else {
        samples = lua_tointeger(_L, 3);
    }
}

-(void) _set_frames
{
    T_WARN_READONLY;
}

@end

int luaopen_graphics (lua_State *L)
{
    [[Graphics alloc] init];

    return 1;
}
