#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
    FILE *f;
    char *name, *s, *t;
    int c;

    if (argc < 2) {
        return 1;
    }
    
    name = calloc(1, strlen(argv[1]));
    for (s = argv[1], t = name ; *s && *s != '.' ; s += 1,  t += 1) {
        if (*s == '.' || *s == '/') {
            *t = '_';
        } else if (isalnum(*s) || *s == '_') {
            *t = *s;
        }
    }
    
    f = fopen(argv[1], "r");

    if (!f) {
        return 1;
    }
    
    printf("const char *%s = \"", name);
    while (1) {
        c = fgetc(f);

        if (feof(f)) {
            break;
        }

	switch (c) {
	case '\\':
	    putchar ('\\');
	    putchar ('\\');
	    break;

	case '"':
	    putchar ('\\');
	    putchar ('\"');
	    break;
	    
	case '\n':
	    putchar ('\\');
	    putchar ('n');
	    break;
	    
	default:
	    putchar (c);
	}
    }
    printf("\";\n");
    fclose(f);
    
    return 0;
}
