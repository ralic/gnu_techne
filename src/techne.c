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

#include <config.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

#ifndef __WIN32__
#include <signal.h>
#endif

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

#include <lualib.h>
#include <lauxlib.h>

#include "techne.h"
#include "prompt/prompt.h"
#include "transform.h"
#include "dynamics.h"
#include "network.h"
#include "input.h"
#include "graphics.h"
#include "shader.h"
#include "accoustics.h"

static int quiet = 0;         /* Suppress messages. */
static int engineering = 0;   /* Disable visual rendering. */
static int mute = 0;          /* Disable audio rendering. */

static int iterate = 0;       /* Keep the main loop running. */
static int interactive = 0;   /* Accept input on the tty. */

static int colorize = 1;      /* Color terminal output. */
static int iterations = 0;

#define COLOR(i, j) (colorize ? "\033\[" #i ";" #j "m" : "")

static char *name = "techne", *class = "Techne";
static long long int beginning;

static Input *input;
static Network *network;
static Dynamics *dynamics;
static Graphics *graphics;
static Accoustics *accoustics;
static Techne *techne;
static Transform *root;

static void push_lua_stack(lua_State *L)
{
    lua_Debug ar;
    int i, h;

    h = lua_gettop (L);

    /* Print the Lua stack. */
    
    lua_pushfstring(L, "\n\n%sLua stack trace:\n", COLOR(0, 31));
    
    for (i = 0 ; lua_getstack (L, i, &ar) ; i += 1) {
    	lua_getinfo(L, "Snl", &ar);

    	if (!strcmp (ar.what, "C")) {
    	    lua_pushfstring (L, "%s#%d%s in C function %s'%s'\n",
			     COLOR(0, 31), i, COLOR(0, 37), COLOR(1, 33),
			     ar.name);
    	} else if (!strcmp (ar.what, "main")) {
    	    lua_pushfstring (L, "%s#%d %s%s:%s%d%s in the main chunk\n",
			     COLOR(0, 31), i, COLOR(0, 33), ar.short_src,
			     COLOR(1, 33), ar.currentline, COLOR(0, 37));
    	} else if (!strcmp (ar.what, "Lua")) {
    	    lua_pushfstring (L, "%s#%d %s%s:%s%d%s in function %s'%s'\n",
			     COLOR(0, 31), i, COLOR(0, 33), ar.short_src,
			     COLOR(1, 33), ar.currentline, COLOR(0, 37), 
			     COLOR(1, 33), ar.name);
    	}
    }

    if (i == 0) {
    	lua_pushstring (L, "No activation records.\n");
    }

    lua_pushfstring(L, COLOR(0,));
    lua_concat (L, lua_gettop(L) - h);
}

static int handler (lua_State *L)
{
    lua_pushstring(L, COLOR(1, 37));
    lua_insert (L, 1);

    push_lua_stack(L);
    lua_concat (L, 2);
    
    return 1;
}

static void print_c_stack()
{
#ifdef HAVE_BACKTRACE
    void *frames[64];
    char **lines;
    int i, n;
    
    /* Print the C stack. */

    t_print_error("\n%sC stack trace:\n", COLOR(0, 31));
    
    n = backtrace (frames, 64);
    lines = backtrace_symbols (frames, n);

    for (i = 0 ; i < n ; i += 1) {
	t_print_error("%s#%-2d%s %s\n", COLOR(0, 31), i, COLOR(0,), lines[i]);
    }
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

    push_lua_stack(_L);
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

    push_lua_stack(_L);
    t_print_error(lua_tostring (_L, -1));
    lua_pop (_L, 1);
    
    print_c_stack();

    t_print_error("\nThis is bad.  Aborting.\n");
    
    abort();
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

static int replacemetatable(lua_State *L)
{
    luaL_checktype (L, 1, LUA_TUSERDATA);
    luaL_checktype (L, 2, LUA_TTABLE);

    if (lua_getmetatable (L, 1)) {
	/* If the value already has a metatable, copy any fields that
	 * are not set in the supplied one. */
	
	lua_pushnil (L);
	
	while (lua_next (L, -2)) {
	    /* Check to see whether the field is overriden and if not
	     * copy it to the new metatable. */
	    
	    lua_pushvalue (L, -2);
	    lua_gettable (L, 2);

	    if (lua_isnil (L, -1)) {
		
		lua_pop (L, 1);
		lua_pushvalue (L, -2);
		lua_insert (L, -2);		
		lua_settable (L, 2);
	    } else {
		lua_pop (L, 2);
	    }
	}

	lua_pop (L, 1);
    }

    lua_setmetatable (L, 1);

    return 1;
}

static void accumulate(Node *root, long long int (*intervals)[2]) {
    Node *child;
    int j;
    
    for (j = 0 ; j < T_PHASE_COUNT ; j += 1) {
	intervals[j][0] += root->profile.intervals[j][0];
	intervals[j][1] += root->profile.intervals[j][1];
    }
    
    for (child = root->down ; child ; child = child->right) {
	accumulate (child, intervals);
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
	lua_pushcfunction (L, handler);
	lua_insert (L, h_0);
	r = lua_pcall (L, nargs, nresults, h_0);
    
	/* Pop the message handler from the stack. */
    
	lua_remove (L, h_0);

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

	return 0;
    }
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

@implementation Techne
-(id) initWithArgc: (int)argc andArgv: (char **)argv
{
    static struct option options[] = {
	{"option", 1, 0, 'O'},
	{"engineering", 0, 0, 'E'},
	{"execute", 1, 0, 'e'},
	{"interactive", 0, 0, 'i'},
	{"mute", 0, 0, 'm'},
	{"quiet", 0, 0, 'q'},
	{"configuration", 1, 0, 'c'},
	{"name", 1, 0, 'n'},
	{"class", 1, 0, 'C'},
	{"color", 1, 0, 'L'},
	{"help", 0, 0, 'h'},
	{0, 0, 0, 0}
    };

    int i, option;

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
    
    /* Modify the Lua path and cpath. */

    luaL_requiref(_L, "package", luaopen_package, 0);
    lua_pushstring (_L, PKGDATADIR"/modules/?.lua;"PKGDATADIR"/modules/?.lc;");
    lua_getfield (_L, -2, "path");

    lua_concat (_L, 2);
    lua_setfield (_L, -2, "path");

    lua_pushstring (_L, PKGLIBDIR"/?.so;");
    lua_getfield (_L, -2, "cpath");
    lua_concat (_L, 2);
    lua_setfield (_L, -2, "cpath");
    lua_pop (_L, 1);

    /* Load the console module on startup. */

    lua_getglobal(_L, "require");
    lua_pushstring(_L, "console");
    if (lua_pcall(_L, 1, 0, 0) != LUA_OK) {
	lua_pop (_L, 1);
    }

    lua_atpanic (_L, panic);

    /* Create the options table. */

    lua_newtable (_L);
    
    /* Parse the command line. */
    
    while ((option = getopt_long (argc, argv,
				  "O:c:d:e:hEimq",
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
	    
	} else if (option == 'n') {
	    name = optarg;
	} else if (option == 'C') {
	    class = optarg;
	} else if (option == 'E') {
	    engineering = 1;
	} else if (option == 'i') {
	    interactive = 1;
	} else if (option == 'm') {
	    mute = 1;
	} else if (option == 'q') {
	    quiet += 1;
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
	    const char *prefixes[] = {"./", PKGDATADIR"/scripts/", ""};
	    char *path;
	    int i;
		
	    /* Try to find the specified script in a bunch of
	       predetermined directories. */
		
	    for (i = 0;
		 i < sizeof(prefixes) / sizeof (char *);
		 i += 1) {
		asprintf (&path, "%s%s", prefixes[i], optarg);

		if (!access (path, F_OK)) {
		    break;
		}
	    }

	    /* Compile the chunk and stash it away behind the options
	     * table. */
	    
	    if(luaL_loadfile (_L, path)) {
		lua_error (_L);
	    } else {
		lua_insert (_L, 1);
	    }

	    t_print_message ("Compiled input file %s\n", path);
	} else if (option == 'h') {
	    t_print_message (
		"Usage: %s [OPTION...]\n\n"
		"Options:\n"
		"  -h, --help                                     "
		"Display this help message.\n"
		"  -c FILE, --configuration=FILE                  "
		"Specify a configuration script.\n"
		"  -O OPTION[=VALUE], --option OPTION[=VALUE]     "
		"Set the option OPTION with value VALUE.\n"
		"  -i, --interactive                              "
		"Drop to an interactive shell after initializing.\n"
		"  -e STAT, --execute=STAT                        "
		"Execute the provided statement.\n"
		"  -E, --engineering                              "
		"Disable rendering and simulate only.\n"
		"  -m, --mute                                     "
		"Disable audio rendering.\n"
		"  -q, --quiet                                    "
		"Suppress output messages.\n"
		"  --name=NAME                                    "
		"Set the application name.\n"
		"  --class=CLASS                                  "
		"Set the application class.\n", argv[0]);

	    exit(1);
	} else {
	    exit(1);
	}
    }

    lua_setglobal (_L, "options");

    /* Initialize the interactive interpreter. */

    luap_sethistory (_L, "~/.techne_history");
    luap_setname (_L, "techne");
    luap_setcolor (_L, colorize);

    lua_pushcfunction (_L, construct_hooks);
    lua_setglobal (_L, "hooks");
    
    lua_pushcfunction (_L, replacemetatable);
    lua_setglobal (_L, "replacemetatable");

    /* Greet the user. */

    t_print_message ("This is Techne, version %s.\n", VERSION);
    t_print_timing_resolution();
    
    input = [[Input alloc] init];
    lua_setglobal (_L, "input");
    
    network = [[Network alloc] init];
    lua_setglobal (_L, "network");
    
    dynamics = [[Dynamics alloc] init];
    lua_setglobal (_L, "dynamics");

    if (!engineering) {
	graphics = [[Graphics alloc] initWithName: name andClass: class];
	lua_setglobal (_L, "graphics");
    } else {
	t_print_message ("Visual output has been disabled.\n");
    }

    if (!mute && !engineering) {
	accoustics = [[Accoustics alloc] init];
	lua_setglobal (_L, "accoustics");
    } else {
	t_print_message ("Audio output has been disabled.\n");
    }

    self = [super init];
    lua_setglobal (_L, "techne");
    
    /* Create the root. */

    root = [[Transform alloc] init];
    lua_setglobal (_L, "root");

    [dynamics toggle];
    [graphics toggle];
    [accoustics toggle];
    [techne toggle];
    [root toggle];
    
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

    t_print_message ("Spent %f CPU seconds initializing.\n",
		     (double)t_get_cpu_time() / 1e9);
    
    return self;
}

-(void) iterate
{
    Node *node;
    long long int runtime, totals[2], intervals[T_PHASE_COUNT][2];;
    char *phases[T_PHASE_COUNT] = {"begin", "input", "step",
				   "transform", "prepare", "traverse",
				   "finish"};
    double c;
    int j;

    beginning = t_get_cpu_time();
    
    while (iterate || interactive) {
	if (interactive) {
	    luap_enter(_L);

	    interactive = 0;
	}
	
	t_begin_interval (root, T_BEGIN_PHASE);    
	[root begin];
	t_end_interval (root, T_BEGIN_PHASE);	

	[input iterate: root];
	[dynamics iterate: root];
	[graphics iterate: root];
	[network iterate];

        iterations += 1;
    }

    runtime = t_get_cpu_time() - beginning;

    [dynamics toggle];
    [graphics toggle];
    [accoustics toggle];
    [techne toggle];
    [root toggle];

    c = 100.0 / runtime;
    memset (intervals, 0, sizeof(intervals));
    memset (totals, 0, sizeof(totals));

    accumulate (root, intervals);
    for (node = root->right ; node != root ; node = node->right) {
	accumulate(node, intervals);
    }
	
    for (j = 0 ; j < T_PHASE_COUNT ; j += 1) {
	totals[0] += intervals[j][0] - intervals[j][1];
	totals[1] += intervals[j][1];
    }

    t_print_message("Ran a total of %d iterations in %.1f seconds at "
                    "%.1f iterations per second.\n"
                    "Spent %.1f (%.1f%%) in the core and %.1f (%.1f%%) in "
		    "the application.\n"
                    "Spent %.2f seconds (%.1f%%) processing collected"
		    " nodes or in unprofiled code.\n",
		    iterations, runtime * 1e-9, iterations * 1e9 / runtime, 
                    totals[0] * 1e-9, c * totals[0], totals[1] * 1e-9,
                    c * totals[1],  (runtime - totals[0] - totals[1]) * 1e-9,
		    (double)(runtime - totals[0] - totals[1]) /
		    runtime * 100, COLOR(0,));
    
    t_print_message("Phase profile:\n"
		    "+-----------------------+\n"
		    "|phase       core   user|\n"
		    "+-----------------------+\n");
	
    for (j = 0 ; j < T_PHASE_COUNT ; j += 1) {
	t_print_message("|%-10s %4.1f%%  %4.1f%%|\n",
			phases[j],
			c * (intervals[j][0] - intervals[j][1]),
			c * intervals[j][1]);
    }

    t_print_message("+-----------------------+\n"
		    "|%-10s %4.1f%%  %4.1f%%|\n"
		    "+-----------------------+\n",
		    "total", c * totals[0], c * totals[1]);
    
    /* Close the state to collect all values and call the respective
     * finalizers. */
    
    lua_close (_L);
}

-(int) _get_interactive
{
    lua_pushboolean (_L, interactive);
    
    return 1;
}

-(int) _get_iterate
{
    lua_pushboolean (_L, iterate);
    
    return 1;
}
 
-(int) _get_time
{
    lua_pushnumber (_L, (t_get_cpu_time() - beginning) / 1e9);
    
    return 1;
}
	
-(int) _get_iterations
{
    lua_pushnumber (_L, iterations);

    return 1;
}

-(void) _set_time
{
    T_WARN_READONLY;
}

-(void) _set_iterate
{
    iterate = lua_toboolean (_L, -1);
}

-(void) _set_interactive
{
    interactive = lua_toboolean (_L, -1);
}
	
-(void) _set_iterations
{
    T_WARN_READONLY;
}

@end
