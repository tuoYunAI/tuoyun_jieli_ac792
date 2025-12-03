#!/bin/sh

FILES="lib_a-strdup.o \
        lib_a-atof.o \
        lib_a-memcpy.o \
        lib_a-memmove.o \
        lib_a-memcmp.o \
        lib_a-memset.o \
        lib_a-strcmp.o \
        lib_a-strcpy.o \
        lib_a-strlen.o \
        lib_a-strncmp.o \
        lib_a-strstr.o \
        lib_a-strchr.o \
        "

function delete_files() {
    for FILE in $FILES; do
		if [ -f $FILE ] ; then
    		rm $FILE
			echo "Delete file : $FILE"
		fi
    done
}

mkdir Temp
cd Temp
ar -x ../libc.a 

delete_files

ar rcU libc.a *.o

cp -rf libc.a ../
cd ../
rm -rf Temp
