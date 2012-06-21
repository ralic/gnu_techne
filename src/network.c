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
#include <lua.h>
#include <lauxlib.h>
#include <microhttpd.h>

#include "../config.h"
#include "techne.h"
#include "network.h"

struct context {
    char *data;
    size_t size;
};    

static int connections, pages, mime, port, block;
static struct MHD_Daemon *http;

static int iterate_post  (void *cls,
			  enum MHD_ValueKind kind,
			  const char *key,
			  const char *filename,
			  const char *content_type,
			  const char *transfer_encoding,
			  const char *data,
			  uint64_t off,
			  size_t size)
{
    lua_pushstring (_L, key);
    lua_pushstring (_L, key);
    lua_gettable (_L, -3);
    
    if (off == 0) {
	if (!lua_isnil (_L, -1)) {
	    /* If a value is already there, check whether this is
	       a group table and if not create one and put it in. */
	    
	    if (lua_istable (_L, -1) && lua_getmetatable (_L, -1)) {
		lua_pop (_L, 1);
	    } else {
		lua_newtable (_L);
		lua_newtable (_L);
		lua_setmetatable (_L, -2);
		
		lua_insert (_L, -2);
		lua_rawseti (_L, -2, 1);
	    }

	    lua_pushnil (_L);
	}
    } else {
	if (lua_istable (_L, -1) && lua_getmetatable (_L, -1)) {
	    int n;

	    lua_pop (_L, 1);
	    n = lua_rawlen (_L, -1);
	    
	    /* We need to continue a grouped value, so
	       remove it from the group table. */

	    lua_rawgeti (_L, -1, n);
	    
	    lua_pushnil (_L);
	    lua_rawseti (_L, -3, n);
	}
    }
    
    /* If it's a file upload the value is a table
       {filename, mime type, data}, otherwise it's
       just the data. */
    
    if (filename) {
	/* If no vaule's there already create one
	   from scratch, otherwise fill it in. */
	
	if (!lua_istable (_L, -1)) {
	    lua_pop (_L, 1);
	    lua_newtable (_L);

	    lua_pushstring (_L, filename);
	    lua_rawseti (_L, -2, 1);

	    lua_pushstring (_L, content_type);
	    lua_rawseti (_L, -2, 2);

	    lua_pushlstring (_L, data, size);
	    lua_rawseti (_L, -2, 3);

	    printf ("  %s = %s/%s\n",
		    key, filename, content_type);
	} else {
	    lua_rawgeti (_L, -1, 3);
	    lua_pushlstring (_L, data, size);
	    lua_concat (_L, 2);
	    lua_rawseti (_L, -2, 3);
	}

	/* Insert the value into the group table if needed. */
	
	if (lua_istable (_L, -2)) {
	    lua_rawseti (_L, -2, lua_rawlen (_L, -2) + 1);
	}
    } else {
	/* Process the new value. */
	
	if (lua_isstring (_L, -1)) {
	    lua_pushlstring (_L, data, size);
	    lua_concat (_L, 2);
	} else if (lua_isnil (_L, -1)) {
	    lua_pop (_L, 1);
	    lua_pushlstring (_L, data, size);
	}

	if (off == 0) {
	    if (lua_rawlen (_L, -1) > 40) {
		printf ("  %s = %.40s...\n", key, lua_tostring (_L, -1));
	    } else {
		printf ("  %s = %s\n", key, lua_tostring (_L, -1));
	    }
	}

	if (lua_istable (_L, -2)) {
	    lua_rawseti (_L, -2, lua_rawlen (_L, -2) + 1);
	}
    }
			
    lua_settable (_L, -3);    
	
    /* printf ("%s, %s, %s, %s, %lu, %zd, %p\n", key, filename, content_type, transfer_encoding, off, size, data); */

    return MHD_YES;
}

static int iterate_get (void *cls, enum MHD_ValueKind kind,
			const char *key, const char *value)
{
    printf ("  %s = %s\n", key, value);
    
    lua_pushstring (_L, key);
    lua_pushstring (_L, key);
    lua_gettable (_L, -3);

    if (lua_isnil (_L, -1)) {
	lua_pop(_L, 1);
	lua_pushstring (_L, value);
    } else if (lua_istable (_L, -1)) {
	lua_pushstring (_L, value);
	lua_rawseti (_L, -2, lua_rawlen (_L, -2) + 1);
    } else {
	assert (lua_isstring (_L, -1));
	
	lua_newtable (_L);
	lua_insert (_L, -2);
	lua_rawseti (_L, -2, 1);
	lua_pushstring (_L, value);
	lua_rawseti (_L, -2, 2);
    }
			
    lua_settable (_L, -3);

    return MHD_YES;
}

static int respond (void *cls,
		    struct MHD_Connection *connection,
		    const char *url, const char *method, const char *version,
		    const char *upload_data, size_t *upload_data_size,
		    void **con_cls)
{
    if (*con_cls == NULL) {
	printf ("Received a %s request for URL %s.\n", method, url);
    
    	*con_cls = calloc (1, sizeof (struct context));
	connections += 1;

    	return MHD_YES;
    }

    /* Aggregate the upload data into a single buffer. */
    
    if (*upload_data_size > 0) {
	struct context *c = (struct context *)*con_cls;
	
    	c->data = realloc (c->data, c->size + *upload_data_size);
    	memcpy (c->data + c->size, upload_data, *upload_data_size);
	
    	c->size += *upload_data_size;
    	*upload_data_size = 0;

    	return MHD_YES;
    }
    
    if (!strcmp (method, "HEAD") || !strcmp (method, "GET") || !strcmp (method, "POST")) {
	struct context *c = (struct context *)*con_cls;
	int h_0;

	h_0 = lua_gettop (_L);

	/* Parse the query and post data if any and
	   gather all fields inside a table. */
	
	lua_newtable (_L);

	/* Parse the query pairs. */
	    
	MHD_get_connection_values (connection,
				   MHD_GET_ARGUMENT_KIND,
				   iterate_get,
				   NULL);
	    
	/* And potentially pre-parsed post data pairs. */
	    
	MHD_get_connection_values (connection,
				   MHD_POSTDATA_KIND,
				   iterate_get,
				   NULL);

	/* And finally the post data buffer. */
	    
	if (c->size > 0) {
	    struct MHD_PostProcessor *post;
	    post = MHD_create_post_processor(connection,
					     512,
					     iterate_post, NULL);

	    /* If the data is parseable put it into the
	       query table otherwise feed the whole chunk
	       as-is via the post parameter. */
		
	    if (post &&
		MHD_post_process(post, c->data, c->size) == MHD_NO) {
		lua_pushlstring (_L, c->data, c->size);
	    }

	    if (post) {
		MHD_destroy_post_processor (post);
	    }
	}

	/* Push nil if no POST data was there. */
	    
	if (lua_type (_L, -1) != LUA_TSTRING) {
	    lua_pushnil (_L);
	}

	/* Retrieve the MIME type and content for the page. */
			    
	lua_rawgeti (_L, LUA_REGISTRYINDEX, mime);

	if (lua_istable (_L, -1)) {
	    lua_pushstring (_L, url);
	    lua_gettable (_L, -2);
	    lua_replace (_L, -2);
	}
			    
	lua_rawgeti (_L, LUA_REGISTRYINDEX, pages);

	if (lua_istable (_L, -1)) {
	    lua_pushstring (_L, url);
	    lua_gettable (_L, -2);
	    lua_replace (_L, -2);

	    if (lua_isfunction (_L, -1)) {
		/* Call the supplied function passing method, url,
		 * query parameters, unparseable post data and HTTP
		 * version in this order. */
		
		lua_pushstring (_L, method);
		lua_pushstring (_L, url);
		lua_pushvalue (_L, -6);
		lua_pushvalue (_L, -6);
		lua_pushstring (_L, version);
    
		lua_pcall(_L, 5, 1, 0);

		if (lua_type (_L, -1) != LUA_TSTRING) {
		    lua_settop (_L, h_0);
		    return MHD_YES;
		}
	    }
	}

	/* Return the content of the requested page. */
			    
	if (lua_isstring (_L, -1)) {
	    struct MHD_Response *response;
	    int result;
	    
	    /* Create and queue the response. */

	    response = MHD_create_response_from_data (lua_rawlen (_L, -1),
						      (void *)lua_tostring (_L, -1),
						      MHD_NO, MHD_YES);

	    MHD_add_response_header (response, "Server",
				     "Techne/" PACKAGE_VERSION);
	    MHD_add_response_header (response, "Cache-Control", "no-store");
	    /* MHD_add_response_header (response, "Connection", "close"); */
	    
	    MHD_add_response_header (response, "Content-Type",
				     lua_tostring (_L, -2) ?
				     lua_tostring (_L, -2) : "text/html");
	    
	    result = MHD_queue_response (connection, MHD_HTTP_OK, response);
	    MHD_destroy_response (response);

	    return result;
	} else {
	    struct MHD_Response *response;
	    int result;

	    const char *content=
		"<html><body>\r\n"
		"<h2>Not Found</h2>\r\n"
		"The requested page could not be found.\r\n"
		"</body></html>\r\n";

	    response = MHD_create_response_from_data (strlen(content),
						      (void *)content,
						      MHD_NO, MHD_NO);

	    MHD_add_response_header (response, "Server",
				     "Techne/" PACKAGE_VERSION);
	    /* MHD_add_response_header (response, "Connection", "close"); */
	    MHD_add_response_header (response, "Content-Type", "text/html");
	    
	    result = MHD_queue_response (connection,
					 MHD_HTTP_NOT_FOUND,
					 response);
	    MHD_destroy_response (response);

	    return result;
	}

	lua_settop (_L, h_0);
    } else {
	struct MHD_Response *response;
	int result;

	const char *content =
	    "<html><body>\r\n"
	    "<h2>Not Implemented</h2>\r\n"
	    "The request is not implemeted by this server.\r\n"
	    "</body></html>\r\n";

	response = MHD_create_response_from_data (strlen(content),
						  (void *)content,
						  MHD_NO, MHD_NO);

	MHD_add_response_header (response, "Server",
				 "Techne/" PACKAGE_VERSION);
	/* MHD_add_response_header (response, "Connection", "close"); */
	MHD_add_response_header (response, "Content-Type", "text/html");
	    
	result = MHD_queue_response (connection,
				     MHD_HTTP_NOT_IMPLEMENTED,
				     response);
	MHD_destroy_response (response);

	return result;
    }
}

static void cleanup (void *cls, struct MHD_Connection *connection,
		     void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct context *c = (struct context *)*con_cls;

    /* Free any allocated context data. */
    
    if (c) {
	if (c->data) {
	    free (c->data);
	}

	free (c);
    }

    connections -= 1;
}

static void run()
{
    if (http) {
	fd_set read, write, except;
	int n = 0;

	assert (connections >= 0);

	/* Wait for traffic to arrive if in blocking mode. */

	if (block && connections == 0) {
	    FD_ZERO(&read);
	    FD_ZERO(&write);
	    FD_ZERO(&except);
      
	    MHD_get_fdset(http, &read, &write, &except, &n);
	    select (n + 1, &read, &write, &except, NULL);
	}
    
	/* Check for http requests. */

	MHD_run (http);
    }
}

@implementation Network

-(id) init
{
    /* Create the page and mimetype tables. */
    
    lua_newtable (_L);
    pages = luaL_ref (_L, LUA_REGISTRYINDEX);
    
    lua_newtable (_L);
    mime = luaL_ref (_L, LUA_REGISTRYINDEX);
    
    return [super init];
}

-(void) iterate
{
    do {
	run();
    } while (block);
}
    
-(int) _get_http
{
    if (http) {
	lua_pushnumber(_L, port);
    } else {
	lua_pushnil (_L);
    }

    return 1;
}

-(int) _get_block
{
    lua_pushboolean (_L, block);

    return 1;
}

-(int) _get_pages
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, pages);

    return 1;
}

-(int) _get_mime
{
    lua_rawgeti(_L, LUA_REGISTRYINDEX, mime);

    return 1;
}

-(void) _set_http
{
    /* Close the current web server. */

    if (http) {
	MHD_stop_daemon (http);
    }

    /* And set up a new instance listening to the
       specified port. */
	
    port = lua_tonumber (_L, 3);

    if (port > 0) {
	http = MHD_start_daemon (MHD_USE_DEBUG,
				 port,
				 NULL, NULL, &respond, NULL,
				 MHD_OPTION_NOTIFY_COMPLETED,
				 cleanup, NULL, MHD_OPTION_END);
    }

    if (http) {
	printf ("Listening for HTTP requests on port %d.\n", port);
    } else {
	printf ("Could not set up port %d to listen "
		"for HTTP requests.\n", port);
    }
}

-(void) _set_block
{
    block = lua_toboolean (_L, 3);
}

-(void) _set_pages
{
    luaL_unref (_L, LUA_REGISTRYINDEX, pages);
    pages = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) _set_mime
{
    luaL_unref (_L, LUA_REGISTRYINDEX, mime);
    mime = luaL_ref (_L, LUA_REGISTRYINDEX);
}

@end
