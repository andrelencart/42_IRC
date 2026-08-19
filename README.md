*This project has been created as part of the 42 curriculum by andcarva, rmota-ma, dicosta-.*

# ft_irc

## Description

ft_irc is an Internet Relay Chat server written in C++98. It accepts multiple
TCP clients simultaneously and provides the essential IRC features required by
the 42 project: connection authentication, user registration, private messages,
channels, channel operators, and channel modes.

The server uses one `poll()` call to coordinate the listening socket, connected
clients, console input, and buffered output. All network sockets are
non-blocking. Incoming data is stored per client until a complete `\r\n`
terminated IRC command is available, and outgoing data is queued until
`poll()` reports that the corresponding socket is writable.

### Supported commands

| Command | Purpose |
|---|---|
| `PASS` | Authenticate using the server password |
| `NICK` | Set a nickname |
| `USER` | Complete user registration |
| `JOIN` | Create or join one or more channels |
| `PART` | Leave one or more channels |
| `PRIVMSG` | Send messages to users or channels |
| `TOPIC` | View, set, or clear a channel topic |
| `INVITE` | Invite a user to a channel |
| `KICK` | Remove a user from a channel |
| `MODE` | View or modify the required channel modes |
| `QUIT` | Disconnect with an optional reason |
| `HELP` | Display the server's short command guide |

The required channel modes are:

| Mode | Effect |
|---|---|
| `i` | Make the channel invite-only |
| `t` | Restrict topic changes to channel operators |
| `k` | Set or remove the channel key |
| `o` | Give or remove channel-operator privileges |
| `l` | Set or remove the channel user limit |

Channel names are compared case-insensitively using ASCII lowercase keys while
preserving the spelling used when the channel was created. Nicknames are
case-sensitive by project decision and are validated using the IRC nickname
character set with a maximum length of nine characters.

The first member of a new channel becomes its operator. When a client leaves,
is kicked, quits, or disconnects unexpectedly, the server removes its channel
membership, operator status, and invitations. Empty channels are deleted.

The server supports two clean shutdown paths: `Ctrl+C` and the local console
command `shutdown`. Connected clients receive
`ERROR :Closing Link: Server Shutdown.`, pending output is flushed through
`POLLOUT`, their sockets are closed, and the server exits normally.

### Project structure

```text
includes/
|-- Channel.hpp
|-- Client.hpp
`-- Server.hpp

src/
|-- main.cpp
|-- Channel/
|   `-- Channel.cpp
|-- Client/
|   `-- Client.cpp
`-- Server/
    |-- Server.cpp
    |-- ClientIO.cpp
    |-- CommandProcessor.cpp
    |-- Registration.cpp
    |-- Join.cpp
    |-- ChannelCommands.cpp
    |-- Mode.cpp
    |-- Message.cpp
    |-- Lookup.cpp
    |-- Replies.cpp
    `-- Cleanup.cpp
```

## Instructions

### Requirements

- A Unix-like operating system
- A C++ compiler supporting C++98
- GNU Make
- An available TCP port from `1` to `65535`

No third-party library is required.

### Compilation

Build the server with:

```bash
make
```

The Makefile compiles with:

```text
-std=c++98 -Wall -Wextra -Werror
```

Available cleanup rules:

```bash
make clean   # remove object files
make fclean  # remove object files and ircserv
make re      # rebuild everything
```

### Starting the server

Run the executable with a port and a password without whitespace:

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 secret
```

Invalid, signed, partially numeric, zero, and out-of-range ports are rejected.
The server password must not be empty or contain whitespace.

### Connecting with an IRC client

Configure a client with the following values:

```text
Server:   127.0.0.1
Port:     6667
Password: secret
TLS/SSL:  disabled
```

The server expects the standard registration sequence:

```text
PASS secret
NICK alice
USER alice 0 * :Alice Example
```

An empty or incorrect `PASS` receives an error but remains connected, allowing
the user to retry. Normal IRC commands remain blocked until `PASS`, `NICK`, and
`USER` have all completed successfully.

### Connecting with netcat

Open a second terminal and connect using a netcat implementation that supports
CRLF conversion:

```bash
nc -C 127.0.0.1 6667
```

Register and join a channel:

```text
PASS secret
NICK alice
USER alice 0 * :Alice Example
JOIN #chat
```

Connect a second client as `bob`, join the same channel, and try:

```text
PRIVMSG alice :Hello Alice
PRIVMSG #chat :Hello channel
TOPIC #chat :Project discussion
MODE #chat +it
INVITE alice #chat
KICK #chat alice :Example reason
PART #chat :Leaving
QUIT :Goodbye
```

A key for a newly created channel is configured with `MODE +k`. After setting
one, other clients provide it as the second JOIN argument:

```text
MODE #private +k channelpass
JOIN #private channelpass
```

Combined mode changes are supported. Parameters are supplied in the order in
which the corresponding mode letters appear:

```text
MODE #chat +it
MODE #chat +kl channelpass 10
MODE #chat -it+o alice
```

Use a trailing parameter when a topic, message, or reason contains spaces. An
explicitly empty trailing parameter clears a topic:

```text
TOPIC #chat :A topic with spaces
TOPIC #chat :
```

### Stopping the server

In the terminal running `ircserv`, either press `Ctrl+C` or enter:

```text
shutdown
```

The shutdown command is local to the server console; it is not an IRC command
available to remote clients.

## Resources

The following references were used to understand IRC syntax, client-server
behavior, channel management, socket programming, and event-driven I/O:

- [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)
- [RFC 2810 — Internet Relay Chat: Architecture](https://www.rfc-editor.org/rfc/rfc2810)
- [RFC 2811 — Internet Relay Chat: Channel Management](https://www.rfc-editor.org/rfc/rfc2811)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)

- Linux manual pages for
  [`socket(2)`](https://man7.org/linux/man-pages/man2/socket.2.html),
  [`poll(2)`](https://man7.org/linux/man-pages/man2/poll.2.html),
  [`recv(2)`](https://man7.org/linux/man-pages/man2/recv.2.html), and
  [`send(2)`](https://man7.org/linux/man-pages/man2/send.2.html)

### Use of AI

AI tools were used as a supplementary learning and review aid. Their use was
limited mainly to clarifying selected IRC, C++98, and socket-programming
concepts; suggesting edge cases for the command parser, client cleanup, and
shutdown flow; and improving documentation wording.

AI suggestions were treated only as proposals. The authors directed the work,
reviewed and adapted every change, understood the resulting implementation, and
performed the final compilation, protocol, stress, and leak verification.
