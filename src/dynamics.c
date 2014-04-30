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
#include <lualib.h>
#include <lauxlib.h>

#include <ode/ode.h>

#include "joints/contact.h"
#include "array/array.h"
#include "dynamics.h"
#include "techne.h"
#include "body.h"
#include "root.h"

static double timescale = 1, stepsize = 0.01, ceiling = 1.0 / 0.0;
static double interval = -1;
static int iterations = 0, collision = LUA_REFNIL;

static long int once;
static double now, then;

static void *pool;
struct context {
    dContact contact;
    dJointFeedback feedback;
};

static Dynamics* instance;

static int next_joint(lua_State *L)
{
    Body *object;
    Joint *joint;
    int i, n;

    object = t_checknode (_L, 1, [Body class]);
    i = lua_tointeger (L, 2);
    n = dBodyGetNumJoints(object->body);

    if (i < n) {
        dJointID j;

        /* Pop the old key and push the next. */

        lua_pop (L, 1);
        lua_pushinteger (L, i + 1);

        /* Push the full userdata for the joint. */

        j = dBodyGetJoint(object->body, i);
        t_pushuserdata (_L, 1, dJointGetData(j));

        /* If the userdata associated with the node does not
         * correspond with a Joint userdata value then it must be an
         * internally generated contact joint.  We create Lua values
         * for these lazily. */

        if (lua_isnil (L, -1)) {
            struct context *context;

            assert (dJointGetType (j) == dJointTypeContact);
            lua_pop (_L, 1);

            /* Allocate a new Contact object for the internally allocated
             * contact joint. */

            context = (struct context *)dJointGetData(j);

            [[Contact alloc] initWithJoint: j
                                andContact: context->contact
                               andFeedback: context->feedback];
        }
    } else {
        /* We're done. */

        lua_pop (L, 1);
        lua_pushnil (L);
        lua_pushnil (L);
    }

    if (joint) {
    } else {
    }

    return 2;
}

static int joints_iterator(lua_State *L)
{
    lua_pushcfunction (L, next_joint);
    lua_pushvalue (L, 1);
    lua_pushinteger(L, 0);

    return 3;
}

static void callback (void *data, dGeomID a, dGeomID b)
{
    dContactGeom geoms[16];
    dReal mu[2] = {-1, -1}, epsilon = 0, sigma = 0, tau = 1;
    int i, j, n, m;

    if (dGeomIsSpace (a) || dGeomIsSpace (b)) {
        /* Recurse into subspaces as needed. */

        dSpaceCollide2 (a, b, data, &callback);

        if (dGeomIsSpace (a)) {
            dSpaceCollide ((dSpaceID)a, data, &callback);
        }

        if (dGeomIsSpace (b)) {
            dSpaceCollide ((dSpaceID)b, data, &callback);
        }
    } else {
        if (dGeomGetBody (a) || dGeomGetBody (b)) {
            m = n = dCollide (a, b, 16, geoms, sizeof(dContactGeom));

            if (n > 0) {
                /* Create a collision event. */

                if (collision != LUA_REFNIL) {
                    double k_s, k_d;
                    int h;

                    h = lua_gettop (_L);

                    t_pushuserdata (_L, 2,
                                     dGeomGetData(a),
                                     dGeomGetData(b));
                    t_callhook(_L, collision, 2, LUA_MULTRET);

                    /* The maximum number of joints to create. */

                    if (lua_type (_L, h + 1) == LUA_TNUMBER) {
                        m = lua_tonumber (_L, h + 1);

                        if (m > n) {
                            m = n;
                        }
                    }

                    /* The coefficient of restitution. */

                    if (lua_type (_L, h + 2) == LUA_TNUMBER) {
                        epsilon = lua_tonumber (_L, h + 2);
                    }

                    /* The coefficient of friction in the primary
                     * direction. */

                    if (lua_type (_L, h + 3) == LUA_TNUMBER) {
                        mu[0] = lua_tonumber (_L, h + 3);

                        if (isinf(mu[0])) {
                            mu[0] = dInfinity;
                        }
                    }

                    /* The coefficient of friction in the secondary
                     * direction. */

                    if (lua_type (_L, h + 4) == LUA_TNUMBER) {
                        mu[1] = lua_tonumber (_L, h + 4);

                        if (isinf(mu[1])) {
                            mu[1] = dInfinity;
                        }
                    }

                    /* The CFM & ERP. */

                    if (lua_type (_L, h + 5) == LUA_TNUMBER) {
                        k_s = lua_tonumber (_L, h + 5);

                        if (lua_type (_L, h + 6) == LUA_TNUMBER) {
                            k_d = lua_tonumber (_L, h + 6);
                        } else {
                            k_d = 0;
                        }

                        t_convert_from_spring(k_s, k_d, &tau, &sigma);
                    }

                    lua_settop (_L, h);
                }
            }

            for (i = 0, j = 0 ; i < n && j < m ; i += 1) {
                dBodyID p, q;
                dJointID joint;
                struct context *context;
                dVector3 u, v, delta;
                dReal *r, *n, ndotdelta;

                /* Caclualte the relative velocity of the contact
                 * point. */

                r = geoms[i].pos;
                n = geoms[i].normal;
                p = dGeomGetBody (geoms[i].g1);
                q = dGeomGetBody (geoms[i].g2);

                if (p) {
                    dBodyGetPointVel (p, r[0], r[1], r[2], u);
                } else {
                    dSetZero (u, 3);
                }

                if (q) {
                    dBodyGetPointVel (q, r[0], r[1], r[2], v);
                } else {
                    dSetZero (v, 3);
                }

                dOP (delta, -, u, v);
                ndotdelta = dDOT(n, delta);

                /* Only consider this contact if the relative velocity
                 * points inward. */

                if (ndotdelta > 0) {
                    continue;
                }

                /* Initialize a new contact joint with values returned
                 * by the user. */

                context = t_allocate_pooled(pool);

                context->contact.geom = geoms[i];

                context->contact.fdir1[0] = delta[0] - ndotdelta * n[0];
                context->contact.fdir1[1] = delta[1] - ndotdelta * n[1];
                context->contact.fdir1[2] = delta[2] - ndotdelta * n[2];

                dSafeNormalize3(context->contact.fdir1);

                context->contact.surface.mode = 0;

                if (mu[0] >= 0) {
                    context->contact.surface.mode |= dContactApprox1;
                    context->contact.surface.mode |= dContactFDir1;
                    context->contact.surface.mu = mu[0];

                    if (mu[1] >= 0) {
                        context->contact.surface.mode |= dContactMu2;
                        context->contact.surface.mu2 = mu[1];
                    }
                } else {
                    context->contact.surface.mu = 0;
                }

                if (epsilon > 0) {
                    context->contact.surface.mode |= dContactBounce;
                    context->contact.surface.bounce = epsilon;
                    context->contact.surface.bounce_vel = 0.01;
                }

                if (sigma > 0) {
                    context->contact.surface.mode |= dContactSoftCFM;
                    context->contact.surface.soft_cfm = sigma;
                }

                if (tau < 1) {
                    context->contact.surface.mode |= dContactSoftERP;
                    context->contact.surface.soft_erp = tau;
                }

                /* Create and count the joint. */

                joint = dJointCreateContact (_WORLD, _GROUP,
                                             &context->contact);

                dJointSetFeedback (joint, &context->feedback);
                dJointSetData (joint, context);
                dJointAttach (joint,
                              dGeomGetBody (a), dGeomGetBody (b));

                j += 1;
            }
        }
    }
}

void t_convert_from_spring(double k_s, double k_d, double *erp, double *cfm)
{
    *cfm = 1.0 / (stepsize * k_s + k_d);
    *erp = stepsize * k_s / (stepsize * k_s + k_d);
}

void t_convert_to_spring(double erp, double cfm, double *k_s, double *k_d)
{
    *k_s = erp / (cfm * stepsize);
    *k_d = - (erp - 1) / cfm;
}

long long unsigned int t_get_dynamics_time ()
{
    return (long long unsigned int)(now * 1e9);
}

@implementation Dynamics
-(void) init
{
    self->index = 3;

    [super init];

    lua_pushstring (_L, "dynamics");
    lua_setfield (_L, -2, "tag");

    /* Initialize the ODE. */

    dInitODE();

    _WORLD = dWorldCreate();
    _GROUP = dJointGroupCreate (0);
    _SPACE = dSimpleSpaceCreate (NULL);

    dWorldSetAutoDisableFlag (_WORLD, 0);

    /* Get the current real time. */

    once = t_get_real_time();

    /* Create the contact context pool. */

    pool = t_build_pool (64, sizeof (struct context), T_FLUSH_ONLY);

    /* Export the joints iterator. */

    lua_pushcfunction (_L, joints_iterator);
    lua_setglobal (_L, "constraints");

    instance = self;
}

+(Builtin *)instance
{
    return instance;
}

-(void) iterate
{
    Root *root;
    double delta;
    long int time;

    t_begin_cpu_interval (&self->core);

    time = t_get_real_time();

    if (interval < 0) {
        delta = timescale * ((double)(time - once) / 1e9);
    } else {
        delta = timescale * interval;
    }

    /* Calculate the time to advance for this iteration. */

    if (delta > ceiling) {
        /* printf ("Skewing the simulation clock by %.3fs\n", */
        /*          ceiling - delta); */
        now += ceiling;
    } else {
        now += delta;
    }

    /* Simulate the system forward. */

    for (delta = now - then;
         delta >= stepsize;
         then += stepsize, delta -= stepsize) {

        /* Empty the contact group. */

        dJointGroupEmpty (_GROUP);
        t_reset_pool (pool);

        t_pause_cpu_interval (&self->core);

        /* Step the tree. */

        for (root = [Root nodes] ; root ; root = (Root *)root->right) {
            t_step_subtree (root, stepsize, then);
        }

        t_begin_cpu_interval (&self->core);

        if (dSpaceGetNumGeoms (_SPACE) > 1) {
            dSpaceCollide (_SPACE, NULL, callback);
        } else if (dSpaceGetNumGeoms (_SPACE) > 0) {
            dGeomID geom;

            geom = dSpaceGetGeom (_SPACE, 0);

            if (dGeomIsSpace (geom)) {
                dSpaceCollide ((dSpaceID)geom, NULL, callback);
            }
        }

        if (iterations > 0) {
            dWorldQuickStep (_WORLD, stepsize);
        } else {
            dWorldStep (_WORLD, stepsize);
        }
    }

    /* Advance the real-world time. */

    once = time;
    t_end_cpu_interval (&self->core);

    /* Transform the tree to update absolute positions and
       orientations. */

    for (root = [Root nodes] ; root ; root = (Root *)root->right) {
        t_transform_subtree (root);
    }
}

-(int) _get_stepsize
{
    lua_pushnumber(_L, stepsize);

    return 1;
}

-(int) _get_time
{
    lua_pushnumber (_L, now);

    return 1;
}

-(int) _get_ceiling
{
    lua_pushnumber(_L, ceiling);

    return 1;
}

-(int) _get_timescale
{
    lua_pushnumber(_L, timescale);

    return 1;
}

-(int) _get_iterations
{
    lua_pushnumber(_L, iterations);

    return 1;
}

-(int) _get_collision
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, collision);

    return 1;
}

-(int) _get_tolerance
{
    lua_newtable (_L);
    lua_pushnumber (_L, dWorldGetCFM (_WORLD));
    lua_rawseti (_L, -2, 1);
    lua_pushnumber (_L, dWorldGetERP (_WORLD));
    lua_rawseti (_L, -2, 2);

    return 1;
}

-(int) _get_popvelocity
{
    lua_pushnumber(_L, dWorldGetContactMaxCorrectingVel (_WORLD));

    return 1;
}

-(int) _get_surfacelayer
{
    lua_pushnumber(_L, dWorldGetContactSurfaceLayer (_WORLD));

    return 1;
}

-(int) _get_gravity
{
    dVector3 g;

    dWorldGetGravity (_WORLD, g);
    array_createarray (_L, ARRAY_TDOUBLE, g, 1, 3);

    return 1;
}

-(int) _get_interval
{
    if (interval < 0) {
        lua_pushnil (_L);
    } else {
        lua_pushnumber (_L, interval);
    }

    return 1;
}

-(void) _set_interval
{
    if (lua_isnumber (_L, 3)) {
        interval = lua_tonumber (_L, 3);
    } else {
        interval = -1;
    }
}

-(void) _set_time
{
    T_WARN_READONLY;
}

-(void) _set_stepsize
{
    stepsize = lua_tonumber(_L, 3);
}

-(void) _set_ceiling
{
    ceiling = lua_tonumber(_L, 3);
}

-(void) _set_timescale
{
    timescale = lua_tonumber(_L, 3);
}

-(void) _set_iterations
{
    iterations = lua_tonumber(_L, 3);

    dWorldSetQuickStepNumIterations (_WORLD, iterations);
}

-(void) _set_collision
{
    luaL_unref(_L, LUA_REGISTRYINDEX, collision);
    collision = luaL_ref(_L, LUA_REGISTRYINDEX);
}

-(void) _set_tolerance
{
    lua_rawgeti (_L, 3, 1);
    dWorldSetCFM (_WORLD, lua_tonumber (_L, -1));
    lua_rawgeti (_L, 3, 2);
    dWorldSetERP (_WORLD, lua_tonumber (_L, -1));
    lua_pop(_L, 2);
}

-(void) _set_popvelocity
{
    dWorldSetContactMaxCorrectingVel (_WORLD, lua_tonumber(_L, 3));
}

-(void) _set_surfacelayer
{
    dWorldSetContactSurfaceLayer (_WORLD, lua_tonumber(_L, 3));
}

-(void) _set_gravity
{
    array_Array *g;

    if(!lua_isnil(_L, 3)) {
        g = array_checkcompatible (_L, 3,
                                   ARRAY_TYPE | ARRAY_RANK | ARRAY_SIZE,
                                   ARRAY_TDOUBLE, 1, 3);

        dWorldSetGravity (_WORLD,
                          g->values.doubles[0],
                          g->values.doubles[1],
                          g->values.doubles[2]);
    } else {
        dWorldSetGravity (_WORLD, 0, 0, 0);
    }
}
@end

int luaopen_dynamics (lua_State *L)
{
    [[Dynamics alloc] init];

    return 1;
}
