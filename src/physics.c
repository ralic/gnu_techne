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
#include <ode/ode.h>

#include "array/array.h"
#include "techne.h"
#include "dynamics.h"
#include "body.h"
#include "joint.h"

static int addforce (lua_State *L)
{
    Body *object;
    array_Array *F, *p;

    object = *(Body **)lua_touserdata (L, 1);

    if (!lua_isnil (L, 2) && object->body) {
	F = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);
	
	if (!lua_isnoneornil (L, 3)) {
	    p = array_checkcompatible (L, 3, ARRAY_TDOUBLE, 1, 3);
	
	    dBodyAddForceAtPos (object->body,
				F->values.doubles[0],
				F->values.doubles[1],
				F->values.doubles[2],
				p->values.doubles[0],
				p->values.doubles[1],
				p->values.doubles[2]);
	} else {
	    dBodyAddForce (object->body,
			   F->values.doubles[0],
			   F->values.doubles[1],
			   F->values.doubles[2]);
	}

	dBodyEnable(object->body);
    }

    return 0;
}

static int addrelativeforce (lua_State *L)
{
    Body *object;
    array_Array *F, *p;

    object = *(Body **)lua_touserdata (L, 1);

    if (!lua_isnil (L, 2) && object->body) {
	F = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);
	
	if (!lua_isnoneornil (L, 3)) {
	    p = array_checkcompatible (L, 3, ARRAY_TDOUBLE, 1, 3);
	
	    dBodyAddRelForceAtRelPos (object->body,
				      F->values.doubles[0],
				      F->values.doubles[1],
				      F->values.doubles[2],
				      p->values.doubles[0],
				      p->values.doubles[1],
				      p->values.doubles[2]);
	} else {
	    dBodyAddRelForce (object->body,
			      F->values.doubles[0],
			      F->values.doubles[1],
			      F->values.doubles[2]);
	}

	dBodyEnable(object->body);
    }

    return 0;
}

static int addtorque (lua_State *L)
{
    Body *object;
    array_Array *T;

    object = *(Body **)lua_touserdata (L, 1);

    if (!lua_isnil (L, 2) && object->body) {
	T = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);
	
	dBodyAddTorque (object->body,
			T->values.doubles[0],
			T->values.doubles[1],
			T->values.doubles[2]);
	
	dBodyEnable(object->body);
    }

    return 0;
}

static int addrelativetorque (lua_State *L)
{
    Body *object;
    array_Array *T;

    object = *(Body **)lua_touserdata (L, 1);

    if (!lua_isnil (L, 2) && object->body) {
	T = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);
	
	dBodyAddRelTorque (object->body,
			T->values.doubles[0],
			T->values.doubles[1],
			T->values.doubles[2]);
	
	dBodyEnable(object->body);
    }

    return 0;
}

static int addjointtorque (lua_State *L)
{
    Joint *object;

    object = t_check_node (L, 1, [Joint class]);

    switch (dJointGetType (object->joint)) {
    case dJointTypeHinge:
	dJointAddHingeTorque (object->joint, luaL_checknumber (L, 2));
	break;
    default:
	t_print_error ("Not implemented.");
	abort();
    }
    
    return 0;
}

static void pushmass (lua_State *L, dMass *mass)
{
    double *I, *c;
    int i, j;
    
    lua_newtable(L);

    /* Mass. */

    lua_pushnumber(L, mass->mass);
    lua_rawseti(L, -2, 1);
    
    /* Center of mass. */
	
    array_createarray (L, ARRAY_TDOUBLE, NULL, 1, 3);
    c = ((array_Array *)lua_touserdata (L, -1))->values.doubles;

    for(i = 0 ; i < 3 ; i += 1) {
	c[i] = mass->c[i];
    }

    lua_rawseti(L, -2, 2);

    /* Inertia. */

    array_createarray (L, ARRAY_TDOUBLE, NULL, 2, 3, 3);
    I = ((array_Array *)lua_touserdata (L, -1))->values.doubles;

    for(i = 0 ; i < 3 ; i += 1) {
	for(j = 0 ; j < 3 ; j += 1) {
	    I[i * 3 + j] = mass->I[i * 4 + j];
	}
    }

    lua_rawseti(L, -2, 3);
}

static void tomass (lua_State *L, int t, dMass *mass)
{    
    array_Array *array;
    double *I, *c, m;

    dMassSetZero(mass);
	
    if(lua_istable (L, t)) {
	/* Mass. */
	
	lua_rawgeti (L, 3, 1);
	m = lua_tonumber (L, -1);

	/* Center of mass. */
	
	lua_rawgeti (L, 3, 2);	
	array = array_checkcompatible (L, -1, ARRAY_TDOUBLE, 1, 3);
	c = array->values.doubles;

	/* Inertia. */
	
	lua_rawgeti (L, 3, 3);
	array = array_checkcompatible (L, -1, ARRAY_TDOUBLE, 2, 3, 3);
	I = array->values.doubles;

	dMassSetParameters (mass, m,
			    c[0], c[1], c[2],
			    I[0], I[4], I[8],
			    I[1], I[2], I[5]);
	
	lua_settop (L, 3);
    }
}

static int adjustmass (lua_State *L)
{
    dMass mass;
    dReal m;
    
    luaL_checktype (L, 1, LUA_TTABLE);
    m = (dReal)luaL_checknumber (L, 2);

    tomass (L, 1, &mass);
    dMassAdjust (&mass, m);
    pushmass (L, &mass);

    return 1;
}

static int translatemass (lua_State *L)
{
    dMass mass;
    array_Array *r;

    luaL_checktype (L, 1, LUA_TTABLE);
    r = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);

    tomass (L, 1, &mass);
    dMassTranslate (&mass,
		    r->values.doubles[0],
		    r->values.doubles[1],
		    r->values.doubles[2]);
    
    pushmass (L, &mass);

    return 1;
}

static int rotatemass (lua_State *L)
{
    dMass mass;
    array_Array *A;
    dMatrix3 R;
    int i, j;

    luaL_checktype (L, 1, LUA_TTABLE);
    A = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);

    for(j = 0; j < 3 ; j += 1) {
	for(i = 0; i < 3 ; i += 1) {
	    R[4 * j + i] = A->values.doubles[3 * j + i + 1];
	}
    }

    tomass (L, 1, &mass);
    dMassRotate (&mass, R);
    pushmass (L, &mass);

    return 1;
}

static int spheremass (lua_State *L)
{
    dMass mass;
    dReal rho, r;

    rho = (dReal)luaL_checknumber (L, 1);
    r = (dReal)luaL_checknumber (L, 2);

    dMassSetSphere (&mass, rho, r);
    pushmass (L, &mass);

    return 1;
}

static int boxmass (lua_State *L)
{
    dMass mass;
    dReal rho, a, b, c;

    rho = (dReal)luaL_checknumber (L, 1);
    a = (dReal)luaL_checknumber (L, 2);
    b = (dReal)luaL_checknumber (L, 3);
    c = (dReal)luaL_checknumber (L, 4);

    dMassSetBox (&mass, rho, a, b, c);
    pushmass (L, &mass);

    return 1;
}

static int capsulemass (lua_State *L)
{
    dMass mass;
    dReal rho, l, r;

    rho = (dReal)luaL_checknumber (L, 1);
    r = (dReal)luaL_checknumber (L, 2);
    l = (dReal)luaL_checknumber (L, 3);

    dMassSetCapsule (&mass, rho, 3, r, l);
    
    pushmass (L, &mass);

    return 1;
}

static int cylindermass (lua_State *L)
{
    dMass mass;
    dReal rho, l, r;

    rho = (dReal)luaL_checknumber (L, 1);
    r = (dReal)luaL_checknumber (L, 2);
    l = (dReal)luaL_checknumber (L, 3);

    dMassSetCylinder (&mass, rho, 3, r, l);
    
    pushmass (L, &mass);

    return 1;
}

static int spring (lua_State *L)
{
    dReal k_d, k_s, h;

    k_s = (dReal)luaL_checknumber (L, 1);
    k_d = (dReal)luaL_checknumber (L, 2);

    lua_getglobal (L, "dynamics");
    lua_getfield (L, -1, "stepsize");
    
    h = (dReal)lua_tonumber (L, -1);

    lua_newtable (L);
    lua_pushnumber (L, 1.0 / (h * k_s + k_d));
    lua_rawseti (L, -2, 1);
    lua_pushnumber (L, h * k_s / (h * k_s + k_d));
    lua_rawseti (L, -2, 2);

    return 1;
}

static int contact (lua_State *L)
{
    dContact contact;
    dJointID j;
    Body *a, *b;
    dReal mu, depth, epsilon, sigma, tau;
    array_Array *pos, *normal;
    int i, simple;

    simple = lua_toboolean (L, lua_upvalueindex (1));

    a = t_check_node (L, 1, [Body class]);
    b = t_check_node (L, 2, [Body class]);
    pos = array_checkcompatible (L, 3, ARRAY_TDOUBLE, 1, 3);
    normal = array_checkcompatible (L, 4, ARRAY_TDOUBLE, 1, 3);

    depth = (dReal)luaL_checknumber (L, 5);
    mu = (dReal)luaL_optnumber (L, 6, 0);
    epsilon = (dReal)luaL_optnumber (L, 7, 0);
    sigma = (dReal)luaL_optnumber (L, 8, 0);
    tau = (dReal)luaL_optnumber (L, 9, 1);

    /* Set surface parameters. */
    
    contact.surface.mode = 0;
		    
    if (mu > 0) {
	if (!simple) {
	    contact.surface.mode |= dContactApprox1;
	}
	
	contact.surface.mode |= dContactFDir1;
	contact.surface.mu = mu;
    }

    if (epsilon > 0) {
	contact.surface.mode |= dContactBounce;
	contact.surface.bounce = epsilon;
	contact.surface.bounce_vel = 0.01;
    }

    if (sigma > 0) {
	contact.surface.mode |= dContactSoftCFM;
	contact.surface.soft_cfm = sigma;
    }

    if (tau < 1) {
	contact.surface.mode |= dContactSoftERP;
	contact.surface.soft_erp = tau;
    }

    for (i = 0 ; i < 3; i += 1) {
	contact.geom.pos[i] = pos->values.doubles[i];
	contact.geom.normal[i] = normal->values.doubles[i];
    }

    dSafeNormalize3 (contact.geom.normal);

    {
	dBodyID p, q;
	dVector3 u, v, delta;
	dReal *r, *n, ndotdelta;

	r = pos->values.doubles;
	n = normal->values.doubles;
	p = a->body;
	q = b->body;
		
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

	/* Only consider this contact if the
	   relative velocity points inward. */

	if (simple || ndotdelta <= 0) {
	    contact.fdir1[0] = delta[0] - ndotdelta * n[0];
	    contact.fdir1[1] = delta[1] - ndotdelta * n[1];
	    contact.fdir1[2] = delta[2] - ndotdelta * n[2];

	    dSafeNormalize3(contact.fdir1);
    
	    contact.geom.depth = depth;
	    contact.geom.g1 = a->geom;
	    contact.geom.g2 = b->geom;

	    j = dJointCreateContact (_WORLD, _GROUP, &contact);

	    dJointAttach (j, a->body, b->body);
	}
    }

    return 0;
}

static int joined (lua_State *L)
{
    Body *a, *b;

    a = t_check_node (L, 1, [Body class]);
    b = t_check_node (L, 2, [Body class]);

    if (a->body && b->body) {
	lua_pushboolean (L, dAreConnected(a->body, b->body));
    } else {
	lua_pushboolean (L, 0);
    }
    
    return 1;
}

static int pointfrombody(lua_State *L)
{
    Body *object;
    dBodyID body;
    dVector3 w;
    array_Array *b;
    int i;

    object = t_check_node (L, 1, [Body class]);
    b = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);
    body = object->body;

    dBodyGetRelPointPos (body,
			 b->values.doubles[0],
			 b->values.doubles[1],
			 b->values.doubles[2],
			 w);
    
    lua_newtable(L);
    
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber(L, w[i]);
	lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int vectorfrombody(lua_State *L)
{
    Body *object;
    dVector3 w;
    array_Array *b;
    int i;

    object = t_check_node (L, 1, [Body class]);
    b = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);

    dBodyVectorToWorld (object->body, b->values.doubles[0], b->values.doubles[1], b->values.doubles[2], w);
    
    lua_newtable(L);
    
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber(L, w[i]);
	lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int pointtobody(lua_State *L)
{
    Body *object;
    dVector3 w;
    array_Array *b;
    int i;

    object = t_check_node (L, 1, [Body class]);
    b = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);

    dBodyGetPosRelPoint (object->body,
			 b->values.doubles[0],
			 b->values.doubles[1],
			 b->values.doubles[2],
			 w);
    
    lua_newtable(L);
    
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber(L, w[i]);
	lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int vectortobody(lua_State *L)
{
    Body *object;
    dVector3 w;
    array_Array *b;
    int i;

    object = t_check_node (L, 1, [Body class]);
    b = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);

    dBodyVectorFromWorld (object->body,
			  b->values.doubles[0],
			  b->values.doubles[1],
			  b->values.doubles[2],
			  w);
    
    lua_newtable(L);
    
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber(L, w[i]);
	lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int pointvelocity(lua_State *L)
{
    Body *object;
    dVector3 w;
    array_Array *b;
    int i;

    object = t_check_node (L, 1, [Body class]);
    b = array_checkcompatible (L, 2, ARRAY_TDOUBLE, 1, 3);

    dBodyGetPointVel (object->body,
		      b->values.doubles[0],
		      b->values.doubles[1],
		      b->values.doubles[2],
		      w);
    
    lua_newtable(L);
    
    for(i = 0; i < 3; i += 1) {
	lua_pushnumber(L, w[i]);
	lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int anaesthetize (lua_State *L)
{
    Body *object;

    object = t_check_node (L, 1, [Body class]);

    if (object->body) {
	dBodyDisable (object->body);
    }
    
    return 0;
}

static int wake (lua_State *L)
{
    Body *object;

    object = t_check_node (L, 1, [Body class]);

    if (object->body) {
	dBodyEnable (object->body);
    }
    
    return 0;
}

static int reattach (lua_State *L)
{
    Joint *object;

    object = t_check_node (L, 1, [Joint class]);
    [object update];
    
    /* [object toggle]; */
    /* [object toggle]; */
    
    return 0;
}

int luaopen_physics (lua_State *L)
{
    const luaL_Reg physics[] = {
	{"sleep", anaesthetize},
	{"wake", wake},
	{"addforce", addforce},
	{"addjointtorque", addjointtorque},
	{"addtorque", addtorque},
	{"addrelativeforce", addrelativeforce},
	{"addrelativetorque", addrelativetorque},
	{"adjustmass", adjustmass},
	{"translatemass", translatemass},
	{"rotatemass", rotatemass},
	{"spheremass", spheremass},
	{"boxmass", boxmass},
	{"capsulemass", capsulemass},
	{"cylindermass", cylindermass},
	{"spring", spring},
	{"joined", joined},
	{"pointfrombody", pointfrombody},
	{"pointtobody", pointtobody},
	{"vectorfrombody", vectorfrombody},
	{"vectortobody", vectortobody},
	{"pointvelocity", pointvelocity},
	{"reattach", reattach},
	{NULL, NULL}
    };

    lua_newtable (L);
    luaL_setfuncs (L, physics, 0);

    lua_pushstring (L, "addcontact");
    lua_pushboolean (L, 0);
    lua_pushcclosure (L, contact, 1);
    lua_settable (L, -3);

    lua_pushstring (L, "addsimplecontact");
    lua_pushboolean (L, 1);
    lua_pushcclosure (L, contact, 1);
    lua_settable (L, -3);

    return 1;
}
