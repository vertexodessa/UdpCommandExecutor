# UDP Command Executor

This project implements a simple UDP server that listens on a specified port (default 7755), receives incoming commands, and executes them in the shell. The commands must be sent in the format:

<TIMESTAMP><DELIMITER><COMMAND><END_OF_COMMAND_MARKER>

By default, these are:
- Delimiter: `::`
- End-of-command marker: `#END#`
- Log file path: `/tmp/command_executor.log`

It ignores any command whose timestamp is not strictly greater than the last processed timestamp.

---

## Features

1. **UDP Listener**  
   The server listens on all network interfaces (`INADDR_ANY`) for UDP packets on the configured port.
2. **Command Execution**  
   Commands are received in the specified format, then executed via the shell (`popen()`).
3. **Monotonically Increasing Timestamps**  
   If a command's timestamp is not greater than the most recent processed timestamp, it is ignored.
4. **Logging**  
   Each command and its output is appended to `/tmp/command_executor.log` (configurable).

---

## Build Instructions

### Requirements

- A C++ compiler (e.g., g++ or clang++) with support for modern C++ (C++11 or later).
- Headers for networking (arpa/inet.h, sys/socket.h, netinet/in.h).
- A POSIX-like environment (Linux, macOS, etc.).

### Compilation

Use g++ (as an example):

g++ -std=c++11 -o udp_command_executor main.cpp

Replace "main.cpp" with the file containing the refactored code if needed. This will produce an executable called "udp_command_executor".

### Installation (Optional)

You can place "udp_command_executor" in your PATH, for example:

sudo cp udp_command_executor /usr/local/bin/

---

## Usage

### Basic Execution

Run without arguments:

./udp_command_executor

This will bind to port 7755 and start listening for UDP commands with the default delimiter `::` and end-marker `#END#`. Logs are stored in /tmp/command_executor.log.

### Custom Port

Supply a custom port as the first command-line argument:

./udp_command_executor 8888

The server now listens on port 8888.

### Sending Commands

Use netcat (nc) to send commands to the server. For example:

echo -n "1675000000::ls -la#END#" | nc -4u -w1 127.0.0.1 7755

Where:
- 1675000000 is a sample timestamp (a long long).
- :: is the delimiter.
- ls -la is the command.
- #END# marks the end of the command.

If you send another command with the same or a smaller timestamp, it will be ignored.

### Broadcast Packets

If your network and firewall allow broadcast packets, you can send to a broadcast address, for instance:

echo -n "1675000001::uname -a#END#" | nc -4u -b 255.255.255.255 7755

The server will receive it if it's listening on 0.0.0.0:7755 and if the system permits broadcast.

---

## Log File

Commands and outputs are logged to /tmp/command_executor.log by default. An example of a log entry:

=====
Timestamp: 1675000000
Command: ls -la
Output:
total 20
drwxr-xr-x  5 user user 4096 Jan 18 00:00 .
drwxr-xr-x 40 user user 4096 Jan 18 00:00 ..
...etc...
=====

---

## Security Considerations

- **Danger of Arbitrary Code Execution**  
  Any user on the network (who can send packets to the port) can run arbitrary commands. Restrict access or run in a sandboxed environment.
- **Firewall and Permissions**  
  Ensure the port is firewalled as needed. Run with non-root privileges if possible.
- **Validation**  
  There's no sanitization of the command string. If needed, restrict the set of allowed commands or parse carefully.

---

## Troubleshooting

1. **Bind Errors**  
   "Cannot bind socket" might mean the port is in use, or you need privileges for ports < 1024.
2. **No Packets Received**  
   Check your firewall rules and confirm the port is open. Verify you are sending to the correct IP/port.
3. **No Log Output**  
   Check write permissions for /tmp/command_executor.log, and watch for error messages on stderr.

---

## Contributing

1. Fork this repository.
2. Create a feature branch.
3. Commit and push your changes.
4. Open a pull request explaining your modifications.

---
