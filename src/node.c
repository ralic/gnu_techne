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
#include <lualib.h>
#include <lauxlib.h>
#include <ctype.h>
#include <math.h>

#include <objc/runtime.h>

#include "node.h"
#include "techne.h"

#define GET_PREFIX "_get_"
#define SET_PREFIX "_set_"

#define GET_PREFIX_LENGTH (sizeof ("_get_") - 1)
#define SET_PREFIX_LENGTH (sizeof ("_set_") - 1)

#define GET_ELEMENT "_get_element"
#define SET_ELEMENT "_set_element"

#define GET_ELEMENT_LENGTH (sizeof ("_get_element") - 1)
#define SET_ELEMENT_LENGTH (sizeof ("_set_element") - 1)

static int initialized, userdata = LUA_REFNIL, metatable = LUA_REFNIL;
static Node *list;
static const void *signature;

int t_is_node (lua_State *L, int index)
{
    if (!lua_type (L, index) == LUA_TUSERDATA || !lua_getmetatable (L, index)) {
	return 0;
    } else {
	if (lua_topointer (L, -1) == signature) {
	    lua_pop (L, 1);
	    return 1;
	} else {
	    lua_pop (L, 1);
	    return 0;
	}
    }
}

id t_check_node (lua_State *L, int index, Class class)
{
    id object;

    if (!t_is_node(L, index)) {
	const char *s = lua_pushfstring(L, "%s expected, got %s",
					[class name],
					luaL_typename(L, index));
	luaL_argerror(L, index, s);
    }

    object = *(id *)lua_touserdata (L, index);

    if (![object isKindOf: class]) {
	const char *s = lua_pushfstring(L, "%s expected, got %s",
					[class name], [object name]);
	luaL_argerror(L, index, s);
    }

    return object;
}

static int constructnode (lua_State *L)
{
    Class class;

    class = (Class)lua_touserdata(L, lua_upvalueindex (1));

    [[class alloc] init];

    /* ...and initialize it. */

    if(lua_istable(L, 1)) {
	lua_pushnil(L);

	while(lua_next(L, 1)) {
	    lua_pushvalue(L, -2);
	    lua_insert(L, -2);
	    lua_settable(L, 2);
	}
    }

    return 1;
}

void t_export_nodes (lua_State *L, Class *classes)
{
    int i;

    lua_newtable (L);

    for (i = 0 ; classes[i] ; i += 1) {
	const char *name;
	char *lower;

	name = [classes[i] name];

	/* Make a temporary copy of the name and down-case it. */
	    
	lower = strcpy(alloca(strlen(name) + 1), name);
	lower[0] = tolower(lower[0]);

	lua_pushlightuserdata (L, classes[i]);
	lua_pushcclosure (L, constructnode, 1);
	lua_setfield(L, -2, lower);
    }	
}

void t_push_userdata (lua_State *L, int n, ...)
{
    va_list ap;
    int i, h;

    va_start(ap, n);

    lua_rawgeti (L, LUA_REGISTRYINDEX, userdata);
    h = lua_gettop (L);

    for (i = 0 ; i < n ; i += 1) {
	lua_pushlightuserdata (L, va_arg(ap, void *));
	lua_gettable (L, h);
    }

    va_end(ap);

    lua_remove (L, h);
}

void t_push_hook (lua_State *L, int reference)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, reference);

    if (!lua_isnoneornil (L, -1) &&
	!lua_isfunction (L, -1) && !lua_isuserdata (L, -1)) {
	t_print_error ("Value supplied as hook is a %s, which is not callable.\n",
		       lua_typename (L, lua_type(L, -1)));
	abort();
    }
}

int t_call_hook (lua_State *L, int reference, int n, int m)
{
    int h;

    if (reference != LUA_REFNIL) {
	/* Insert the hook before any parameters. */

	t_push_hook (L, reference);
	if (n > 0) {
	    lua_insert (L, -(n + 1));
	}

	/* Call the hook. */

	h = lua_gettop (L);
	t_call(L, n, m);

	return lua_gettop (L) - h + n + 1;
    } else {
	lua_pop (L, n);

	return 0;
    }
}

static int next_attribute(lua_State *L)
{
    Node *object;
    const struct protocol *protocol;
    int i;

    if (!t_is_node (L, 1)) {
	lua_pushnil (L);

	return 1;
    }

    object = *(Node **)lua_touserdata(L, 1);
    protocol = object->protocol;

    /* If the key is a number get the array part of the properties. */

    if (lua_type(L, 2) == LUA_TNUMBER) {
	int k;

	k = lua_tointeger (L, 2) + 1;

	/* Find the array element getter. */

	for (i = 0;
	     i < protocol->size &&
		 protocol->properties[0][i].name != NULL;
	     i += 1);

	lua_pop (L, 1);

	/* Push the integer key and call the getter to push the
	 * value. */

	if (i < protocol->size && k <= object->length) {
	    IMP implementation;
	    SEL selector;
	    int h_0, h, n;

	    lua_pushinteger (L, k);

	    h_0 = lua_gettop(_L);

	    selector = protocol->properties[0][i].selector;
	    implementation = class_getMethodImplementation(protocol->class, selector);

	    n = (*(int (*)(id, SEL, ...))implementation)(object, selector, k);

	    h = lua_gettop(_L);

	    /* Clean up any garbage pushed by the getter onto the
	     * stack. */

	    for (i = 1 ; i <= h - (h_0 + n) ; i += 1) {
		lua_remove (_L, h_0 + i);
	    }

	    return 2;
	} else {
	    lua_pushnil (L);
	}
    }

    /* First time we get here is when we've just finished the array
     * part. The key will then be null.*/

    if (lua_toboolean (L, lua_upvalueindex (1))) {
	return 1;
    }

    if (lua_isnil (L, 2)) {
	i = 0;
    } else {
	const char *k;

	assert (lua_type (L, 2) == LUA_TSTRING);
	k = lua_tostring (L, 2);

	/* Find the getter for the next key. */

	for (i = 1;
	     i < protocol->size &&
		 k != protocol->properties[0][i - 1].name;
	     i += 1);
    }

    /* Skip the getter for the array elements and the catch-all
     * getter. */

    if (i < protocol->size &&
	(protocol->properties[0][i].name == NULL ||
	 *protocol->properties[0][i].name == '\0')) {
	i += 1;
    }

    lua_pop (L, 1);

    /* Push the key-value pair. */

    if (i < protocol->size) {
	IMP implementation;
	SEL selector;
	int h_0, h, n;

	lua_pushstring (L, protocol->properties[0][i].name);

	h_0 = lua_gettop(_L);

	selector = protocol->properties[0][i].selector;
	implementation = class_getMethodImplementation(protocol->class, selector);
	n = (*(int (*)(id, SEL, ...))implementation)(object, selector);

	h = lua_gettop(_L);

	/* Clean up any garbage pushed by the getter onto the
	 * stack. */

	for (i = 1 ; i <= h - (h_0 + n) ; i += 1) {
	    lua_remove (_L, h_0 + i);
	}

	return 2;
    } else {
	lua_pushnil (L);

	return 1;
    }
}

static int configuration_iterator(lua_State *L)
{
    t_check_node(L, 1, [Node class]);

    lua_pushcfunction (L, next_attribute);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, 0);

    return 3;
}

static int children_iterator(lua_State *L)
{
    Node *object;

    object = t_check_node(L, 1, [Node class]);

    lua_getglobal (L, "next");
    lua_rawgeti (L, LUA_REGISTRYINDEX, object->children);    
    lua_pushnil(L);

    return 3;
}

static int ancestor(lua_State *L)
{
    Node *object;
    int i, n;

    object = t_check_node(L, 1, [Node class]);
    n = luaL_optinteger(L, 2, 0);

    for (i = 0 ; i < n && object ; i += 1, object = object->up);

    if (object) {
	t_push_userdata (L, 1, object);
    } else {
	lua_pushnil (L);
    }

    return 1;
}

static int next_ancestor(lua_State *L)
{
    int k;

    k = lua_tointeger(L, 2);

    lua_pop (L, 1);
    lua_pushinteger (L, k + 1);
    lua_pushcfunction (L, ancestor);
    lua_pushvalue (L, 1);
    lua_pushinteger (L, k + 1);
    t_call (L, 2, 1);

    return lua_isnil (L, -1) ? 1 : 2;
}

static int next_key(lua_State *L)
{
    static int state;

    if (lua_isnil (L, 2)) {
	state = 0;
    }

    /* First try to complete from the user assigned values. */

    if (state == 0) {
	lua_getuservalue (L, 1);
	lua_insert (L, -2);
	if (!lua_next (L, -2)) {
	    /* Pop the uservalue table, and prime the next stage. */

	    lua_pop (L, 1);
	    lua_pushnil (L);
	    state = 1;
	}
    }

    /* Next go through the children. */

    if (state == 1) {
	Node *object;

	object = *(Node **)lua_touserdata(L, 1);
	lua_rawgeti (L, LUA_REGISTRYINDEX, object->children);
	lua_insert (L, -2);

	if (!lua_next (L, -2)) {
	    lua_pop (L, 1);
	    lua_pushinteger(L, 0);

	    state = 2;
	}
    }

    /* And finish with the attributes. */

    if (state == 2) {
	lua_pushcfunction (L, next_attribute);
	lua_insert (L, 1);
	t_call (L, 2, 2);

	if (lua_isnil (L, -2)) {
	    lua_pushnil (L);
	    return 1;
	}
    }

    return 2;
}

static int next_array_key(lua_State *L)
{
    Node *object;
    static int state;
    int k;

    k = lua_tointeger (L, 2);

    if (k == 0) {
	state = 0;
    }

    /* First try to complete from the user assigned values. */

    if (state == 0) {
	lua_getuservalue (L, 1);
	lua_pushinteger (L, k + 1);
	lua_rawgeti(L, -2, k + 1);

	if (lua_isnil (L, -1)) {
	    /* Pop the lot and prime the next stage. */

	    lua_settop (L, 1);
	    lua_pushinteger (L, 0);

	    state = 1;
	}
    }

    /* Next go through the children. */

    if (state == 1) {
	object = *(Node **)lua_touserdata(L, 1);
	lua_rawgeti (L, LUA_REGISTRYINDEX, object->children);
	lua_pushinteger (L, k + 1);
	lua_rawgeti(L, -2, k + 1);

	if (lua_isnil (L, -1)) {
	    /* Prime the next stage. */

	    lua_settop (L, 1);
	    lua_pushinteger (L, 0);

	    state = 2;
	}
    }

    /* And finish with the attributes. */

    if (state == 2) {
	lua_pushboolean (L, 1);
	lua_pushcclosure (L, next_attribute, 1);
	lua_insert (L, 1);
	t_call (L, 2, 2);

	if (lua_isnil (L, -2)) {
	    lua_pushnil (L);
	    return 1;
	}
    }

    return 2;
}

static int ancestors_iterator(lua_State *L)
{
    lua_pushcfunction (L, next_ancestor);
    lua_pushvalue(L, 1);
    lua_pushinteger (L, 0);

    return 3;
}

static int ancestors_tostring(lua_State *L)
{
    lua_pushstring(L, "Ancestors");
   
    return 1;
}

static int ancestors_len(lua_State *L)
{
    Node *object;
    int i;
    
    for (i = 0, object = *(Node **)lua_touserdata(L, lua_upvalueindex(1));
	 object;
	 i += 1, object = object->up);

    lua_pushinteger (L, i - 1);

    return 1;
}

static int ancestors_index(lua_State *L)
{
    Node *object;
    int i, n;

    if (lua_type (_L, 2) == LUA_TNUMBER) {
	n = lua_tonumber (_L, 2);
	object = *(Node **)lua_touserdata (_L, lua_upvalueindex(1));
	
	for (i = 0 ; i < n && object ; i += 1, object = object->up);

	if (object) {
	    t_push_userdata (_L, 1, object);
	} else {
	    lua_pushnil (_L);
	}
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

static int ancestors_newindex(lua_State *L)
{
    Node *object;
    int i, n;

    if (lua_type (_L, 2) == LUA_TNUMBER) {
    	n = lua_tonumber (_L, 2);
	object = *(Node **)lua_touserdata (_L, lua_upvalueindex(1));
	
	for (i = 0 ; i < n - 1 && object ; i += 1, object = object->up);

	if (object) {
	    t_push_userdata (_L, 1, object);
	    lua_pushstring (_L, "parent");
	    lua_pushvalue (_L, 3);
	    lua_settable (_L, -3);
	}
    }

    return 0;
}

static int __ipairs(lua_State *L)
{
    lua_pushcfunction (L, next_array_key);
    lua_pushvalue(L, 1);
    lua_pushinteger (L, 0);

    return 3;
}

static int __pairs(lua_State *L)
{
    lua_pushcfunction (L, next_key);
    lua_pushvalue(L, 1);
    lua_pushnil (L);

    return 3;
}

static int __tostring(lua_State *L)
{
    Node *object;

    object = *(Node **)lua_touserdata(L, 1);

    if (object->tag.reference != LUA_REFNIL) {
	lua_pushstring(L, object->tag.string);
    } else {
	lua_pushfstring(L, "%s: %p", [[object class] name], object);
    }

    return 1;
}

static int __len(lua_State *L)
{
    Node *object;
    const struct protocol *protocol;
    int i;

    object = *(Node **)lua_touserdata(L, 1);
    protocol = object->protocol;

    /* Look for an array element getter.  If one exists return the
     * custom length of the object.  Otherwise return the raw length
     * of the environment. */

    for (i = 0 ; i < protocol->size ; i += 1) {
	if (protocol->properties[0][i].name == NULL) {
	    lua_pushnumber (L, object->length);
	    return 1;
	}
    }

    lua_getuservalue (L, 1);
    lua_pushnumber (L, lua_rawlen (L, -1));

    return 1;
}

static int __gc(lua_State *L)
{
    Node *object;

    object = *(Node **)lua_touserdata(L, 1);
    [object free];

    return 0;
}

static int __index(lua_State *L)
{
    Node *object;
    const struct protocol *protocol;
    int i, j;

    object = *(Node **)lua_touserdata(L, 1);

    if (object->get != LUA_REFNIL && !object->rawaccess) {
	object->rawaccess = 1;
	i = t_call_hook (L, object->get, 2, LUA_MULTRET);
	object->rawaccess = 0;
	 
	return i;
    }

    protocol = object->protocol;

    if (lua_type (L, 2) == LUA_TNUMBER) {
	lua_Number n;

	n = lua_tonumber (L, 2);

	if ((lua_Integer)n == n) {
	    /* Try to find a getter for array elements. */

	    for (i = 0 ; i < protocol->size ; i += 1) {
		if (protocol->properties[0][i].name == NULL) {
		    IMP implementation;
		    SEL selector;

		    selector = protocol->properties[0][i].selector;
		    implementation = class_getMethodImplementation(protocol->class, selector);

		    return (*(int (*)(id, SEL, ...))implementation)(object, selector, (int)n);
		}
	    }
	}
    } else if (lua_type (L, 2) == LUA_TSTRING) {
	const char *k;

	k = lua_tostring (L, 2);

	/* Look through the node's protocol and see if we have a
	 * matching method for getting. */

	for (i = 0 ; i < protocol->size ; i += 1) {
	    const char *name;

	    name = protocol->properties[0][i].name;
	    if (name && !strcmp(name, k)) {
		IMP implementation;
		SEL selector;

		selector = protocol->properties[0][i].selector;
		implementation = class_getMethodImplementation(protocol->class, selector);

		/* This ugly thing casts the implementation to the
		 * correct function pointer type, dereferences it, and
		 * calls it with the correct arguments. */

		return (*(int (*)(id, SEL, ...))implementation)(object, selector);
	    }
	}
    }

    /* Try to find a catch-all getter. */

    for (i = 0 ; i < protocol->size ; i += 1) {
	if (protocol->properties[0][i].name &&
	    *protocol->properties[0][i].name == '\0') {
	    IMP implementation;
	    SEL selector;

	    selector = protocol->properties[0][i].selector;
	    implementation = class_getMethodImplementation(protocol->class, selector);

	    j = (*(int (*)(id, SEL, ...))implementation)(object, selector);

	    if (j > 0) {
		return j;
	    }

	    break;
	}
    }

    /* If the key is not part of the protocol look it up in
     * the environment. */

    lua_getuservalue (L, 1);
    lua_replace (L, 1);
    lua_rawget (L, 1);

    return 1;
}

static int __newindex(lua_State *L)
{
    Node *object;
    const struct protocol *protocol;
    int i, j;

    /* This is symmetric to __index.  See comments above. */

    object = *(Node **)lua_touserdata(L, 1);

    if (object->set != LUA_REFNIL && !object->rawaccess) {
	object->rawaccess = 1;
	t_call_hook (L, object->set, 3, 0);
	object->rawaccess = 0;
	 
	return 1;
    }

    protocol = object->protocol;

    if (lua_type (L, 2) == LUA_TNUMBER) {
	lua_Number n;

	n = lua_tonumber (L, 2);

	if ((lua_Integer)n == n) {
	    /* Try to find a setter for array elements. */

	    for (i = 0 ; i < protocol->size ; i += 1) {
		if (protocol->properties[1][i].name == NULL) {
		    IMP implementation;
		    SEL selector;

		    selector = protocol->properties[1][i].selector;
		    implementation = class_getMethodImplementation(protocol->class, selector);

		    (*(void (*)(id, SEL, ...))implementation)(object, selector, (int)n);

		    return 0;
		}
	    }
	}
    } else if (lua_type (L, 2) == LUA_TSTRING) {
	const char *k;

	k = lua_tostring (L, 2);

	/* Try to find a setter for the key and call it. */

	for (i = 0 ; i < protocol->size ; i += 1) {
	    const char *name;

	    name = protocol->properties[1][i].name;
	    if (name && !strcmp(name, k)) {
		IMP implementation;
		SEL selector;

		selector = protocol->properties[1][i].selector;
		implementation = class_getMethodImplementation(protocol->class, selector);

		(*(void (*)(id, SEL, ...))implementation)(object, selector);

		return 0;
	    }
	}
    }

    /* Try to find a catch-all setter. */

    for (i = 0 ; i < protocol->size ; i += 1) {
	if (protocol->properties[1][i].name &&
	    *protocol->properties[1][i].name == '\0') {
	    IMP implementation;
	    SEL selector;

	    selector = protocol->properties[1][i].selector;
	    implementation = class_getMethodImplementation(protocol->class, selector);

	    j = (*(int (*)(id, SEL, ...))implementation)(object, selector);

	    /* Return only if the key was handled. */

	    if (j > 0) {
		return 0;
	    }

	    break;
	}
    }

    /* If the key is not part of the protocol store the value in the
     * environment. */

    lua_getuservalue (L, 1);
    lua_replace (L, 1);
    lua_rawset (L, 1);

    return 0;
}

@implementation Node

+(const struct protocol *) introspect
{
    struct protocol *protocol;
    Method *methods;
    int i, sizes[2];

    /* Look for a cached copy of the protocol in the registry. */

    lua_pushlightuserdata (_L, self);
    lua_rawget (_L, LUA_REGISTRYINDEX);

    if (lua_isnil (_L, -1)) {
	/* This is the first allocation for this class.  Determine the
	 * protocol and cache it for future allocations. */

	protocol = (struct protocol *) calloc (1, sizeof(struct protocol));

	/* If this is a derived class, fetch the superclass' protocol
	 * and start with that.  Otherwise start from scratch. */

	if (self != [Node class]) {
	    const struct protocol *superprotocol;

	    superprotocol = [[self superclass] introspect];

	    memcpy (protocol, superprotocol,
		    sizeof(struct protocol));

	    if (protocol->size > 0) {
		for (i = 0 ; i < 2 ; i += 1) {
		    size_t length;

		    length = protocol->size *
			sizeof(protocol->properties[i][0]);

		    protocol->properties[i] = malloc (length);

		    memcpy (protocol->properties[i],
			    superprotocol->properties[i],
			    length);
		}
	    }

	    sizes[0] = protocol->size;
	    sizes[1] = protocol->size;
	} else {
	    sizes[0] = 0;
	    sizes[1] = 0;
	}

	methods = class_copyMethodList ([self class], NULL);

	for (i = 0 ; methods[i] ; i += 1) {
	    const char *name;
	    SEL selector;
	    int j;

	    selector = method_getName(methods[i]);
	    name = sel_getName(selector);

	    /* Choose between setter and getter vectors and strip the
	     * prefix from the name (minus one accounts for the
	     * terminating null character). */

	    if (!strncmp(name, GET_ELEMENT, GET_ELEMENT_LENGTH)) {
		name = NULL;
		j = 0;
	    } else if (!strncmp(name, SET_ELEMENT, SET_ELEMENT_LENGTH)) {
		name = NULL;
		j = 1;
	    } else if (!strncmp(name, GET_PREFIX, GET_PREFIX_LENGTH)) {
		name += GET_PREFIX_LENGTH;
		j = 0;
	    } else if (!strncmp(name, SET_PREFIX, SET_PREFIX_LENGTH)) {
		name += SET_PREFIX_LENGTH;
		j = 1;
	    } else {
		j = -1;
	    }

	    if (j >= 0) {
		int k;

		/* If name is NULL then no reference is made here.
		 * This means that the method is intended for array
		 * elements. */

		lua_pushstring (_L, name);
		name = lua_tostring (_L, -1);
		luaL_ref (_L, LUA_REGISTRYINDEX);

		/* Look through the protocol in case the method exists
		 * in the superclass. */

		for (k = 0;
		     k < sizes[j] &&
			 protocol->properties[j][k].name != name ;
		     k += 1);

		/* If not allocate a new spot. */

		if (k == sizes[j]) {
		    sizes[j] += 1;
		    protocol->properties[j] = realloc (protocol->properties[j],
						       sizes[j] * sizeof(protocol->properties[j][0]));
		}

		protocol->properties[j][k].name = name;
		protocol->properties[j][k].selector = selector;
	    }
	}

	free (methods);

	if (sizes[0] != sizes[1]) {
	    fprintf (stderr, "%s: ", [self name]);
	    assert(sizes[0] == sizes[1]);
	}
	
	protocol->size = sizes[0];
	protocol->class = self;

	/* Cache the protocol structure in the registry. */

	lua_pushlightuserdata (_L, [self class]);
	lua_pushlightuserdata (_L, protocol);
	lua_rawset (_L, LUA_REGISTRYINDEX);

	t_print_message ("Established the protocol of class '%s'.\n",
			 [[self class] name]);
    } else {
	protocol = lua_touserdata(_L, -1);
    }

    lua_pop (_L, 1);

    return protocol;
}

-(id) init
{
    self = [super init];

    self->protocol = [[self class] introspect];

    if (!initialized) {
	/* Create the userdata reference table. */

	lua_newtable (_L);

	lua_newtable (_L);
	lua_pushstring (_L, "__mode");
	lua_pushstring (_L, "v");
	lua_settable (_L, -3);
	lua_setmetatable (_L, -2);

	userdata = luaL_ref (_L, LUA_REGISTRYINDEX);

	lua_pushcfunction (_L, configuration_iterator);
	lua_setglobal (_L, "configuration");

	lua_pushcfunction (_L, children_iterator);
	lua_setglobal (_L, "children");

	lua_pushcfunction (_L, ancestors_iterator);
	lua_setglobal (_L, "ancestors");

	lua_pushcfunction (_L, ancestor);
	lua_setglobal (_L, "ancestor");

	initialized = 1;
    }

    /* Put newly created nodes in the orphans list. */

    if (list) {
	self->right = list->right;
	self->left = list;
	list->right->left = self;
	list->right = self;
    } else {
	self->left = self;
	self->right = self;

	list = self;
    }

    self->tag.reference = LUA_REFNIL;
    self->tag.string = NULL;

    self->up = NULL;
    self->down = NULL;

    self->rawaccess = 0;
    self->linked = 0;
    self->key.reference = LUA_REFNIL;
    self->key.number = NAN;
    self->key.string = NULL;;

    /* Initialize the hooks. */

    self->link = LUA_REFNIL;
    self->unlink = LUA_REFNIL;
    self->step = LUA_REFNIL;
    self->traverse = LUA_REFNIL;
    self->prepare = LUA_REFNIL;
    self->finish = LUA_REFNIL;
    self->begin = LUA_REFNIL;
    self->get = LUA_REFNIL;
    self->set = LUA_REFNIL;

    /* Create the children table. */

    lua_newtable (_L);
    self->children = luaL_ref (_L, LUA_REGISTRYINDEX);

    /* Create the object userdata. */

    *(id *)lua_newuserdata(_L, sizeof(id)) = self;

    if (metatable == LUA_REFNIL) {
	lua_newtable (_L);
	lua_pushstring(_L, "__len");
	lua_pushcfunction(_L, (lua_CFunction)__len);
	lua_settable(_L, -3);
	lua_pushstring(_L, "__index");
	lua_pushcfunction(_L, (lua_CFunction)__index);
	lua_settable(_L, -3);
	lua_pushstring(_L, "__newindex");
	lua_pushcfunction(_L, (lua_CFunction)__newindex);
	lua_settable(_L, -3);
	lua_pushstring(_L, "__tostring");
	lua_pushcfunction(_L, (lua_CFunction)__tostring);
	lua_settable(_L, -3);
	lua_pushstring(_L, "__pairs");
	lua_pushcfunction(_L, (lua_CFunction)__pairs);
	lua_settable(_L, -3);
	lua_pushstring(_L, "__ipairs");
	lua_pushcfunction(_L, (lua_CFunction)__ipairs);
	lua_settable(_L, -3);
	lua_pushstring(_L, "__gc");
	lua_pushcfunction(_L, (lua_CFunction)__gc);
	lua_settable(_L, -3);

	signature = lua_topointer (_L, -1);
	metatable = luaL_ref (_L, LUA_REGISTRYINDEX);
    }

    lua_rawgeti (_L, LUA_REGISTRYINDEX, metatable);
    lua_setmetatable(_L, -2);

    /* Add an environment table. */

    lua_newtable (_L);
    lua_setuservalue(_L, -2);

    /* Add a weak reference. */

    lua_rawgeti (_L, LUA_REGISTRYINDEX, userdata);
    lua_pushlightuserdata (_L, self);
    lua_pushvalue (_L, -3);
    lua_settable (_L, -3);
    lua_pop(_L, 1);

    return self;
}

-(id) free
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->children);

    luaL_unref (_L, LUA_REGISTRYINDEX, self->unlink);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->link);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->step);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->traverse);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->prepare);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->finish);
    luaL_unref (_L, LUA_REGISTRYINDEX, self->begin);

    return self;
}

-(void) meetParent: (Node *)parent
{
}

-(void) missParent: (Node *)parent
{
}

-(void) meetSibling: (Node *)sibling
{
}

-(void) missSibling: (Node *)sibling
{
}

-(void) adopt: (Node *)child
{
    Node *l, *r, *sibling;

    /* Before the child can get adopted by a parent we need to remove
     * it from the orphans list. */

    assert (child->left);
    assert (child->right);

    child->left->right = child->right;
    child->right->left = child->left;
    
    child->left = NULL;
    child->right = NULL;
    child->up = self;

    /* t_trace ("%s(%p) => %s(%p)\n", [self name], self, [child name], child); */
    if (self->down) {
	/* Find where to insert the new child.  Children are ordered
	 * according to key with numbers first and then strings in
	 * ascending order.  Nodes with other types of keys are
	 * unordered and come last in the list. */
	
	for (l = NULL, r = self->down ; r ; l = r, r = r->right) {
	    if (!isnan(child->key.number)) {
		if (!isnan(r->key.number) &&
		    r->key.number > child->key.number) {
		    break;
		} else {
		    continue;
		}
	    } else if (child->key.string != NULL) {
		if (!isnan(r->key.number)) {
		    continue;
		} else if (r->key.string != NULL &&
		    strcmp(r->key.string, child->key.string) > 0) {
		    break;
		} else {
		    continue;
		}
	    } else {
		if (!isnan(r->key.number) ||
		    r->key.string != NULL) {
		    continue;
		} else {
		    break;
		}
	    }
	}
	assert (r || l);

	child->left = l;
	child->right = r;

	if (l) {
	    l->right = child;
	}

	if (r) {
	    r->left = child;
	}

	if (!l) {
	    assert (self->down == r);
	    self->down = child;
	}
    } else {
	self->down = child;
    }

    /* Introduce the newcomer to the family. */
    
    for (sibling = self->down ; sibling ; sibling = sibling->right) {
	if (sibling != child) {
	    [child meetParent: self];
	    [child meetSibling: sibling];
	    [sibling meetSibling: child];
	}
    }   
}

-(void) renounce: (Node *)child
{
    Node *sibling;
    
    /* Say bye-bye to your family. */
    
    for (sibling = self->down ; sibling ; sibling = sibling->right) {
	if (sibling != child) {
	    [child missParent: self];
	    [child missSibling: sibling];
	    [sibling missSibling: child];
	}
    }

    /* Remove the child. */
    
    if (child->right) {
	child->right->left = child->left;
    }
	    
    if (child->left){
	child->left->right = child->right;
    } else {
	self->down = child->right;
    }

    child->up = NULL;

    /* Link it into the orphans list.  Since the child is removed from
     * its parent it follows that at least the parent's root is an
     * orphan so the list is already initialized. */

    assert (list);
    child->right = list->right;
    child->left = list;
    list->right->left = child;
    list->right = child;
}

-(void) toggle
{
    Node *child, *sister;

    /* If the node is being linked call
       the hook first, before descending
       into the ancestors. */
    
    if (!self->linked) {
	t_push_userdata (_L, 1, self);
    	t_call_hook (_L, self->link, 1, 0);
    }

    /* Toggle ourselves. */
    
    self->linked = !self->linked;

    /* Recurse on the node's children. */
    
    for(child = self->down ; child ; child = sister) {
	sister = child->right;
        [child toggle];
    }

    /* If the node is being unlinked call the hook
       on the way out, after all the ancestors have
       been unlinked. */
    
    if (!self->linked) {
	t_push_userdata (_L, 1, self);
    	t_call_hook (_L, self->unlink, 1, 0);
    }
}

-(void) begin
{
    Node *child, *next;

    t_push_userdata (_L, 1, self);
    t_call_hook (_L, self->begin, 1, 0);
    
    for(child = self->down ; child ; child = next) {
	next = child->right;
	[child begin];
    }
}

-(void) stepBy: (double) h at: (double) t
{
    Node *child, *next;

    t_push_userdata (_L, 1, self);
    lua_pushnumber (_L, h);
    lua_pushnumber (_L, t);
    t_call_hook (_L, self->step, 3, 0);
    
    for(child = self->down ; child ; child = next) {
	next = child->right;
	[child stepBy: h at: t];
    }
}

-(void) prepare
{
    Node *child, *next;

    t_push_userdata (_L, 1, self);
    t_call_hook (_L, self->prepare, 1, 0);
    
    for(child = self->down ; child ; child = next) {
	next = child->right;
	[child prepare];
    }
}

-(void) traverse
{
    Node *child, *next;

    t_push_userdata (_L, 1, self);
    t_call_hook (_L, self->traverse, 1, 0);
    
    for(child = self->down ; child ; child = next) {
	next = child->right;
	[child traverse];
    }
}

-(void) finish
{
    Node *child, *next;

    t_push_userdata (_L, 1, self);
    t_call_hook (_L, self->finish, 1, 0);
    
    for(child = self->down ; child ; child = next) {
	next = child->right;
	[child finish];
    }
}

-(int) _get_tag
{
    lua_pushstring (_L, self->tag.string);
    
    return 1;
}
 
-(void) _set_tag
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->tag.reference);
    
    if (lua_isstring (_L, 3)) {
	self->tag.string = lua_tostring (_L, 3);
	self->tag.reference = luaL_ref (_L, LUA_REGISTRYINDEX);
    } else {
	self->tag.string = NULL;
	self->tag.reference = LUA_REFNIL;
    }
}

-(int) _get_linked
{
    lua_pushboolean (_L, self->linked);
    
    return 1;
}
 
-(void) _set_linked
{
    /* Do nothing. */
}

-(int) _get_link
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->link);
    
    return 1;
}

-(int) _get_unlink
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->unlink);
    
    return 1;
}

-(int) _get_step
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->step);

    return 1;
}

-(int) _get_traverse
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->traverse);

    return 1;
}

-(int) _get_prepare
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->prepare);

    return 1;
}

-(int) _get_finish
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->finish);

    return 1;
}

-(int) _get_begin
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->begin);

    return 1;
}

-(int) _get_get
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->get);

    return 1;
}

-(int) _get_set
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->set);

    return 1;
}

-(void) _set_link
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->link);
    self->link = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_unlink
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->unlink);
    self->unlink = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_step
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->step);
    self->step = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_traverse
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->traverse);
    self->traverse = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_prepare
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->prepare);
    self->prepare = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_finish
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->finish);
    self->finish = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_begin
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->begin);
    self->begin = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_get
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->get);
    self->get = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_set
{
    luaL_unref (_L, LUA_REGISTRYINDEX, self->set);
    self->set = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(int) _get_parent
{
    if (self->up) {
	lua_rawgeti (_L, LUA_REGISTRYINDEX, userdata);
	lua_pushlightuserdata (_L, self->up);
	lua_rawget (_L, -2);
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_ancestors
{
    lua_newtable (_L);

    lua_newtable (_L);
    lua_pushstring(_L, "__len");
    lua_pushvalue (_L, 1);
    lua_pushcclosure(_L, (lua_CFunction)ancestors_len, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__index");
    lua_pushvalue (_L, 1);
    lua_pushcclosure(_L, (lua_CFunction)ancestors_index, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__newindex");
    lua_pushvalue (_L, 1);
    lua_pushcclosure(_L, (lua_CFunction)ancestors_newindex, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__tostring");
    lua_pushvalue (_L, 1);
    lua_pushcclosure(_L, (lua_CFunction)ancestors_tostring, 1);
    lua_settable(_L, -3);
    lua_setmetatable(_L, -2);

    return 1;
}

-(void) _set_parent
{
    if (!lua_isnil (_L, 3)) {
	t_check_node (_L, 3, [Node class]);
	lua_pushlightuserdata (_L, self);
	lua_pushvalue (_L, 1);
	lua_settable (_L, 3);
    } else if (self->up) {
	t_push_userdata (_L, 1, self->up);
	lua_rawgeti (_L, LUA_REGISTRYINDEX, self->key.reference);
	lua_pushnil (_L);
	lua_settable (_L, -3);
    }
}

-(void) _set_ancestors
{
    /* Not supported for now. */
}

-(int) _get_
{
    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->children);
    lua_pushvalue (_L, 2);
    lua_rawget (_L, -2);

    if (lua_isnil (_L, -1)) {
	lua_pop (_L, 2);

	return 0;
    } else {
	return 1;
    }
}

-(int) _set_
{
    Node *child;
    
    /* Check if there's a node linked with the same key and if so free
     * it. */

    lua_rawgeti (_L, LUA_REGISTRYINDEX, self->children);
    lua_pushvalue (_L, 2);
    lua_rawget (_L, -2);
 
    if(t_is_node (_L, -1)) {
	child = *(Node **)lua_touserdata (_L, -1);
	lua_pop (_L, 1);
            
	if(child->linked) {
	    [child toggle];
	}
		
	[self renounce: child];

	luaL_unref (_L, LUA_REGISTRYINDEX, child->key.reference);
	child->key.reference = LUA_REFNIL;
	child->key.number = NAN;
	child->key.string = NULL;
	
	/* Remove the key->child entry. */
    
	lua_pushvalue (_L, 2);
	lua_pushnil (_L);
	lua_rawset (_L, -3);
    } else {
	/* Pop the nil value but leave the children table on the stack
	 * as we'regoing to need it when adding the new child
	 * below. */
	
	lua_pop (_L, 1);
    }
  
    /* Link the new node. */
	
    if(t_is_node (_L, 3)) {
	child = *(Node **)lua_touserdata (_L, 3);

	/* Unlink the child if it already has a parent... */

	if (child->up) {
	    lua_rawgeti (_L, LUA_REGISTRYINDEX, child->up->children);

	    /* Remove the key->child entry from the old parent. */
	    
	    lua_rawgeti (_L, LUA_REGISTRYINDEX, child->key.reference);
	    lua_pushnil (_L);
	    lua_rawset (_L, -3);
	    
	    lua_pop (_L, 1);

	    /* Remove from the old parent and toggle if necessary. */
	    
	    [child->up renounce: child];
	    
	    luaL_unref (_L, LUA_REGISTRYINDEX, child->key.reference);
	    child->key.reference = LUA_REFNIL;
	    child->key.number = NAN;
	    child->key.string = NULL;

	    if (child->linked) {
		[child toggle];
	    }
	}

	assert (child->key.reference == LUA_REFNIL);
	assert (isnan(child->key.number));
	assert (child->key.string == NULL);
	
	lua_pushvalue (_L, 2);
	child->key.reference = luaL_ref (_L, LUA_REGISTRYINDEX);

	if (lua_type (_L, 2) == LUA_TNUMBER) {
	    child->key.number = lua_tonumber (_L, 2);
	}

	if (lua_type (_L, 2) == LUA_TSTRING) {
	    child->key.string = lua_tostring (_L, 2);
	}

	lua_replace (_L, 1);
	lua_rawset (_L, 1);

	/* Add to the new parent and toggle if necessary. */
	    
	[self adopt: child];
	
	if (self->linked) {
	    [child toggle];
	}
    
	return 1;
    } else {
	/* Pop the children table since we're not goint to be adding
	 * any children. */
	
	lua_pop (_L, 1);
	
	return 0;
    }
}

@end
