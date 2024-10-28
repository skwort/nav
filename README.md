# nav
`nav` is a navigation tool for Linux shell environments designed to speed up
directory navigation in bash. 

## About
`nav` is a powerful upgrade of a navigation bash script I created a few years
ago, designed to make directory navigation faster and more efficient. The goal
is simple: fewer keystrokes to get from point A to point B. While the original
script served its purpose, it lacked statefulness, making it impossible to
return to a previous directory easily. With `nav`, you can now seamlessly
navigate back and forth between directories.

DEMO - ascii cinema.

### Architecture
`nav` consists of three main components:
- **Daemon:** A per-user background process that maintains navigation state on a
  per-shell basis.
- **Client:** Used to interact with the daemon.
- **Bash Wrapper:** Wraps the client to provide a user-friendly command-line
  interface.

The client and daemon communicate using Unix Domain Socket (UDS) datagrams for
efficient interaction. Initially, I considered using `socat` for communication,
but its overhead and inflexibility led me to implement a custom client,
significantly speeding up setup and teardown, while adding more flexibility.

Message routing is achieved using a named socket system. The server socket is
lives at `rootdir/nav.sock`, while client sockets live under `rootdir/pid.sock`,
where `pid` is the PID off the shell. Note that `rootdir` can be user defined,
but defaults to `/home/$USER/.nav/`. See the client and daemon `README`s for
more details.

Most of the daemon state is stored in dynamically allocated linked lists.
There's a list of shells, tags, and actions, with the latter existing on a
per-shell basis. Actions represent previous navigation commands.

## Usage
From an end-user perspective, you should only ever need to interact with the
bash interface. The usage for bash is as follows:
```
Usage: nav [command] <arguments>

Commands:
  [tag]             Navigate to the specified tag.
  add [tag] [path]  Add a new tag-path association.
  delete [tag]      Remove the specified tag.
  show              Show all tag-path associations.
  back|b              Undo the previous action.
  actions           List all recorded actions.
```
Each successful navigation, i.e. `nav [tag]` will push the current working
directory to the action stack for that shell. `nav back` will *undo* the
navigation.

Tab-complete for tags and directories is supported.

Details of the client and daemon interfaces are given in individual `README`s
under their respective `src` directories.

## Installation
First I'd suggest installing the daemon as a system unit. A basic unit file
might look like:
```
[Unit]
Description=Nav Daemon
After=default.target

[Service]
ExecStart=/path/to/nav/build/daemon
Restart=on-failure

[Install]
WantedBy=default.target
```

To install it, you can do the following:
```bash
# Link the unit so systemd can see it
ln -sf /path/to/navd/unit /home/$USER/.config/systemd/user/navd.service

# Reload and install
systemctl --user daemon-reload
systemctl --user enable navd.service
systemctl --user start navd.service

# Check the daemon is running
systemctl --user status navd
```

I suggest checking the journal output to ensure everything works as expected:
```bash
# Dump the log
journalctl --user -u navd.service

# Follow logs in real-time
journalctl --user -u navd.service -f
```

Once installed, you need to configure your `.bashrc` to install the `nav`
function in your shell. Add the following lines:
```bash
NAV_CLIENT=/path/to/nav/build/client
source /path/to/nav/scripts/nav.sh
```

## Testing
Integration testing is conducted using `pytest`. To run tests you need to:
```bash
# Make and activate a virtual environment
python -m venv .venv
source .venv/bin/activate

# Install the dependencies
pip install -r requirements.txt

# Run tests
pytest
```

Tests use a custom, pseudo-random root directory, so they shouldn't interfere
with your system installation.
