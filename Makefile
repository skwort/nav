CC = gcc
IDIR = ./include
CFLAGS = -I${IDIR} -Wall -Werror -g
SRCDIR = ./src
OBJDIR = ./build/obj

# Directories for daemon, client, and shared sources
DAEMON_SRCDIR = ${SRCDIR}/daemon
SHARED_SRCDIR = ${SRCDIR}/shared

# Object directories for each target
DAEMON_OBJDIR = ${OBJDIR}/daemon
SHARED_OBJDIR = ${OBJDIR}/shared

# Ensure the object directories exist
$(OBJDIR) $(DAEMON_OBJDIR) $(SHARED_OBJDIR):
	mkdir -p $@

# Find all .c files in daemon and shared directories
DAEMON_SRCS = $(wildcard ${DAEMON_SRCDIR}/*.c) $(wildcard ${SHARED_SRCDIR}/*.c)

# Convert .c source files to corresponding .o files in the appropriate OBJDIR
DAEMON_OBJS = $(patsubst ${SRCDIR}/%.c, ${OBJDIR}/%.o, $(DAEMON_SRCS))

# Main target
daemon: $(DAEMON_OBJS) | $(OBJDIR) $(DAEMON_OBJDIR) $(SHARED_OBJDIR)
	$(CC) -o ./build/$@ $^ $(CFLAGS)

# Pattern rule to compile .c files into .o files in the appropriate OBJDIR
${OBJDIR}/%.o: ${SRCDIR}/%.c | $(OBJDIR) $(DAEMON_OBJDIR) $(SHARED_OBJDIR)
	$(CC) -c -o $@ $< $(CFLAGS)

# Clean up object files and executables
.PHONY: clean
clean:
	rm -rf ${OBJDIR} ./build/daemon
