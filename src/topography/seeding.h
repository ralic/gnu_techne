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

#define SEED_SIZE (3 * 3 * 4)
#define TRANSFORMED_SEED_SIZE ((4 + 4 * 3 + 2) * sizeof(float) +        \
                               2 * sizeof(unsigned int))

typedef struct {
    char *buffer;
    int fill, capacity, clusters, triangles;
    double center;
} seeding_Bin;

typedef struct {
    seeding_Bin bins[BINS_N];
    double density, ceiling, horizon, error, clustering;
    int level, triangles_n[2];
} seeding_Context;

void initialize_seeding (seeding_Context *seeding_in);
void begin_seeding (seeding_Context *seeding_in, roam_Context *context_in);
void seed_tile (int i);
void finish_seeding ();

#endif
