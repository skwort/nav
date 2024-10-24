# nav
`nav` is a navigation tool for Linux shell environments designed to speed up
directory navigation. The ultimate goal of nav is to be a `cd` on steroids; a
drop-in replacement for `cd` with additional powerful features to improve
efficiency in shell environments.

I've been using a basic `bash` script for this functionality for a few years,
but am now rewriting it from scratch to expand its capabilities and improve its
performance.

## Usage

### 1. Start `navd`
Run the `navd` daemon in a terminal:

```bash
./navd
```

### 2. Interact with `navd` from a separate shell
To begin interacting with `navd`, follow these steps:

#### Open the socket:
You need to create a socket connection to communicate with `navd`. You can do
this by using `socat`:

```bash
exec 9<> >(socat - UNIX-SENDTO:/home/$USER/.nav/navd.sock,bind=/home/$USER/.nav/$$.sock)
```

This creates a socket bound to the `navd` daemon.

#### Register a shell:
Once the socket is open, you can register the current shell session with `navd`
by sending the following message:

```bash
echo "$$ register" >&9
```

This sends the shell's PID to `navd` and registers the shell.

#### Unregister a shell:
To unregister the current shell session from `navd`, you can send the following
message:

```bash
echo "$$ unregister" >&9
```

This sends a message to `navd` to remove the shell from its session tracking.

## Current Status
As of now, the core functionality of `navd` (the navigation daemon) is still
under development, and the Bash wrapper that interfaces with the daemon has not
yet been written. Currently, only the following features are implemented:
- **Shell registration**: `navd` registers shell sessions when a new shell
  interacts with it.
- **Shell unregistration**: Shells can be unregistered from `navd`.

## Planned Usage
Once fully implemented, `nav` will provide an intuitive way to navigate the
filesystem using directory tags and a stack-based undo system.

### Example commands (not yet implemented):
- `nav add foo ~/bar`: Tag the directory `~/bar` with the label `foo`.
- `nav foo`: Navigate to the directory tagged `foo`.
- `nav back`: Return to the previous directory (undo).

## Implementation
`nav` consists of two core components:
- **Bash wrapper**: The user-facing interface that interacts with the shell.
  This wrapper will be responsible for sending commands from the shell to the
  `navd` daemon. _(This is not yet implemented.)_
- **Navigation daemon (navd)**: A state management daemon that handles
  directory mappings, shell sessions, and tracks navigation actions. Currently,
  `navd` can register and unregister shell sessions.

## Flow
1. **Daemon Startup**: 
   - The `navd` daemon starts when the user logs in and creates a Unix Domain
     Socket (UDS) at `~/.nav/navd.sock`.

2. **Shell Session**: 
   - When a new shell session starts, the user’s first `nav` command interacts
     with `navd`, registering the shell and granting it a session. The session
     will manage an action stack for that shell, allowing for individual shell
     instances to have separate navigation histories. _(Currently, only
     registration and unregistration are implemented.)_

3. **Navigation Commands**: 
   - Future commands will allow users to change directories via tags and manage
     their navigation history. The `dir map` will persist across sessions via a
     config file, allowing directory tags to remain intact even after a user
     logs out.

## Future Development
- **Directory Tagging**: Allow users to tag directories with labels for quick
  access.
- **Action Stack**: Track navigation actions, allowing users to undo commands.
- **Bash Wrapper**: Implement the user-facing Bash script to interact with the
  daemon.

## navd API
The navigation daemon (`navd`) and the `nav` Bash wrapper will communicate via
a text-based API once the full functionality is implemented. The current API
allows the registration and unregistration of shell sessions, as follows:
```
PID register
PID unregister 
```