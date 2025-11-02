/**
 * @file commands.h
 * @brief Command dispatching interface.
 *
 * This header provides the declaration for the command dispatching function,
 * which allows different command strings to be dispatched and handled by
 * appropriate functions.
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

/**
 * @brief Dispatches a command string to the appropriate function.
 *
 * This function takes a command string and executes the corresponding command
 * from the dispatch table. The command is executed based on the provided
 * process ID (pid) of the sending process (i.e., the shell that sent the
 * message). The `args` parameter contains the remaining arguments after the
 * command string.
 *
 * @param cmd_str The command string to be dispatched and executed.
 * @param pid The process ID of the sending process (shell).
 * @param args The remaining arguments for the command.
 */
void dispatch_command(char *cmd_str, int pid, char *args);

#endif /* COMMANDS_H_ */
