#
# "main" pseudo-component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)



COMPONENT_ADD_INCLUDEDIRS := ../include
COMPONENT_SRCDIRS := ../main
#
## just to remove make compiling warning
#src/stack_check.o: <:=
#
## disable stack protection in files which are involved in initialization of that feature
#src/stack_check.o: CFLAGS := $(filter-out -fstack-protector%, $(CFLAGS))
