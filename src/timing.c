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

#include <time.h>
#include "techne.h"

#ifdef __WIN32__
#include <windows.h>
#include <winbase.h>

LARGE_INTEGER frequency;
#endif

void t_print_timing_resolution()
{
#ifdef __WIN32__
    BOOL available;

    available = QueryPerformanceFrequency (&frequency);

    if(!available) {
	t_print_warning ("No performance counter available.\n");
    } else {
	t_print_message ("The frequency of the realitme clock is "
			 "%ld Hz.\nNo process time clock available, "
			 "using the real-time clock instead.\n",
			 frequency.QuadPart);
    }	
#else
    struct timespec time;
    
    clock_getres(CLOCK_REALTIME, &time);
    t_print_message ("The resolution of the realitme clock is %ld ns.\n", time.tv_nsec);
    clock_getres(CLOCK_PROCESS_CPUTIME_ID, &time);
    t_print_message ("The resolution of the process clock is %ld ns.\n", time.tv_nsec);
#endif
}

long long int t_get_real_time ()
{
#ifdef __WIN32__
    LARGE_INTEGER ticks;
    
    QueryPerformanceCounter(&ticks);

    return ticks.QuadPart * 1000000000ll / frequency.QuadPart;
#else
    struct timespec time;

    clock_gettime (CLOCK_REALTIME, &time);

    return (long long int)time.tv_sec * 1000000000 + time.tv_nsec;
#endif
}

long long int t_get_cpu_time ()
{
#ifdef __WIN32__
    return t_get_real_time();
#else
    struct timespec time;

    clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &time);

    return (long long int)time.tv_sec * 1000000000 + time.tv_nsec;
#endif
}
