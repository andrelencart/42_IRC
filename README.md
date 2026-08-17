*This project has been created as part of the 42 curriculum by andcarva, rmota-ma, dicosta-.*

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
- Client read, write, hangup, and socket-error handling through `poll`.
- Per-client read buffer for commands ending in `\r\n`.
- Per-client write buffer with `POLLOUT` polling and partial-send handling.
- Temporary `recv`/`send` errors keep the client connected for a later poll cycle.
- `SIGPIPE` is ignored so a closed client cannot terminate the server during `send`.
- Disconnect cleanup removes member, operator, and invitation state, deletes empty channels, and notifies remaining members.
- Shutdown through `SIGINT`.
- Makefile configured with `-std=c++98 -Wall -Wextra -Werror`.

### Client

`Client` stores:

- file descriptor
- nickname
- username
- password state
- authentication state
- read buffer
- write buffer for queued outgoing messages
- close-after-write state for replies that must be delivered before disconnecting

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

## Command Status

### PASS

Implemented for the subject registration requirements.

- Validates the received password.
- Disconnects the client after delivering the error reply if the password is empty or wrong.
- Rejects repeated `PASS` after the password has already been accepted.

Current behavior:

- `PASS` with no parameter queues `461` and disconnects after the reply is sent.
- `PASS` with a wrong password queues `464` and disconnects after the reply is sent.
- `PASS` with the correct password marks the client password state as accepted.
- `PASS` after a successful password sends `462` and keeps the client connected.

### NICK

Implemented for the subject registration flow.

- Stores the nickname.
- Checks for an empty nickname.
- Checks for duplicate nicknames while ignoring the current client fd.
- Performs some character validation.
- Validates invalid characters from the first character onward.

### USER

Implemented for the subject registration flow.

- Stores the username.
- Prevents repeating `USER` after it has already been set.
- Parses the normal IRC client format:

```text
Syntax: user/USER <username> 0 * <realname>
```

- Sends `461 USER :Not enough parameters` when the command does not have enough parameters.
- Sends `461 USER :Not enough parameters` when the second and third parameters are not `0` and `*`.

Future improvement:

- Store and/or parse the full realname when it contains spaces after `:`.

### JOIN

Partially implemented.

- Creates channels.
- Allows joining existing channels.
- Supports comma-separated channel lists.
- Supports password/key when creating a channel.
- Makes the first member of a new channel an operator.
- Sends `JOIN`, `353`, and `366` replies.
- Sends `471`, `473`, or `475` when a channel is full, invite-only, or has the wrong key.
- Consumes a stored invitation after a successful join.

Known limitations:

- Avoid inconsistent states when a client is already in the channel.
- Review empty-key and malformed channel-list parsing.

### PRIVMSG

Partially implemented.

- Sends direct messages to a connected nickname.
- Supports trailing messages after `:`.
- Supports `PRIVMSG #channel :message` and broadcasts it to other channel members.
- Rejects an empty recipient, empty message, unknown channel, and sends to channels the client has not joined.

Known limitations:

- An unknown nickname does not yet return the correct IRC error or stop delivery cleanly.
- Multiple comma-separated recipients are not supported.

### KICK

Partially implemented.

- Looks up the channel.
- Looks up the target user.
- Checks whether the sender is in the channel and is an operator.
- Broadcasts the `KICK` command to channel members.
- Removes the target from member, operator, and invitation state.

Known limitations:

- Standard trailing kick reasons are parsed incorrectly.
- Review permission rules for kicking operators.
- Send correct errors: no such channel, sender not in channel, target not in channel, missing privileges.

### INVITE

Partially implemented.

- Looks up the channel.
- Looks up the target user.
- Adds the user to the invited list when the channel is invite-only or keyed.

Known limitations:

- Validate whether the sender must be an operator.
- Validate whether the invited user is already in the channel.
- Send IRC invite replies to the sender and invited user.
- Allow invitations to ordinary channels.
- Send correct errors.

### HELP

Project helper command.

- Shows a small manual connection sequence.
- This is not part of the mandatory subject commands, but it can help with manual testing.

## Mandatory Operator Commands

### TOPIC

Implemented with current limitations.

Should support:

```text
TOPIC #channel
TOPIC #channel :new topic
```

Current behavior:

- View the current topic.
- Change the topic.
- Respect mode `+t`, where only operators can change the topic.
- Send correct numeric replies for an existing topic or no topic.

Limitation: `TOPIC #channel :` is treated as a topic query instead of clearing the topic.

### MODE

Implemented for one mode change per command.

The subject requires channel modes:

```text
i - invite-only
t - restrict TOPIC changes to operators
k - channel password/key
o - give/remove operator privileges
l - user limit
```

Supported examples:

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

Known limitations:

- Combined mode strings such as `MODE #channel +it` are validated but do not apply their changes.
- `MODE #channel` does not return the current channel modes.
- Some mode-specific IRC error cases still need review.

## Remaining Work To Match The Subject

- Fix C++98 compatibility in the polling loop.
- Complete JOIN error handling and safe parsing.
- Correct direct-message errors and delivery to unknown nicknames.
- Complete `KICK` and `INVITE` parsing, permissions, notifications, and IRC errors.
- Support combined MODE strings and mode queries.
- Allow clearing a topic with an empty trailing parameter.
- Add basic `PING/PONG` support for compatibility with real clients.
- Reply with `421 ERR_UNKNOWNCOMMAND` for unknown commands.
- Improve general IRC command parsing.
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
USER user1 0 * :<name>
JOIN #test
```

With two clients:

```text
PASS pass
NICK user2
USER user2  0 * :<name>
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

## AI Usage

AI tools were used as a learning and development aid during this project. They assisted with:

- Explaining IRC protocol concepts, C++98 syntax, socket programming.
- Reviewing code structure and suggesting refactoring opportunities.
- Helping identify edge cases, validation paths, and possible memory-management issues.
- Suggesting test scenarios for command parsing, channel management, modes, and client disconnections.
- Assisting with documentation wording.

All code was reviewed, understood, adapted where necessary, and validated by the authors.

## Note

This README describes the current state of the code and the placeholders still needed to satisfy the subject. Some structures already exist in the code, but still need to be connected to the final commands.
