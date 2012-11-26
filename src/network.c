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
#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <microhttpd.h>

#include "../config.h"
#include "techne.h"
#include "network.h"

#define BEGIN_FIELD_TAG ("<@")
#define END_FIELD_TAG ("@>")
#define EVALUATION_TAG ("= ")
#define BEGIN_BLOCK_TAG ("begin ")
#define END_BLOCK_TAG ("finish ")
#define INSERT_BLOCK_TAG ("insert ")

struct context {
    char *data;
    size_t size;
};    

static int connections, pages, port, block;
static struct MHD_Daemon *http;

static int calculate_level(const char *s)
{
    int i, started, level, uselevel = 0;
	
    /* Scan the string to decide how to print it. */
	
    for (i = 0, started = 0 ; s[i] ; i += 1) {
        
        /* Check what long string delimeter level to use so that
         * the string won't be closed prematurely. */
	    
        if (!started) {
            if (s[i] == ']') {
                started = 1;
                level = 0;
            }
        } else {
            if (s[i] == '=') {
                level += 1;
            } else if (s[i] == ']') {
                if (level >= uselevel) {
                    uselevel = level + 1;
                }
            } else {
                started = 0;
            }
        }
    }

    return uselevel;
}

static int compile_template(lua_State *L)
{
    char *c, *s, *source;
    int intag = 0, inblock = 0, h, result;

    /* Make a local copy of the template source since we're going to
     * be performing open surgery on it. */
    
    source = alloca (lua_rawlen (L, -1) + 1);
    if (!source) {
	lua_pushliteral (L, "Could not allocate memory for the template source.");
	return LUA_ERRMEM;
    }
    
    strcpy (source, lua_tostring (L, -1));
    
    h = lua_gettop(L);
    lua_pushfstring (L, "local _blocks, request = {}, ... ");

    for (c = source ; c ; ) {
	/* At each point we can be scanning inside a tag or
	 * undelimited data and. */
	
        if (!intag) {
            char *equal;
            int i, n;

	    /* Look for the start of the tag delimiter. */
	    
            s = strstr (c, BEGIN_FIELD_TAG);

	    /* If we're in a block add a statement to print all
	     * input otherwise complain if non-whitespace input
	     * is read. */
		
            if (inblock) {
                if (s) {
                    *s = '\0';

		    /* Calculate what long string delimiters to use so
		     * that the string won't be closed prematurely. */
		    
		    n = calculate_level(c);
		    for (i = 0, equal = alloca(n + 1);
			 i < n;
			 equal[i] = '=', i += 1);
		    equal[n] = '\0';
            
		    lua_pushfstring (L, " _pieces[#_pieces + 1] = [%s[%s]%s]; ",
				     equal, c, equal);
                } else {
		    /* Complain if no start-of-tag delimiter was found
		     * and we're inside a block.  This means the block
		     * will not be properly closed. */
		
		    lua_settop(L, h);
		    lua_pushstring (L,
				    "Read past end of input while "
				    "inside a block.");

		    return LUA_ERRSYNTAX;		    
		}
            } else {
		for (; s ? c < s : *c != '\0' ; c += 1) {
		    if (!isspace (*c)) {
			lua_settop(L, h);
			lua_pushstring (L, "Read non-whitespace input outside of any block.");

			return LUA_ERRSYNTAX;
		    }			
		}
	    }

	    /* Fast-forward to the start of the tag and prepare to
	     * scan it.  If no tag was found finish.*/
	    
            if (s) {
                c = s + sizeof(BEGIN_FIELD_TAG) - 1;
                intag = 1;
            } else {
                break;
            }
        } else {
	    /* Look for the end of the tag delimiter. */
	    
            s = strstr (c, END_FIELD_TAG);

            if (s) {
		*s = '\0';

		/* Scan the begining of the tag to decide what it is. */
		
		if (!strncmp(c,
			     EVALUATION_TAG,
			     sizeof(EVALUATION_TAG) - 1)) {
		    /* Insert a statement to evaluate the expression
		     * inside the tag and print it to the output. */
		    
		    lua_pushfstring (L,
				     " _pieces[#_pieces + 1] = "
				     "tostring(%s); ",
				     c + sizeof(EVALUATION_TAG) - 1);
		} else if (!strncmp(c,
				    INSERT_BLOCK_TAG,
				    sizeof(INSERT_BLOCK_TAG) - 1)) {
		    int invalid;
		    char *s, *t;

		    for (s = c + sizeof(INSERT_BLOCK_TAG) - 1;
			 isspace(*s);
			 s += 1);

		    for (t = s, invalid = 0 ; !isspace(*t) ; t += 1) {
			if (!isalpha(*t) && *t != '_') {
			    invalid = 1;
			}
		    }
		    
		    *t = '\0';

		    if (invalid) {
			lua_settop(L, h);
			lua_pushfstring (L, "Invalid block name '%s'.", s);

			return LUA_ERRSYNTAX;
		    }
		    
		    /* Insert a statement to insert a block into the
		     * output. */
		    
		    lua_pushfstring (L,
				     " if not _blocks.%s then"
				     " error 'Undefined block \\\'%s\\\'.'"
				     " else"
				     " _pieces[#_pieces + 1] = "
				     "tostring(_blocks.%s())"
				     " end ", s, s, s);
		} else if (!strncmp(c,
				    BEGIN_BLOCK_TAG,
				    sizeof(BEGIN_BLOCK_TAG) - 1)) {
		    /* Emit code to declare a new block, which is
		     * represented by a function which, when called
		     * prints out the contents of the block. */
		    
		    inblock += 1;
		    lua_pushfstring (L,
				     " function _blocks.%s()"
				     " local _pieces = {} ",
				     c + sizeof(BEGIN_BLOCK_TAG) - 1);
		} else if (!strncmp(c,
				    END_BLOCK_TAG,
				    sizeof(END_BLOCK_TAG) - 1)) {
		    /* Close the block function, concatenating all
		     * source pieces and returning the source. */
		    
		    inblock -= 1;
		    lua_pushliteral (L,
				     " local _source = ''"
				     " for i = 1, #_pieces do"
				     " _source = _source .. _pieces[i]"
				     " end"
				     " return _source"
				     " end ");
		} else {
		    /* Simple blocks just contain Lua code that should
		     * be copied verbatim. */
		    
		    lua_pushstring (L, c);
		}

		c = s + sizeof(END_FIELD_TAG) - 1;
		intag = 0;
	    } else {
		lua_settop(L, h);
                lua_pushstring (L,
				"Read past end of input while inside "
				"a field.");

		return LUA_ERRSYNTAX;
            }
	}
    }

    /* Emit the postamble and concatenate all pieces. */
    
    lua_pushfstring (_L, " return _blocks.main() ");    
    lua_concat (L, lua_gettop(L) - h);

    /* puts("####"); */
    /* puts(lua_tostring(_L, h + 1)); */
    /* puts("####"); */

    /* Try to load the generated code and remove both it and the
     * supplied source. */
    
    result = luaL_loadbuffer(L,
			     lua_tostring(L, h + 1),
			     lua_rawlen(L, h + 1),
			     "=template");
    lua_remove (L, h);
    lua_remove (L, h);
    
    return result;
}

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
			    
	lua_rawgeti (_L, LUA_REGISTRYINDEX, pages);

	if (lua_istable (_L, -1)) {
	    lua_pushstring (_L, url);
	    lua_gettable (_L, -2);
	    lua_replace (_L, -2);

	    if (lua_istable(_L, -1)) {
		lua_rawgeti (_L, -1, 1);
		lua_rawgeti (_L, -2, 2);
		lua_remove (_L, -3);
	    } else {
		lua_pushliteral (_L, "text/html");
		lua_insert (_L, -2);
	    }

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

	    if (!compile_template (_L)) {
		lua_newtable (_L);
		lua_pushstring (_L, method);
		lua_setfield (_L, -2, "method");
		
		lua_pushstring (_L, url);
		lua_setfield (_L, -2, "url");

		lua_pushvalue (_L, -5);
		lua_setfield (_L, -2, "parameters");

		lua_pushvalue (_L, -5);
		lua_setfield (_L, -2, "data");

		lua_pushstring (_L, version);
		lua_setfield (_L, -2, "version");

		lua_pcall(_L, 1, 1, 0);
	    }
	    
	    /* Create and queue the response. */

	    response = MHD_create_response_from_data (lua_rawlen (_L, -1),
						      (void *)lua_tostring (_L, -1),
						      MHD_NO, MHD_YES);

	    MHD_add_response_header (response, "Server",
				     "Techne/" PACKAGE_VERSION);
	    MHD_add_response_header (response, "Cache-Control", "no-store");
	    /* MHD_add_response_header (response, "Connection", "close"); */
	    
	    MHD_add_response_header (response, "Content-Type",
				     lua_tostring (_L, -2));
	    
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

-(void) init
{
    [super init];
    self->index = 2;

    lua_pushstring (_L, "network");
    lua_setfield (_L, -2, "tag");
    
    /* Create the page table. */
    
    lua_newtable (_L);
    pages = luaL_ref (_L, LUA_REGISTRYINDEX);
}

-(void) iterate
{
    t_begin_interval(self);

    do {
	run();
    } while (block);

    t_end_interval(self);
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

@end
