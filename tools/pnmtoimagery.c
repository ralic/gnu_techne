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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <pam.h>

static void dump_pixels(char *filename, int append_comma)
{
    FILE *fp;
    int i, j, k;
    struct pam input;
    tuple **tuples;

    if (filename) {
        fp = fopen(filename, "r");
    } else {
        fp = stdin;
    }

    if(fp == NULL) {
        pm_error("%s.", strerror(errno));
    }
    
    tuples = pnm_readpam(fp, &input, sizeof(struct pam));

    if(input.width != input.height) {
        pm_error("imagery data must be a square.");
    }

    if(input.maxval > 255) {
        pm_error("cannot handle more than 8 bits per channel.");
    }

    fprintf(stdout,
            "array.nuchars (%d, %d, %d, \"",
            input.height, input.width, input.depth);

    for(i = 0 ; i < input.height ; i += 1) {
        for(j = 0 ; j < input.width ; j += 1) {
            for(k = 0 ; k < input.depth ; k += 1) {
                fprintf(stdout, "\\%03d", tuples[i][j][k]);
            }
        }
    }
    
    fprintf(stdout, "\")%s\n", append_comma ? "," : "");

    if (filename) {
        fclose (fp);
    }
}

int main (int argc, char **argv) {
    int i;
    
    pnm_init(&argc, argv);

    fprintf(stdout, "local array = require \"array\"\n\n"
	    "return ");

    if (argc > 2) {
	fprintf(stdout, "{\n");
    
        for (i = 1 ; i < argc ; i += 1) {
            dump_pixels(argv[i], i == argc - 1);
        }
        
	fprintf(stdout, "}\n");
    } else {
        dump_pixels(NULL, 0);
    }

    exit (0);
}
