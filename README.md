*This project has been created as part of the 42 curriculum by andcarva, .*

# ft_irc

IRC server written in C++98 for the 42 `ft_irc` project.

The goal of this project is to build an IRC server capable of accepting multiple TCP clients at the same time, authenticating users with a password, managing channels, and implementing the mandatory commands required by the subject.

Useful reference:

```text
https://datatracker.ietf.org/doc/html/rfc1459
```

## Build

```bash
make
```

The generated binary is:

```bash
./ircserv
```

To clean:

```bash
make clean
make fclean
make re
```

## Usage

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 pass
```

You can then connect with an IRC client such as `xchat`, `irssi`, `nc`, or another client available on the 42 computers.

Basic sequence expected by the current server:

```text
PASS pass
NICK andre
USER andre
JOIN #test
PRIVMSG other :message
```

## Structure

```text
includes/
|-- Server.hpp
|-- Client.hpp
`-- Channel.hpp

src/
|-- main.cpp
|-- Server/
|   |-- Server.cpp      # socket, accept, poll loop, read buffer
|   |-- Commands.cpp    # commands and dispatcher
|   |-- Parsing.cpp     # JOIN/channel parsing
|   |-- Replies.cpp     # send and broadcast helpers
|   |-- Cleanup.cpp     # client removal  
|-- Client/
|   `-- Client.cpp
`-- Channel/
    `-- Channel.cpp
```

## Current State

### Server Base

- IPv4 TCP socket using `socket`, `bind`, `listen`, and `accept`.
- Non-blocking sockets using `fcntl`.
- Client multiplexing using `poll`.
- Per-client buffer for commands ending in `\r\n`.
- Shutdown through `SIGINT`.
- C++98 compilation with `-Wall -Wextra -Werror`.

### Client

`Client` stores:

- file descriptor
- nickname
- username
- password state
- authentication state
- read buffer
- write buffer, prepared but not heavily used yet

### Channels

`Channel` already has state for:

- channel name
- password/key
- topic
- user limit
- invite-only mode
- members
- operators
- invited users

Not all of this state is connected to complete commands yet.

## Commands Already Started

### PASS

Partially implemented.

- Validates the received password.
- Removes the client if the password is empty or wrong.

### NICK

Partially implemented.

- Stores the nickname.
- Checks for an empty nickname.
- Checks for duplicate nicknames.
- Performs some character validation.

### USER

Partially implemented.

- Stores the username.
- Prevents repeating `USER` after it has already been set.

Placeholder:

- Adapt parsing to the normal IRC client format:

```text
USER <username> <hostname> <servername> :<realname>
```

### JOIN

Partially implemented.

- Creates channels.
- Allows joining existing channels.
- Supports comma-separated channel lists.
- Supports password/key when creating a channel.
- Sends `JOIN`, `353`, and `366` replies.

Placeholders:

- Automatically make the first client in a channel an operator.
- Send the complete member list in `353`.
- Send correct IRC errors for full channels, invite-only channels, and wrong keys.
- Avoid inconsistent states when a client is already in the channel.

### PRIVMSG

Partially implemented.

- Sends a direct message to a nickname.

Placeholders:

- Support the full message after `:`.
- Support `PRIVMSG #channel :message`.
- Broadcast to channel members.
- Do not send the message back to the sender for channel messages.
- Send correct errors: no recipient, no text, no such nick, no such channel, client not in channel.

### KICK

Partially implemented.

- Looks up the channel.
- Looks up the target user.
- Checks whether the sender is in the channel and is an operator.
- Removes the member from the channel.

Placeholders:

- Fix the IRC `KICK` broadcast/reply format.
- Fix the loop that sends the message to channel members.
- Review permission rules for kicking operators.
- Send correct errors: no such channel, sender not in channel, target not in channel, missing privileges.

### INVITE

Partially implemented.

- Looks up the channel.
- Looks up the target user.
- Adds the user to the invited list.

Placeholders:

- Fix `isInvited` verification.
- Validate whether the sender must be an operator.
- Validate whether the invited user is already in the channel.
- Send IRC invite replies to the sender and invited user.
- Send correct errors.

### HELP

Project helper command.

- Shows a small manual connection sequence.
- This is not part of the mandatory subject commands, but it can help with manual testing.

## Mandatory Commands Still To Address

### TOPIC

Mandatory subject placeholder.

Should support:

```text
TOPIC #channel
TOPIC #channel :new topic
```

Expected behavior:

- View the current topic.
- Change the topic.
- Respect mode `+t`, where only operators can change the topic.
- Send correct numeric replies for an existing topic or no topic.

### MODE

Mandatory subject placeholder.

The subject requires channel modes:

```text
i - invite-only
t - restrict TOPIC changes to operators
k - channel password/key
o - give/remove operator privileges
l - user limit
```

Expected examples:

```text
MODE #channel +i
MODE #channel -i
MODE #channel +t
MODE #channel +k password
MODE #channel -k
MODE #channel +o nick
MODE #channel -o nick
MODE #channel +l 10
MODE #channel -l
```

## Features Still To Do To Match The Subject

- Implement `TOPIC`.
- Implement `MODE` with `i`, `t`, `k`, `o`, `l`.
- Ensure the first client in a channel becomes an operator.
- Improve `PRIVMSG` for channels and messages using `:`.
- Fix `KICK` and `INVITE` formatting and IRC errors.
- Implement complete cleanup when a client disconnects:
  - remove from all channels
  - remove from operators
  - remove from invited users
  - delete empty channels
  - notify remaining members
- Add basic `PING/PONG` support for compatibility with real clients.
- Reply with `421 ERR_UNKNOWNCOMMAND` for unknown commands.
- Improve general IRC command parsing.
- Ensure all messages end with `\r\n`.
- Handle partial `send` or send errors.
- Ignore `SIGPIPE` to avoid crashes when a client closes the connection.
- Review all numeric errors so they are closer to the RFC/subject.

## Suggested Manual Tests

With `nc`:

```bash
nc 127.0.0.1 6667
```

Then send:

```text
PASS pass
NICK user1
USER user1
JOIN #test
```

With two clients:

```text
PASS pass
NICK user2
USER user2
JOIN #test
PRIVMSG user1 :hello
```

Important tests still pending:

- Two clients in the same channel.
- Private message between clients.
- Message to a channel.
- Channel creation with password.
- Join with wrong password.
- Kick a user.
- Invite to an invite-only channel after `MODE +i`.
- Topic with and without mode `+t`.

## Note

This README describes the current state of the code and the placeholders still needed to satisfy the subject. Some structures already exist in the code, but still need to be connected to the final commands.
