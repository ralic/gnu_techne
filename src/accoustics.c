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

#include <AL/al.h>
#include <AL/alc.h>

#include "accoustics.h"
#include "techne.h"

@implementation Accoustics
-(void) init
{
    ALCdevice *device;
    ALCcontext *audio = NULL;
    ALenum error;
    
    [super init];
    lua_pushstring (_L, "accoustics");
    lua_setfield (_L, -2, "tag");

    /* Create an openal context. */

    alcGetError(NULL);

    device = alcOpenDevice (NULL);
    error = alcGetError(NULL);

    if (error != ALC_NO_ERROR) {
	t_print_error ("I could not open the audio device (%s).\n",
		       alcGetString(NULL, error));
    }
    
    audio = alcCreateContext (device, NULL);
    error = alcGetError(device);

    if (error != ALC_NO_ERROR) {
	t_print_error ("I could not create the audio context (%s).\n",
		       alcGetString(device, error));
    }

    alcMakeContextCurrent(audio);
    alcProcessContext(audio);

    /* Print useful debug information. */    

    t_print_message ("The audio renderer is: '%s %s'.\n",
		     alGetString(AL_RENDERER), alGetString(AL_VERSION));
}

-(int) _get_gain
{
    float g;
	
    alGetListenerf (AL_GAIN, &g);
    lua_pushnumber (_L, g);

    return 1;
}

-(int) _get_renderer
{
    lua_pushstring (_L, (const char *)alGetString(AL_RENDERER));
    lua_pushstring (_L, " ");
    lua_pushstring (_L, (const char *)alGetString(AL_VERSION));
    lua_concat (_L, 3);

    return 1;
}

-(void) _set_gain
{
    alListenerf (AL_GAIN, lua_tonumber (_L, 3));
}

-(void) _set_renderer
{
    T_WARN_READONLY;
}

@end
