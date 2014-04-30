#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include <alloca.h>
#include <ctype.h>

#define BEGIN_FIELD_TAG ("<@")
#define END_FIELD_TAG ("@>")
#define EVALUATION_TAG ("= ")
#define BEGIN_BLOCK_TAG ("begin ")
#define END_BLOCK_TAG ("finish ")
#define INSERT_BLOCK_TAG ("insert ")

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

int t_compiletemplate(lua_State *L, const char *source)
{
    char *c, *s, *copy;
    int intag = 0, inblock = 0, h, result;

    /* Make a local copy of the template source since we're going to
     * be performing open surgery on it. */

    if (!(copy = strdupa (source))) {
        lua_pushliteral (L, "Could not allocate memory for the template source.");
        return LUA_ERRMEM;
    }

    h = lua_gettop(L);
    lua_pushfstring (L, "local _blocks, context = {}, ... ");

    for (c = copy ; c ; ) {
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
                    if (inblock) {
                        lua_settop(L, h);
                        lua_pushstring (L, "Nested blocks are not allowed.");

                        return LUA_ERRSYNTAX;
                    }

                    /* Emit code to declare a new block, which is
                     * represented by a function which, when called
                     * prints out the contents of the block. */

                    inblock = 1;
                    lua_pushfstring (L,
                                     " function _blocks.%s()"
                                     " local _pieces = {} ",
                                     c + sizeof(BEGIN_BLOCK_TAG) - 1);
                } else if (!strncmp(c,
                                    END_BLOCK_TAG,
                                    sizeof(END_BLOCK_TAG) - 1)) {
                    if (!inblock) {
                        lua_settop(L, h);
                        lua_pushstring (L, "Superfluous block finish tag.");

                        return LUA_ERRSYNTAX;
                    }

                    /* Close the block function, concatenating all
                     * source pieces and returning the source. */

                    inblock = 0;
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

    lua_pushfstring (L, " return _blocks.main() ");
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

    /* Remove the source string. */

    lua_remove (L, h + 1);

    return result;
}

int t_rendertemplate(lua_State *L, const char *source)
{
    int result;

    if ((result = t_compiletemplate(L, source)) != LUA_OK) {
        return result;
    }

    lua_insert (L, -2);

    return lua_pcall(L, 1, 1, 0);
}
