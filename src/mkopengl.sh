FUNCTIONS="glXCreateContextAttribsARB glDebugMessageCallbackARB glDebugMessageControlARB"

echo -e "#include \"opengl.h\"\n\n__attribute__((constructor)) static void load_opengl_entry_points()\n{" > opengl.c

for f in $FUNCTIONS; do
    echo "__$f = (PFN`echo $f | tr a-z A-Z`PROC)glXGetProcAddressARB((const GLubyte *)\"$f\");" 
done >> opengl.c

echo "}" >> opengl.c

echo -e "#ifndef _OPENGL_H_\n#define _OPENGL_H_\n\n#include <GL/gl.h>\n#include <GL/glu.h>\n#include <GL/glext.h>\n#include <GL/glx.h>\n#include <GL/glxext.h>\n" > opengl.h

for f in $FUNCTIONS; do
    echo -e "PFN`echo $f | tr a-z A-Z`PROC __$f;\n#define $f __$f\n"
done >> opengl.h

echo "#endif" >> opengl.h
