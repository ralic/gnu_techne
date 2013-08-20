/* Copyright (C) 2013 Papavasileiou Dimitris                             
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
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <png.h>

int main(int argc, char **argv)
{
   png_structp png;
   png_infop info;
   png_bytep *rows;
   int width, height, depth, i, j, k, raw = 0;
   FILE *fp;

   opterr = 0;
     
   while ((k = getopt (argc, argv, "r")) != -1)
       switch (k)
       {
       case 'r':
           raw = 1;
           break;
       case '?':
           if (isprint (optopt)) {
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
           } else {
               fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
           }
           
           return 1;
       default:
           abort ();
       }

   if(argv[optind]) {
       fp = fopen(argv[optind], "rb");
   } else {
       fp = stdin;
   }
   
   if (fp == NULL) {
       fprintf(stderr, "%s: %s: %s.", argv[0], argv[1], strerror(errno));
       return 1;
   }

   png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

   if (png == NULL) {
       fprintf(stderr,
	       "%s: %s: could not create read struct.",
	       argv[0], argv[1]);
       fclose(fp);
       
       return 2;
   }

   info = png_create_info_struct(png);
   
   if (info == NULL) {
       png_destroy_read_struct(&png, NULL, NULL);
       fprintf(stderr,
	       "%s: %s: could not create info struct.",
	       argv[0], argv[1]);
       fclose(fp);

       return 3;
   }

   if (setjmp(png_jmpbuf(png))) {
       png_destroy_read_struct(&png, &info, NULL);
       fprintf(stderr,
	       "%s: %s: something went wrong while reading the png file.",
	       argv[0], argv[1]);
       fclose(fp);

       return 4;
   }

   png_init_io(png, fp);
   png_set_sig_bytes(png, 0);
   png_read_png(png, info,
		PNG_TRANSFORM_PACKING | PNG_TRANSFORM_STRIP_16,
		NULL);

   /* At this point we have read the entire image */

   width = png_get_image_width(png, info);
   height = png_get_image_height(png, info);
   depth = png_get_channels(png, info);
   rows = png_get_rows(png, info);

   if (!raw) {
       fputs("local array = require \"array\"\n\n"
             "return ", stdout);
   }
   
   fprintf(stdout, "array.nuchars (%d, %d, %d, \"", width, height, depth);

   for(i = height - 1 ; i >= 0 ; i -= 1) {
       for(j = 0 ; j < width ; j += 1) {
	   for(k = 0 ; k < depth ; k += 1) {
	       fprintf(stdout, "\\%03d", rows[i][j * depth + k]);
	   }
       }
   }

   fputs("\")", stdout);

   if (!raw) {
       fputc('\n', stdout);
   }
   
   /* Clean up. */
   
   png_destroy_read_struct(&png, &info, NULL);
   fclose(fp);
   
   return 0;
}
