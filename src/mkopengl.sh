#!/bin/bash

GLX_FUNCTIONS="glXCreateContextAttribsARB"
GL_FUNCTIONS="glDebugMessageCallbackARB glDebugMessageControlARB glGetUniformIndices glGetActiveUniformBlockiv glUniformBlockBinding glGetActiveUniformBlockName glGetActiveUniformsiv"

#rm opengl.c gl.h glx.h

echo -e "#include \"gl.h\"\n#include \"glx.h\"\n\n__attribute__((constructor)) static void load_opengl_entry_points()\n{" > opengl.c

for f in $GL_FUNCTIONS $GLX_FUNCTIONS; do
    echo "__$f = (PFN`echo $f | tr a-z A-Z`PROC)glXGetProcAddressARB((const GLubyte *)\"$f\");" 
done >> opengl.c

echo "}" >> opengl.c

echo -e "#ifndef _LOCAL_GL_H_\n#define _LOCAL_GL_H_\n\n#include <GL/gl.h>\n#include <GL/glu.h>\n#include <GL/glext.h>\n" > gl.h

for f in $GL_FUNCTIONS; do
    echo -e "PFN`echo $f | tr a-z A-Z`PROC __$f;\n#define $f __$f\n"
done >> gl.h

echo "#endif" >> gl.h

echo -e "#ifndef _LOCAL_GLX_H_\n#define _LOCAL_GLX_H_\n\n#include <GL/glext.h>\n#include <GL/glx.h>\n#include <GL/glxext.h>\n" > glx.h

for f in $GLX_FUNCTIONS; do
    echo -e "PFN`echo $f | tr a-z A-Z`PROC __$f;\n#define $f __$f\n"
done >> glx.h

echo "#endif" >> glx.h
