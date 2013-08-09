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

#ifndef _SEEDING_H_
#define _SEEDING_H_

#define BINS_N 32

#define SEED_BUFFER_SIZE (10000)
#define SEED_SIZE (3 * 3 * 4)
#define VEGETATION_LEVEL_BIAS 0

typedef struct {
    char *buffer;
    int patches, fill, capacity, triangles;
    double center, sum;
} seeding_Bin;

typedef struct {
    seeding_Bin bins[BINS_N];
    double density, ceiling, horizon, error, clustering;
    int triangles_n[2];
} seeding_Context;

void initialize_seeding (seeding_Context *seeding_in);
void begin_seeding (seeding_Context *seeding_in, roam_Context *context_in);
int seed_tile (int i);
void finish_seeding ();

#endif
