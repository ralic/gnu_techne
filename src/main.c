/* Copyright (C) 2012 Papavasileiou Dimitris
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include <lualib.h>
#include <lauxlib.h>

#include <config.h>

#ifndef __WIN32__
#include <signal.h>
#endif

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#ifdef HAVE_LIBDW
#include <elfutils/libdwfl.h>
#endif

#include "prompt/prompt.h"
#include "profiling.h"
#include "techne.h"
#include "graphics.h"
#include "network.h"
#include "accoustics.h"
#include "input.h"
#include "dynamics.h"
#include "root.h"

#define COLOR(i, j) (colorize ? "\033\[" #i ";" #j "m" : "")

static int interactive = 0;   /* Accept input on the tty. */
static int quiet = 0;         /* Suppress messages. */

static int colorize = 1;      /* Color terminal output. */

static int message (lua_State *L)
{
    t_print_message(luaL_checkstring(L, 1));

    return 0;
}

static int warn (lua_State *L)
{
    t_print_warning(luaL_checkstring(L, 1));

    return 0;
}

static int hooks_call (lua_State *L)
{
    int i, h, h_1, n = 0;

    h = lua_gettop (L);

    /* Get the hooks table and loop through it. */

    lua_pushvalue (L, lua_upvalueindex(1));
    lua_pushnil (L);

    while (lua_next(L, -2)) {
        /* Push all arguments. */

        for (i = 0 ; i < h ; i += 1, lua_pushvalue(L, i + 1));

        if (n == 0) {
            t_call (L, h, LUA_MULTRET);

            /* If the hook returned any values stash them away to
             * return them to the caller once we've called all
             * hooks. */

            h_1 = lua_gettop(L);
            for (i = h + 3 ; i <= h_1 ; i += 1) {
                lua_insert (L, h + 1);
            }

            n = h_1 - h - 2;
        } else {
            /* We already have some values to return so discard
             * anything the hook might return. */

            t_call (L, h, 0);
        }
    }

    lua_pop(L, 1);

    return n;
}

static int hooks_index(lua_State *L)
{
    lua_pushvalue (L, lua_upvalueindex(1));
    lua_replace(L, 1);
    lua_gettable(L, 1);

    return 1;
}

static int hooks_newindex(lua_State *L)
{
    lua_pushvalue (L, lua_upvalueindex(1));
    lua_replace(L, 1);
    lua_settable(L, 1);

    return 0;
}

static int hooks_tostring (lua_State *L)
{
    lua_pushliteral (L, "Hooks");

    return 1;
}

static int construct_hooks(lua_State *L)
{
    lua_newuserdata (L, 0);

    lua_newtable (_L);
    lua_pushstring(_L, "__call");
    lua_pushvalue (_L, 1);
    lua_pushcclosure(_L, (lua_CFunction)hooks_call, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__index");
    lua_pushvalue (_L, 1);
    lua_pushcclosure(_L, (lua_CFunction)hooks_index, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__newindex");
    lua_pushvalue (_L, 1);
    lua_pushcclosure(_L, (lua_CFunction)hooks_newindex, 1);
    lua_settable(_L, -3);
    lua_pushstring(_L, "__tostring");
    lua_pushcfunction(_L, (lua_CFunction)hooks_tostring);
    lua_settable(_L, -3);
    lua_setmetatable(_L, -2);

    return 1;
}

static void pushstack(lua_State *L)
{
    lua_Debug ar;
    int i, h;

    h = lua_gettop (L);

    /* Print the Lua stack. */

    lua_pushfstring(L, "\n\n%sLua stack trace:\n", COLOR(0, 31));

    for (i = 0 ; lua_getstack (L, i, &ar) ; i += 1) {
        lua_getinfo(L, "Snl", &ar);

        if (!strcmp (ar.what, "C")) {
            lua_pushfstring (L, "%s#%d%s in C function %s'%s'%s\n",
                             COLOR(0, 31), i, COLOR(0,37),
                             COLOR(1, 37), ar.name, COLOR(0,37));
        } else if (!strcmp (ar.what, "main")) {
            lua_pushfstring (L, "%s#%d%s in the %smain chunk%s at %s%s:%d%s\n",
                             COLOR(0, 31), i, COLOR(0,37),
                             COLOR(1, 37), COLOR(0,37),
                             COLOR(1, 33), ar.short_src,
                             ar.currentline, COLOR(0,));
        } else if (!strcmp (ar.what, "Lua")) {
            lua_pushfstring (L, "%s#%d%s in function %s'%s'%s at %s%s:%d%s\n",
                             COLOR(0, 31), i, COLOR(0,37),
                             COLOR(1, 37), ar.name, COLOR(0,37),
                             COLOR(1, 33), ar.short_src,
                             ar.currentline, COLOR(0,));
        }
    }

    if (i == 0) {
        lua_pushstring (L, "No activation records.\n");
    }

    lua_pushfstring(L, COLOR(0,));
    lua_concat (L, lua_gettop(L) - h);
}

static int error_handler (lua_State *L)
{
    lua_pushstring(L, COLOR(1, 37));
    lua_insert (L, 1);

    pushstack(L);
    lua_concat (L, 2);

    return 1;
}

static void print_c_stack()
{
#ifdef HAVE_BACKTRACE
    void *frames[64];
    int i, n;

    t_print_error("\n%sC stack trace:\n", COLOR(0, 31));

    n = backtrace (frames, 64);

#ifdef HAVE_LIBDW
    {
        static Dwfl* dwfl;
        Dwfl_Callbacks callbacks;
        char *debuginfo_path = NULL;

        callbacks.find_elf = dwfl_linux_proc_find_elf;
        callbacks.find_debuginfo = dwfl_standard_find_debuginfo;
        callbacks.section_address = NULL;
        callbacks.debuginfo_path = &debuginfo_path;

        dwfl = dwfl_begin(&callbacks);

        if(dwfl == NULL ||
           dwfl_linux_proc_report (dwfl, getpid()) != 0 ||
           dwfl_report_end (dwfl, NULL, NULL) != 0) {
            if (dwfl) {
                dwfl_end(dwfl);
            }

            t_print_error("Could not read debugging symbols.\n");

            return;
        }

        for (i = 0 ; i < n ; i += 1) {
            Dwarf_Addr addr;
            Dwfl_Module *module;
            Dwfl_Line *line;
            const char *function;

            addr = (uintptr_t)frames[i];
            module = dwfl_addrmodule (dwfl, addr);
            line = dwfl_getsrc (dwfl, addr);
            function = dwfl_module_addrname(module, addr);

            t_print_error("%s#%-2d%s",
                          COLOR(0, 31), i, COLOR(0,37));

            if (function) {
                t_print_error("in %s%s%s ",
                              COLOR(1, 37), function, COLOR(0,37));
            }

            if(line) {
                Dwarf_Addr addr;
                const char *filename;
                int n;

                filename = dwfl_lineinfo (line, &addr, &n, NULL, NULL, NULL);
                t_print_error("at %s%s:%d%s\n",
                              COLOR(1, 33), basename(filename),
                              n, COLOR(0,));
            } else {
                t_print_error("at %s%p%s\n",
                              COLOR(1, 33), frames[i], COLOR(0,));
            }
        }

        dwfl_end(dwfl);
    }
#else
    {
        char **lines;
        lines = backtrace_symbols (frames, n);

        for (i = 0 ; i < n ; i += 1) {
            t_print_error("%s#%-2d%s %s\n",
                          COLOR(0, 31), i, COLOR(0,), lines[i]);
        }
    }
#endif
#endif
}

#ifndef __WIN32__
static void remove_handlers()
{
    struct sigaction action;

    /* Disable the handler for further occurences of this signal. */

    action.sa_handler = SIG_DFL;
    action.sa_flags = 0;
    sigemptyset (&action.sa_mask);

    sigaction (SIGILL, &action, NULL);
    sigaction (SIGABRT, &action, NULL);
    sigaction (SIGSEGV, &action, NULL);
    sigaction (SIGPIPE, &action, NULL);
    sigaction (SIGALRM, &action, NULL);
    sigaction (SIGFPE, &action, NULL);
    sigaction (SIGUSR1, &action, NULL);
    sigaction (SIGUSR2, &action, NULL);
}

static void handle_signal(int signum)
{
    remove_handlers();

    t_print_error("%sSignal received: %s", COLOR(1, 37), strsignal(signum));

    pushstack(_L);
    t_print_error(lua_tostring (_L, -1));
    lua_pop (_L, 1);

    print_c_stack();
    t_print_error("This is bad.  Aborting.\n");

    abort();
}
#endif

static int panic(lua_State *L)
{
#ifndef __WIN32__
    remove_handlers();
#endif

    t_print_error("%s%s", COLOR(1, 37), lua_tostring(L, -1));
    print_c_stack();
    t_print_error("\nThis is bad.  Aborting.\n");

    abort();
}

const char *t_ansi_color (int i, int j)
{
    char *strings[][2] = {
        {COLOR(31, 0),  COLOR(31, 1)},
        {COLOR(32, 0),  COLOR(32, 1)},
        {COLOR(33, 0),  COLOR(33, 1)},
        {COLOR(34, 0),  COLOR(34, 1)},
        {COLOR(35, 0),  COLOR(35, 1)},
        {COLOR(36, 0),  COLOR(36, 1)},
        {COLOR(37, 0),  COLOR(37, 1)}
    };

    if (i == 0 && j == 0) {
        return COLOR(0,);
    } else {
        assert (i >= 31 && i <= 37 && j >= 0 && j <= 1);

        return strings[i - 31][j];
    }
}

int t_call (lua_State *L, int nargs, int nresults)
{
    static int nesting;
    int r, h_0;

    nesting += 1;

    if (nesting == 1) {
        /* Push the message handler and call the function. */

        h_0 = lua_gettop(L) - nargs;
        lua_pushcfunction (L, error_handler);
        lua_insert (L, h_0);
        r = lua_pcall (L, nargs, nresults, h_0);

        /* Pop the message handler from the stack. */

        lua_remove (L, h_0);
        nesting -= 1;

        if (r) {
            /* Pop the error message in case of failure. */

            return lua_error (L);
        } else {
            return r;
        }
    } else {
        /* This is a nested call, so a handler is already
         * established. */

        lua_call (L, nargs, nresults);
        nesting -= 1;

        return 0;
    }
}

void t_print_message (const char *format, ...)
{
    va_list argp;

    if (quiet < 1) {
        va_start(argp, format);
        vfprintf(stderr, format, argp);
        va_end(argp);
    }
}

void t_print_warning (const char *format, ...)
{
    va_list argp;

    if (quiet < 2) {
        fprintf (stderr, COLOR(1, 33));
        va_start(argp, format);
        vfprintf(stderr, format, argp);
        va_end(argp);
        fprintf (stderr, COLOR(0,));
    }
}

void t_print_error (const char *format, ...)
{
    va_list argp;

    if (quiet < 3) {
        va_start(argp, format);
        vfprintf(stderr, format, argp);
        va_end(argp);
    }
}

int main(int argc, char **argv)
{
    Node *node;

    static struct option options[] = {
        {"option", 1, 0, 'O'},
        {"execute", 1, 0, 'e'},
        {"profile", 0, 0, 'p'},
        {"load", 1, 0, 'l'},
        {"load-all", 1, 0, 'a'},
        {"interactive", 0, 0, 'i'},
        {"quiet", 0, 0, 'q'},
        {"configuration", 1, 0, 'c'},
        {"colorize", 1, 0, 'L'},
        {"help", 0, 0, 'h'},
        {0, 0, 0, 0}
    };

    int i, option, profile = 0;

    _L = luaL_newstate();

    /* Only output color text if the output is an interactive
     * terminal. */

    if (!isatty (STDOUT_FILENO) ||
        !isatty (STDERR_FILENO)) {
        colorize = 0;
    } else {
        colorize = 1;
    }

#ifndef __WIN32__
    /* Set up signal handlers for some signals so that we can attempt
     * to provide stack traces. */

    {
        struct sigaction action, ignore;

        action.sa_handler = handle_signal;
        action.sa_flags = 0;
        sigemptyset (&action.sa_mask);

        sigaction (SIGILL, &action, NULL);
        sigaction (SIGABRT, &action, NULL);
        sigaction (SIGSEGV, &action, NULL);
        sigaction (SIGPIPE, &action, NULL);
        sigaction (SIGALRM, &action, NULL);
        sigaction (SIGFPE, &action, NULL);
        sigaction (SIGUSR1, &action, NULL);
        sigaction (SIGUSR2, &action, NULL);

        /* Install a handler to ignore broken pipes. */

        ignore.sa_handler = SIG_IGN;
        ignore.sa_flags = 0;
        ignore.sa_restorer = NULL;

        sigemptyset(&ignore.sa_mask);
        sigaction(SIGPIPE, &ignore, NULL);
    }
#endif

    luaL_requiref(_L, "package", luaopen_package, 0);

    /* Modify the Lua path and cpath. */

    lua_pushstring (_L, PKGDATADIR"/modules/?.lua;"
                        PKGDATADIR"/modules/?/init.lua;");
    lua_getfield (_L, -2, "path");

    lua_concat (_L, 2);
    lua_setfield (_L, -2, "path");

    lua_pushstring (_L, PKGLIBDIR"/?.so;");
    lua_getfield (_L, -2, "cpath");
    lua_concat (_L, 2);
    lua_setfield (_L, -2, "cpath");

    /* Add module loaders for all internal modules. */

    lua_getfield (_L, -1, "preload");
    {
        const struct {
            const char *name;
            lua_CFunction loader;
        } list[] = {
            {"techne", luaopen_techne},
            {"graphics", luaopen_graphics},
            {"dynamics", luaopen_dynamics},
            {"accoustics", luaopen_accoustics},
            {"input", luaopen_input},
            {"network", luaopen_network},

            {"coroutine", luaopen_coroutine},
            {"string", luaopen_string},
            {"table", luaopen_table},
            {"math", luaopen_moremath},
            {"bit32", luaopen_bit32},
            {"io", luaopen_io},
            {"os", luaopen_os},
            {"debug", luaopen_debug},
        };
        int i;

        for (i = 0 ; i < sizeof(list) / sizeof(list[0]) ; i += 1) {
            lua_pushstring(_L, list[i].name);
            lua_pushcfunction(_L, list[i].loader);
            lua_settable(_L, -3);
        }
    }

    lua_pop (_L, 2);

    lua_atpanic (_L, panic);

    /* Load the base library. */

    luaL_requiref(_L, "base", luaopen_morebase, 1);
    lua_pop(_L, 1);

    /* Create the options table. */

    lua_newtable (_L);

    /* Parse the command line. */

    while ((option = getopt_long (argc, argv,
                                  "O:c:e:l:ahiqp",
                                  options, 0)) != -1) {
        if (option == 'O') {
            char *equal;

            equal = strchr (optarg, '=');

            if (equal) {
                *equal = '\0';
            }

            lua_pushstring (_L, optarg);

            /* First get the existing option. */

            lua_pushstring (_L, optarg);
            lua_gettable (_L, -3);

            /* If there is only one encapsulate it. */

            if (lua_type (_L, -1) == LUA_TNIL) {
                lua_pop (_L, 1);
            } else if (lua_type (_L, -1) != LUA_TTABLE) {
                lua_newtable (_L);
                lua_insert (_L, -2);
                lua_rawseti (_L, -2, 1);
            }

            /* Now push the specified value. */

            if (equal) {
                lua_pushstring (_L, equal + 1);

                /* Convert to number if possible. */

                if (lua_isnumber (_L, -1)) {
                    lua_pushnumber (_L, lua_tonumber (_L, -1));
                    lua_replace (_L, -2);
                }
            } else {
                lua_pushboolean (_L, 1);
            }

            /* If this is a duplicate option
               insert it into the table. */

            if (lua_type (_L, -2) == LUA_TTABLE) {
                i = lua_rawlen (_L, -2);
                lua_rawseti (_L, -2, i + 1);
            }

            /* Finally set the option value. */

            lua_settable (_L, -3);
        } else if (option == 'e') {
            /* Compile the chunk and stash it away behind the options
             * table. */

            if(luaL_loadbuffer (_L, optarg, strlen(optarg),
                              "=command line")) {
                lua_error (_L);
            } else {
                lua_insert (_L, 1);
            }
        } else if (option == 'i') {
            interactive = 1;
        } else if (option == 'q') {
            quiet += 1;
        } else if (option == 'l' || option == 'a') {
            const char *s;
            char **list, *all[] = {
                "techne", "graphics", "dynamics",
                "accoustics", "input", "network",

                "coroutine", "string", "table",
                "math", "bit32", "io", "os", "debug",

                "serialize", "units", "bindings",
                "resources", "array", "arraymath",
                "joints", "primitives", "bodies",
                "shading", "shapes", "automotive",
                "widgets", "controllers", "textures",
                "topography", "physics"
            };
            int i, n;

            if (optarg) {
                list = &optarg;
                n = 1;
            } else {
                list = all;
                n = sizeof(all) / sizeof(all[0]);
            }

            /* Pretend the -e switch was used with the input:
               module_name = require 'modulename' */

            for (i = 0 ; i < n ; i += 1) {
                lua_pushfstring (_L, "%s = require '%s'", list[i], list[i]);
                s = lua_tostring(_L, -1);

                if(luaL_loadbuffer (_L, s, strlen(s), "=command line")) {
                    lua_error (_L);
                } else {
                    lua_insert (_L, 1);
                }

                lua_pop (_L, 1);
            }
        } else if (option == 'L') {
            if (!strcmp (optarg, "always")) {
                colorize = 1;
            } else if (!strcmp (optarg, "never")) {
                colorize = 0;
            } else if (strcmp (optarg, "auto")) {
                t_print_warning ("Ignoring unknown color "
                                 "setting '%s'.\n", optarg);
            }
            luap_setcolor (_L, colorize);
        } else if (option == 'c') {
            char *path = NULL;
            int i, n;

            if (!access (optarg, F_OK)) {
                path = strdup(optarg);
            } else {
                lua_getfield (_L, -1, "prefix");

                /* Wrap a single prefix inside a table. */

                if (lua_type (_L, -1) == LUA_TSTRING) {
                    lua_createtable (_L, 1, 0);
                    lua_insert (_L, -2);
                    lua_rawseti (_L, -2, 1);
                }

                if (lua_type (_L, -1) == LUA_TTABLE) {
                    n = lua_rawlen (_L, -1);

                    /* Try to find the specified script in a bunch of
                       predetermined directories. */

                    for (i = 0 ; i < n ; i += 1) {
                        lua_rawgeti(_L, -1, i + 1);

                        asprintf (&path, "%s/%s",
                                  lua_tostring(_L, -1), optarg);

                        lua_pop(_L, 1);

                        if (!access (path, F_OK)) {
                            break;
                        } else {
                            free (path);
                        }
                    }

                    if (i == n) {
                        path = NULL;
                    }
                }

                lua_pop(_L, 1);
            }

            if (!path) {
                t_print_warning("Could not find input file '%s'.\n",
                                optarg);
            } else {
                /* Compile the chunk and stash it away behind the options
                 * table. */

                if(luaL_loadfile (_L, path)) {
                    lua_error (_L);
                } else {
                    lua_insert (_L, 1);
                }

                t_print_message ("Compiled the input file %s\n", path);

                free (path);
            }
        } else if (option == 'p') {
            profile = 1;
        } else if (option == 'h') {
            t_print_message (
                "Usage: %s [OPTION...]\n\n"
                "Options:\n"
                "  -h, --help                                     "
                "Display this help message.\n"
                "  -c FILE, --configuration=FILE                  "
                "Load the specified input file.\n"
                "  -l MODULE, --load=MODULE                       "
                "Load the specified module during initialization.\n"
                "  -a, --load-all                                 "
                "Load all modules during initialization.\n"
                "  -O OPTION[=VALUE], --option OPTION[=VALUE]     "
                "Set the option OPTION with value VALUE.\n"
                "  -i, --interactive                              "
                "Drop to an interactive shell after initializing.\n"
                "  -e STAT, --execute=STAT                        "
                "Execute the provided statement.\n"
                "  -p, --profile                                  "
                "Enable profiling.\n"
                "  --colorize=always|never|auto                   "
                "Specify when to color console output.\n"
                "  -q, --quiet                                    "
                "Suppress output messages.\n", argv[0]);

            exit(1);
        } else {
            exit(1);
        }
    }

    lua_setglobal (_L, "options");

    if (profile) {
        t_enable_profiling();
    }

    /* Initialize the interactive interpreter. */

    luap_sethistory (_L, "~/.techne_history");
    luap_setname (_L, "techne");
    luap_setcolor (_L, colorize);

    lua_pushcfunction (_L, construct_hooks);
    lua_setglobal (_L, "hooks");

    lua_pushcfunction (_L, message);
    lua_setglobal (_L, "message");

    lua_pushcfunction (_L, warn);
    lua_setglobal (_L, "warn");

    /* Run the boot scripts. */

    lua_getglobal(_L, "require");
    lua_pushstring(_L, "boot");
    assert(lua_pcall(_L, 1, 0, 0) == LUA_OK);

    /* Proceed to execute specified input. */

    if (lua_gettop(_L) > 0) {
        t_print_message ("Executing input scripts.\n");

        /* Run the provided input files. */

        while(lua_gettop(_L) > 0) {
            assert (lua_type (_L, -1) == LUA_TFUNCTION);
            t_call (_L, 0, 0);
        }
    } else {
        t_print_message ("No input files provided.\n");
    }

    if (interactive) {
        t_print_message ("Dropping to a shell.  Press ctrl-d to continue.\n");

        luap_enter(_L);
    }

    if ([Techne instance]) {
        [[Techne instance] iterate];
    }

    /* Unlink all root nodes. */

    for (node = [Root nodes] ; node ; node = node->right) {
        [node toggle];
    }

    t_print_message("Bye!\n");
    exit (EXIT_SUCCESS);
}
