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
|   |-- Server.cpp            # socket setup and poll loop
|   |-- ClientIO.cpp          # read/write buffering and socket I/O
|   |-- CommandProcessor.cpp  # command dispatch and shared parsing
|   |-- Registration.cpp      # PASS, NICK, and USER
|   |-- Join.cpp              # JOIN and join replies
|   |-- ChannelCommands.cpp   # PART, TOPIC, KICK, and INVITE
|   |-- Mode.cpp              # channel MODE handling
|   |-- Message.cpp           # PRIVMSG and HELP
|   |-- Lookup.cpp            # client and channel lookups
|   |-- Replies.cpp           # send and broadcast helpers
|   `-- Cleanup.cpp            # client and channel cleanup
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
- Per-client read buffer for commands ending in `\r\n`, with a 512-byte IRC
  line limit.
- Per-client write buffer with `POLLOUT` polling and partial-send handling.
- Temporary `recv`/`send` errors keep the client connected for a later poll cycle.
- `SIGPIPE` is ignored so a closed client cannot terminate the server during `send`.
- Disconnect cleanup removes member, operator, and invitation state, deletes empty channels, and notifies remaining members.
- A selected reference IRC client remained connected for about one hour without
  `PING` / `PONG`; current client compatibility does not require `PING` /
  `PONG`, explicit `QUIT`, or `CAP` handling.
- Commands are parsed once into a shared representation: an uppercase command
  name, normal parameters, and an optional trailing parameter.
- Empty input is ignored safely; unknown commands receive `421`, and normal
  commands remain unavailable until `PASS`, `NICK`, and `USER` complete registration.
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

Current limitation: nickname matching is not yet case-insensitive, and a
successful post-registration nickname change is not announced to shared peers.

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

Implemented.

- Creates channels.
- Allows joining existing channels.
- Supports comma-separated channel lists.
- Makes the first member of a new channel an operator.
- Ignores a request to join a channel the client already belongs to.
- Validates channel names.
- Sends `JOIN`, then the existing topic when present, followed by `353` and `366` replies.
- Sends `471`, `473`, or `475` when a channel is full, invite-only, or has the wrong key.
- Consumes a stored invitation after a successful join.
- A key supplied when creating a channel is ignored; channel keys are set with
  `MODE +k` and are only used to enter an existing keyed channel.

### PART

Implemented.

- Supports one or more comma-separated channels.
- Supports an optional part message, including an explicitly empty message.
- Broadcasts one `PART` message for each channel left.
- Removes the client from member, operator, and invitation state, and deletes
  channels that become empty.
- Reports missing parameters, unknown channels, and attempts to leave a channel
  the client has not joined.

### PRIVMSG

Implemented for individual recipients and channels.

- Sends direct messages to a connected nickname.
- Supports trailing messages after `:`.
- Supports `PRIVMSG #channel :message` and broadcasts it to other channel members.
- Rejects an empty recipient, empty message, unknown channel, and sends to channels the client has not joined.
- Returns `401 ERR_NOSUCHNICK` for an unknown direct-message recipient.

Current limitation: multiple comma-separated recipients are not supported.

### KICK

Implemented.

- Looks up the channel.
- Looks up the target user.
- Checks whether the sender is in the channel and is an operator.
- Broadcasts the `KICK` command to channel members.
- Removes the target from member, operator, and invitation state.
- Supports a multi-word trailing reason, a default reason, and an explicitly
  empty reason.
- Reports the command-specific errors for missing parameters, unknown channels
  or users, membership, target membership, and privileges.
- Allows one channel operator to kick another operator.
- Deletes a channel when kicking its last member.


### INVITE

Implemented.

- Looks up the channel.
- Looks up the target nickname.
- Requires the inviter to be in the channel.
- Requires the inviter to be a channel operator when the channel is invite-only (`+i`).
- Rejects unknown users/channels and users who are already members with the
  appropriate IRC errors.
- Works for ordinary and invite-only channels, storing the invitation for a
  later `JOIN`.
- Sends `341 RPL_INVITING` to the inviter and an `INVITE` notification to the target.

### HELP

Project helper command.

- Shows a small manual connection sequence.
- This is not part of the mandatory subject commands, but it can help with manual testing.

## Mandatory Operator Commands

### TOPIC

Implemented.

```text
TOPIC #channel
TOPIC #channel :new topic
```

Current behavior:

- View the current topic.
- Change the topic.
- Respect mode `+t`, where only operators can change the topic.
- Send correct numeric replies for an existing topic or no topic.
- Clear a topic with `TOPIC #channel :`.

Current limitation: the broadcast for a cleared topic does not yet preserve the
explicit empty trailing `:`.

### MODE

Implemented for channel mode queries and combined changes.

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
MODE #channel +it
MODE #channel +kl password 10
MODE #channel -it+o nick
```

Current behavior:

- `MODE #channel` returns `324` with the active flags, masked key, and limit.
- Combined flags and sign changes are supported.
- `i`, `t`, `k`, `o`, and `l` consume their required parameters correctly.
- Valid changes in a mixed-validity request still apply; only actual changes
  are broadcast.
- Invalid, missing, unknown, and permission-restricted operations return IRC errors.

## Remaining Work To Match The Subject

- Make nickname and channel comparisons case-insensitive; announce nickname
  changes after registration.
- Support comma-separated `PRIVMSG` recipients if required by the reference client.
- Create a unified numeric-reply helper and standardize numeric reply formatting.
- Preserve the explicit empty trailing parameter in a cleared-topic broadcast.
- Send only one `QUIT` notification to a peer who shares multiple channels with
  the disconnecting client.
- Review `fcntl()` failures, transient `accept()` errors, portability of
  `_fds.data()`, and remaining legacy comments/debug output.
- Perform final compatibility testing with a reference IRC client and the full
  mandatory-command regression suite.

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

The implementation has been exercised with socket regression suites covering
registration, parsing boundaries, JOIN, PART, PRIVMSG, TOPIC, KICK, INVITE,
MODE, fragmented/batched input, abrupt disconnect cleanup, and multiple
clients. Valgrind checks completed for representative and six-client scenarios
with no reported memory errors or leaks.

## AI Usage

AI tools were used as a learning and development aid during this project. They assisted with:

- Explaining IRC protocol concepts, C++98 syntax, socket programming.
- Reviewing code structure and suggesting refactoring opportunities.
- Helping identify edge cases, validation paths, and possible memory-management issues.
- Suggesting test scenarios for command parsing, channel management, modes, and client disconnections.
- Assisting with documentation wording.

All code was reviewed, understood, adapted where necessary, and validated by the authors.

## Note

This README describes the implemented server behaviour and the remaining
compatibility and protocol-polish work.
