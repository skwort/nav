CC = gcc
IDIR = ./include
CFLAGS = -I${IDIR} -Wall -Werror -g
SRCDIR = ./src
OBJDIR = ./build/obj

# Find all .c files in the src directory
SRCS = $(wildcard ${SRCDIR}/*.c)

# Convert .c source files to corresponding .o files in OBJDIR
OBJS = $(patsubst ${SRCDIR}/%.c, ${OBJDIR}/%.o, $(SRCS))

# Specify the dependencies for header files
DEPS = ${IDIR}/log.h

# Main target to link the object files and create the executable
navd: $(OBJS)
	$(CC) -o ./build/$@ $^ $(CFLAGS)

# Create the obj directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Pattern rule to compile .c files into .o files in the OBJDIR
${OBJDIR}/%.o: ${SRCDIR}/%.c $(DEPS) | $(OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

# Clean up object files and executable
.PHONY: clean
clean:
	rm -f ${OBJDIR}/*.o ./build/navd
