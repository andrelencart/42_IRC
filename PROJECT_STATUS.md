# Project Status — ft_irc

Last updated: 2026-08-19

## Current Git state

- Branch: `André'sBranch---Server`
- Latest commit: `dc1fb9a` — Add the server-console shutdown command and notify
  connected clients.
- The branch matches its remote tracking branch.
- The working tree contains the strict port parser changes in `src/main.cpp`,
  `includes/Server.hpp`, and `src/Server/CommandProcessor.cpp`; the poll-driven
  shutdown and console-EOF changes in `includes/Server.hpp` and
  `src/Server/Server.cpp`; the checked `fcntl()` and transient `accept()` error
  handling in `src/Server/Server.cpp` and `src/Server/ClientIO.cpp`; the
  RFC-style nickname validation in `src/Server/Registration.cpp`; the disabled
  per-command terminal logger in `src/Server/ClientIO.cpp`; this status update;
  the subject-compliant `README.md` and `.gitignore` development-README entry;
  and the untracked `ircserv` binary produced by verification.

## Completed work

- Channel cleanup is centralised: removing a client also removes its member,
  operator, and invitation state from every channel. Empty channels are deleted
  and remaining members receive a `QUIT` message.
- `JOIN` consumes an invitation after a successful join, and `KICK` removes all
  of the target's channel state.
- The poll loop handles readable, writable, error, hang-up, and invalid client
  file-descriptor events in one place.
- Client output is buffered. Replies are queued, `POLLOUT` is enabled only
  while output is pending, partial sends are retained, and temporary send
  errors do not disconnect a client.
- `PASS` errors are queued correctly. Empty or incorrect passwords deliberately
  leave the client connected and unauthenticated so the user can retry; normal
  commands remain blocked until the correct password is supplied. The final
  README must describe this retry policy instead of claiming a disconnection.
- `SIGPIPE` is ignored so a closed peer cannot terminate the server during
  `send()`.
- The README is currently stale and requires its final subject-compliant
  rewrite after the remaining code corrections.
- IRC lines are parsed once into a shared `Command` representation containing
  an uppercase command name, normal parameters, and an optional trailing
  parameter.
- Empty lines are ignored safely, lowercase/mixed-case commands are accepted,
  unsupported commands receive `421`, and the temporary `#` broadcast path
  has been removed.
- Registration-only commands are separated from normal commands, which remain
  blocked until `PASS`, `NICK`, and `USER` complete authentication.
- Strict `\r\n` framing is retained and the 512-byte IRC line limit prevents
  an incomplete read buffer from growing without bound.
- Existing handlers now consume the shared parser result. This also preserves
  multi-word KICK reasons and distinguishes a TOPIC query from an explicitly
  empty topic, allowing `TOPIC #channel :` to clear it.
- The parser/registration implementation was compiled and exercised through
  local TCP tests covering empty and malformed commands, command-name case,
  registration ordering, fragmented and batched commands, trailing parameters,
  and IRC line-size boundaries.
- JOIN now ignores clients who are already members, validates channel names,
  sends an existing topic before the names list, and centralizes `332`, `353`,
  and `366` replies through `_sendJoinReplies()`.
- A key supplied while creating a channel through JOIN is no longer stored;
  channel keys are set through `MODE +k` and JOIN keys are used only to enter
  an existing keyed channel.
- `PART` now supports comma-separated channels and optional part messages.
  It sends one departure message per channel, removes all channel state for
  the departing client, and deletes empty channels.
- Unknown direct-message recipients now receive `401 ERR_NOSUCHNICK` and are
  not used as delivery file descriptors.
- `KICK` validation and execution are separated into `_handleKick()` and
  `_kickFromChannel()`. The new flow returns the command-specific errors,
  permits one channel operator to kick another, preserves an explicit empty
  comment, and deletes a channel if it becomes empty.
- MODE is split into three phases: `_prepareModeRequest()` finds the channel,
  answers queries, and checks permissions; `_parseModeChanges()` interprets
  combined flags and parameters; and `_executeModeChanges()` applies valid
  operations and broadcasts the successful result once.
- MODE supports combined flags and sign changes, including `+it`, `+kl key 10`,
  and `-it+o nick`. The required `i`, `t`, `k`, `o`, and `l` modes consume their
  correct parameters, and only state changes are broadcast.
- `MODE #channel` returns `324` with the active flags, a masked key placeholder,
  and the current limit when present. Invalid, missing, unknown, and
  permission-restricted mode operations return their existing IRC errors while
  valid operations in the same mode string continue to be applied.
- `INVITE` now validates the target nickname, channel, inviter membership,
  invite-only operator permission, and existing target membership with the
  appropriate `401`, `403`, `442`, `443`, and `482` replies.
- Successful invitations are stored for JOIN, work on ordinary and invite-only
  channels, send a server-prefixed `341 RPL_INVITING` to the inviter, and send
  a correctly prefixed `INVITE` notification to the target.
- Server implementation responsibilities are now separated across
  `ClientIO.cpp`, `CommandProcessor.cpp`, `Registration.cpp`, `Join.cpp`,
  `ChannelCommands.cpp`, `Mode.cpp`, `Message.cpp`, and `Lookup.cpp` instead of
  being concentrated in `Commands.cpp` and `Parsing.cpp`.
- The Makefile now groups Server, Client, and Channel source filenames and uses
  `addprefix` to construct their paths, keeping the expanded source list
  unchanged while making it easier to maintain.
- Removed the unused `_checkDupes()`, `_userToFd()`, and legacy
  `broadcastToChannel()` implementations and declarations after confirming
  that they had no callers. Corrected the `signalhHandler` declaration to
  `signalHandler`.
- Numeric replies now pass through one `_sendNumericReply()` formatter, which
  adds the configured server prefix, the client nickname or pre-NICK `*`, and
  the final IRC `\r\n` terminator.
- The named `ERR_*` and `RPL_*` macros remain the handler-facing interface and
  now expand into arguments for `_sendNumericReply()`. Numeric codes and reply
  descriptions therefore remain centralized in `includes/Server.hpp` instead
  of being repeated in command implementations.
- The numeric macro block is grouped by purpose and line-wrapped so each reply's
  code, parameters, and description can be read together without changing its
  expansion or runtime behaviour.
- Hard-coded `451` replies and hand-built `001`, `324`, `353`, and `366`
  replies were migrated to named macros and the common formatter.
- Unrelated errors were corrected: extra JOIN/INVITE parameters and extra JOIN
  keys no longer misuse `407` or `461`, channel-mode errors use `472`, and an
  invalid `+l` value uses `696` instead of the missing-parameter error `461`.
- Channel command broadcasts now distinguish an absent trailing parameter from
  an explicitly empty one, so clearing a topic broadcasts `TOPIC #channel :`.
- Comma-separated `PRIVMSG` targets are processed independently for nicknames,
  channels, and mixed lists. Invalid or empty targets no longer block valid
  recipients, direct deliveries use their individual target, exact duplicate
  target strings receive one copy, ambiguous extra parameters are rejected,
  and the merged debug output has been removed. Case-variant references to the
  same channel may deliver separately, which RFC 2810 permits for target lists.
- Channel identity is now ASCII case-insensitive. `_channels` uses a lowercase
  internal key while each `Channel` retains the spelling used at creation, so
  `#Room`, `#room`, and `#ROOM` resolve to one channel while replies and
  broadcasts consistently use `#Room`. Nicknames intentionally remain
  case-sensitive by project decision.
- `QUIT` is accepted before and after registration with default, single-word,
  multi-word, and explicitly empty reasons. The implementation removes the
  client from all channel state, notifies each shared peer at most once, queues
  an `ERROR` closing message to the quitting client, and closes its connection
  after buffered output is flushed. Abrupt disconnect cleanup now uses the
  same unique-peer notification path.

## Verification already performed

- `make` completed successfully after the networking changes.
- Socket smoke tests passed for registration, JOIN, direct and channel
  `PRIVMSG`, TOPIC, `+t`, channel key rejection (`475`), invited `+i` joins,
  KICK, abrupt disconnect cleanup, and fragmented input.
- The output-buffering paths were exercised in normal operation. Intentional
  kernel backpressure/partial-send conditions were not forced.
- `make` completed successfully with `-std=c++98 -Wall -Wextra -Werror`; an
  immediate repeated `make` correctly reported that nothing needed rebuilding.
- Three local socket suites ran 92 scripted checks in total. Their raw results
  were 31/39, 17/28, and 15/25 passing. One first-suite failure was caused by a
  test nickname exceeding the server's nine-character limit and passed when
  rerun with a valid nickname.
- All checks for the new parser/registration work passed: safe empty input,
  mixed/lowercase commands, `421`, `451`, one-time registration, fragmented
  input, multiple commands in one packet, full trailing text, strict `\r\n`,
  the 512-byte boundary, and oversized-buffer disconnection.
- Successful command checks included direct/channel `PRIVMSG`, topic set/query,
  `+t`/`-t`, multi-word KICK reasons, `+k`/key rejection (`475`), `+l`/full
  rejection (`471`), `+i`/invite-only rejection (`473`), invited join, and
  `+o`/`-o` privilege changes.
- Earlier checks confirmed that `TOPIC #channel :` clears the stored topic and
  a later query returns `331`. Those checks exposed the omitted trailing `:`;
  the broadcast fix has been implemented but not yet retested.
- An earlier suite identified case-sensitive channel matching, repeated QUIT
  notices for peers sharing multiple channels, and missing explicit `QUIT` as
  unresolved. The channel and QUIT implementations have since been added;
  focused QUIT verification is still pending. `PING` and `CAP` remain absent.
- A two-client Valgrind scenario covered registration, JOIN, channel PRIVMSG,
  TOPIC, peer disconnect, and server shutdown. Valgrind reported 0 errors, 0
  bytes in use at exit, 103 allocations matched by 103 frees, and only the
  three standard file descriptors open at exit.
- The JOIN changes compiled successfully and passed all 23 focused socket
  checks. Coverage included new/existing channels, repeated JOIN, topic reply
  order, channel-name validation and length boundaries, key creation rules,
  correct/wrong existing keys, channel limits, invite-only channels, and
  invited joins. Two initial assertions incorrectly expected a leading space
  before unprefixed `332`; the raw replies were correct for current formatting
  and both corrected assertions passed.
- The `PART` change compiled successfully with a forced rebuild and passed
  focused two-client TCP checks: multi-channel departure, supplied/default/
  empty part messages, `461`, `403`, `442`, and empty-channel deletion.
- The PRIVMSG and KICK changes compiled successfully with a forced rebuild.
  Focused two-client TCP checks confirmed `401` for an unknown direct-message
  nickname and successful direct delivery to an existing nickname. KICK
  coverage included `461`, `403`, `401`, `442`, `441`, and `482`; kicking an
  operator; supplied, default, and explicitly empty comments; delivery to the
  kicked client; and deletion of an empty channel.
- MODE compiled successfully with `-std=c++98 -Wall -Wextra -Werror` and passed
  51 focused TCP checks. Coverage included queries, combined flags, sign
  changes, parameter assignment, key masking, permission errors, no-op
  suppression, unknown flags between valid flags, missing parameters, invalid
  limits, invalid operator targets, partial success after errors, and MODE
  broadcasts.
- A three-client MODE scenario under Valgrind covered queries, combined changes,
  invalid flags and parameters, operator changes, and disconnect cleanup.
  Valgrind reported 0 errors, 0 bytes in use at exit, 199 allocations matched by
  199 frees, and only the three standard file descriptors open at exit.
- INVITE compiled successfully and passed all 15 focused three-client TCP
  checks. Coverage included missing parameters, unknown channels and
  nicknames, inviter membership, existing target membership, ordinary-channel
  invitations, invite-only operator permission, `341` formatting, target
  notifications, invitation storage, and successful invited JOINs.
- The reorganized source completed a clean rebuild with `-std=c++98 -Wall
  -Wextra -Werror`; a repeated `make` correctly reported that everything was
  up to date. `git diff --check` passed, all moved `Server` member definitions
  were checked for unique placement, and the removed functions were confirmed
  unused.
- A fresh six-client socket regression suite passed all 65 checks both normally
  and under Valgrind. Coverage included registration and rejection paths,
  lowercase commands, fragmented and batched input, JOIN, PART, KICK, INVITE,
  TOPIC, direct/channel PRIVMSG, abrupt disconnect cleanup, and successful and
  failing MODE operations for all required `i`, `t`, `k`, `o`, and `l` flags.
- Valgrind 3.22 reported 0 errors, 0 bytes in use at exit, and 524 allocations
  matched by 524 frees. File-descriptor tracking showed only the three standard
  descriptors plus Valgrind's own inherited log-file descriptor.
- A selected reference IRC client previously remained connected for about one
  hour without `PING` / `PONG`; no `PING` / `PONG` or `CAP` handling was needed
  in that earlier compatibility check.
- The numeric-reply changes compiled successfully with `-std=c++98 -Wall
  -Wextra -Werror`. A focused local socket suite passed all 13 checks: the
  pre-NICK `451` target, `001`, `421`, `411`, `353`, `366`, `331`, `472`,
  `696`, and `401` formatting; plus explicit empty TOPIC delivery to both the
  setter and a channel peer. The test server shut down cleanly afterward.
- A read-only consistency search found no remaining raw numeric construction,
  old `_sendMsg(fd, ERR_...)` usage, or obsolete broadcast-helper call
  signatures.
- The merged comma-separated `PRIVMSG` implementation compiled successfully
  before correction. A consumer-focused local suite kept the server alive but
  passed only 5 of 13 behavioral assertions. Confirmed failures covered the
  combined target in delivered messages, incorrect `401` parameters, early
  abortion after invalid targets, empty list entries, mixed channel/nickname
  targets, and silent truncation when `:` was omitted from a multi-word message.
- The corrected implementation compiled successfully with `-std=c++98 -Wall
  -Wextra -Werror`, and `git diff --check` passed. The complete consumer-focused
  `PRIVMSG` matrix passed all 24 cases with no skips. Coverage included normal,
  duplicate, mixed, invalid, forbidden, and empty targets; missing and repeated
  commas; missing `:`; empty and missing messages; spaces and tabs; fragmented
  and batched commands; exact 512-byte framing; oversized-client disconnection;
  removal of merged debug output; and continued server operation afterward.
- The ASCII case-insensitive channel-key change compiled successfully with the
  Makefile's C++98 warning/error flags. A four-client TCP matrix exercised 30
  cases across registration, mixed-case JOIN/PRIVMSG/TOPIC/MODE/INVITE/KICK/
  PART, original-name preservation, case-sensitive nickname lookup, duplicate
  and malformed input, fragmented and batched commands, and post-error server
  stability. Twenty-nine initial assertions passed. The remaining assertion
  observed two deliveries for `PRIVMSG #Room,#room`; RFC 2810 explicitly
  permits list dispatch without duplicate-path suppression, so this behavior
  was accepted and left unchanged. The server exited normally with status zero.
- The final consumer-side audit completed a clean build and exercised 149
  executable assertions. It passed 143 and exposed six discrepancies: numeric
  port suffixes are accepted, empty/incorrect PASS attempts remain connected,
  a one-word TOPIC without `:` is treated as a query, nickname changes are not
  announced to peers, and closed console input causes a busy poll loop.
- The two PASS observations are now accepted as the intended retry policy, not
  code failures. Strict port parsing was implemented and verified separately,
  leaving the TOPIC, nickname-notification, and console-EOF discrepancies to
  correct.
- The corrected direct-message check, every required `+/- i,t,k,o,l` path,
  channel key/limit/invite combinations, operator KICK, invitation/operator fd
  reuse cleanup, twelve simultaneous clients, and a 3000-message slow-reader
  stress case all passed.
- A final Valgrind lifecycle reported 0 errors, 0 bytes in use at exit, and 150
  allocations matched by 150 frees. A real IRC-client rerun was skipped because
  HexChat, Irssi, and WeeChat were unavailable; the `nc` consumer check passed.
- The subject audit found that the direct destructor `send()` occurred outside
  `poll()` readiness, which is explicitly prohibited. The implementation has
  now been changed to queue shutdown messages and flush them after `POLLOUT`.
- The poll-driven shutdown change compiled successfully with the Makefile's
  C++98 warning/error flags, and a repeated `make` performed no relinking. All
  six focused shutdown scenarios passed: no clients, an unregistered client,
  two registered clients with pending HELP output, a slow reader with 8000
  queued channel messages, SIGINT, and a two-client Valgrind lifecycle.
- Every tested client received `ERROR :Closing Link: Server Shutdown.` followed
  by TCP EOF, and the server exited with status 0. The pending HELP output was
  delivered before the closing ERROR. In the slow-reader case, the server
  waited for writable readiness, flushed 3,536,076 bytes including the final
  ERROR, then closed normally.
- The shutdown Valgrind run reported 0 errors, 0 bytes in use at exit, and 107
  allocations matched by 107 frees. Descriptor tracking reported only
  Valgrind's own inherited log descriptor beyond standard input/output/error.
- A static call-path check confirmed that the only network `send()` remaining
  is in `_flushClientOutput()`, which is called from the client event handler
  only when `poll()` reports `POLLOUT`.
- Console polling now disables the stdin entry by setting its descriptor to
  `-1` after EOF, `POLLHUP`, `POLLERR`, or `POLLNVAL`. A negative descriptor is
  ignored by the same `poll()` call, so the server can continue waiting for the
  listener and clients without repeatedly waking on a permanently closed stdin.
  The helper processes readable console bytes before disabling a combined
  `POLLIN | POLLHUP` event, so a final `shutdown` line is not discarded.
- The console-EOF change compiled successfully and passed all ten focused
  consumer-side cases. Exact and mixed-case `shutdown` commands exited with
  status 0, and a final `shutdown\n` immediately followed by EOF was processed
  before the console descriptor was disabled. Missing-newline, extra-parameter,
  and deliberately fragmented inputs did not cause an accidental shutdown or
  crash; each server remained available until its SIGINT cleanup.
- With stdin attached to `/dev/null`, the server used 0 CPU ticks during a
  0.5-second sample, continued accepting a client, and completed registration
  with numeric `001`. SIGINT then delivered
  `ERROR :Closing Link: Server Shutdown.`, TCP EOF, and server exit status 0.
  Valgrind reported 0 errors and 0 bytes in use at exit for the same closed-stdin
  lifecycle.
- The first registration assertion incorrectly used `consolecheck`, which is
  longer than the server's nine-character nickname limit; the server correctly
  returned numeric `432`. The corrected `eofcheck` case received `001` and
  passed, so this was a test-input error rather than a server failure.
- Port parsing no longer uses `atoi()`. The shared parsing source now validates
  every character, accepts only the explicit TCP port range 1 through 65535,
  and rejects partially numeric and overflowing arguments without integer
  wraparound.
- The strict port matrix passed all 14 cases. Valid inputs `6667`, `06667`, and
  `65535` listened on the expected ports and shut down with status 0. Empty,
  zero, signed, alphabetic, whitespace-contaminated, suffixed, over-65535, and
  extremely large values were rejected with status 1. The updated code built
  with the required flags, and a repeated `make` performed no relinking.
- The listening and accepted-client `fcntl(F_SETFL, O_NONBLOCK)` results are now
  checked. A listening-socket failure aborts startup; an accepted-client failure
  closes only that new descriptor and leaves the server available. `accept()`
  now returns to `poll()` for `EINTR`, `EAGAIN`, `EWOULDBLOCK`, and
  `ECONNABORTED`, while unexpected listener errors remain fatal.
- The socket-error change compiled successfully and passed all nine focused
  cases: normal registration and poll-driven shutdown; fifty immediate
  connect/disconnect attempts followed by successful registration; injected
  `EAGAIN`, `EINTR`, and `ECONNABORTED` accept failures; an injected fatal
  `EBADF` accept failure; listener and accepted-client `fcntl()` failures; and a
  closed-stdin Valgrind lifecycle. The accepted-client failure produced EOF for
  only that client, and the following client received `001`. Valgrind reported
  0 errors, 0 bytes in use at exit, and normal exit status 0.
- The poll-array argument now uses `&_fds[0]` instead of the C++11-only
  `std::vector::data()`. The vector is guaranteed to contain the listener and
  console entries before the loop reaches `poll()`, so the C++98 expression is
  valid. The change compiled successfully; a client registered with numeric
  `001`, then the console `shutdown` command delivered the closing `ERROR`, TCP
  EOF, and server exit status 0. Both runtime assertions passed.
- The authenticated per-command terminal logger is commented out while its two
  lines remain beside the command-processing path for quick local debugging.
  A focused test confirmed that a channel key and private-message contents no
  longer appear in the server terminal output.
- A fresh final audit cross-referenced the complete implementation with the
  local subject and exercised 172 behavioral assertions. Before the nickname
  correction, 171 passed and `NICK #bad` was the only failure: it was accepted
  instead of receiving `432 ERR_ERRONEUSNICKNAME`. The two deliberately skipped
  post-merge checks were recorded separately and were not counted as failures.
- The fresh lifecycle matrix passed all 11 checks, including twelve simultaneous
  clients, clean shutdown with zero/unregistered/registered clients, queued HELP
  output ordering, mixed-case and malformed console input, SIGINT, closed stdin
  with zero sampled CPU ticks, and a slow reader receiving 2,360,039 buffered
  bytes followed by the shutdown error and TCP EOF.
- A real `nc` consumer registered, joined a channel, sent a message, issued
  QUIT, received its closing error, and exited normally. A fresh three-client
  Valgrind lifecycle covered JOIN, MODE, INVITE, PRIVMSG, TOPIC, QUIT, abrupt
  disconnect, and shutdown. It reported 0 errors, 0 bytes in use at exit, 206
  allocations matched by 206 frees, and only the three standard descriptors
  open at exit.
- The required clean build cycle passed: `make fclean`, clean `make`, repeated
  `make`, `make clean`, rebuild, and `make re`. `git diff --check` also passed.
  An optional `-pedantic-errors` syntax check still reports the legacy extra
  namespace-scope semicolons in `Client.cpp` and `Channel.cpp`; the Makefile's
  required C++98 warning/error flags compile successfully.
- Nickname validation now uses an explicit IRC allowlist instead of a blacklist:
  the first character must be an ASCII letter or IRC special character, later
  characters may additionally be digits or `-`, and the existing nine-character
  maximum remains. This rejects channel prefixes and other punctuation as
  nicknames, avoids non-standard `isascii()` and unsafe ctype calls, and keeps
  the project's explicitly chosen case-sensitive nickname lookup policy.
- Post-fix verification passed all 45 checks. The focused nickname matrix passed
  36/36 valid, invalid, empty, overlength, non-ASCII, retry, duplicate, and
  case-policy cases. The mandatory-command regression passed 8/8 checks across
  registration, parsing, JOIN, PRIVMSG, TOPIC, MODE, INVITE, KICK, PART, QUIT,
  shutdown, and terminal-output privacy. A final Valgrind lifecycle passed with
  0 errors, 0 bytes in use at exit, 137 allocations matched by 137 frees, and
  only the three standard file descriptors open at exit.
- The tracked `README.md` is now subject-compliant and contains the mandatory
  first line plus Description, Instructions, and Resources sections. It records
  verified build, registration, command, shutdown, and policy behavior, provides
  correct client and netcat usage, cites the protocol and networking resources,
  and includes a concise, truthful AI-use disclosure. The previous detailed
  README is preserved locally as ignored `README_DEVELOPMENT.md`.

## Remaining delivery work

The non-blocking output work is complete. Work through these unresolved items
one at a time, removing or refining entries here as each is completed.

1. **Reference-client compatibility**
   - Keep nicknames case-sensitive as explicitly chosen for this project.
   - Channel identity is ASCII case-insensitive; special RFC punctuation case
     mapping remains optional unless the reference client requires it.
   - Confirm the corrected comma-separated `PRIVMSG` behavior with the selected
     reference client during final compatibility testing.
2. **Subject and portability cleanup**
    - Remove remaining legacy comments and JOIN debug output.
    - Store the USER real name if desired.
    - Consider RFC 1459 case mapping for `[]\\` and `{ }|`.
    - Remove the extra namespace-scope semicolons exposed by a strict pedantic
      C++98 syntax check.
3. **Final delivery verification**
    - Run the complete build/clean cycle under the required C++98 flags.
    - Test partial and multiple commands in a packet, simultaneous clients,
      abrupt disconnects, and fd reuse.
    - Exercise every mandatory command and mode in success and failure cases.
    - Connect using the selected reference IRC client and check for leaks or
      crashes.

## Recommended next task

Recheck the deliberately deferred TOPIC and nickname-broadcast behavior after
merging the teammates' work. Then run reference-client compatibility when
HexChat, Irssi, or WeeChat becomes available and perform the final pre-delivery
Git review.

## Post-merge recheck backlog

These items were deliberately skipped because other team members are currently
working on the related areas. Keep them visible and recheck them after merging:

- Support a one-word topic without a trailing parameter marker, for example
  `TOPIC #channel oneword`, while preserving normal query and `:` behavior.
- Broadcast successful post-registration nickname changes once to the changing
  client and each shared peer, while retaining the chosen case-sensitive
  nickname policy.

### Closed console input flow

Before this change:

```text
poll() reports stdin POLLHUP
→ the console helper ignores it because there is no POLLIN
→ poll() immediately reports the same POLLHUP
→ the server repeats continuously and consumes CPU
```

After this change:

```text
poll() reports stdin EOF, POLLHUP, POLLERR, or POLLNVAL
→ the console pollfd is set to -1
→ the same poll() ignores that entry
→ the server waits normally for listener and client events
```

No additional `poll()` is introduced, stdin is not explicitly closed, and
interactive console behavior is unchanged while stdin remains available.

### Poll-driven server shutdown flow

```text
Iteration 1:
poll() reports stdin POLLIN
→ read "shutdown"
→ g_stop = 1

Iteration 2:
queue shutdown messages
→ enable POLLOUT for clients
→ call poll() again

poll() reports client POLLOUT
→ _flushClientOutput()
→ actual send()
→ close client after writing

When all clients are closed:
→ loop finishes
→ destructor closes the listening socket
```

## NOTES and COMMENTS

### INVITE flow

For an `INVITE bob #chat` request:

1. Require both the target nickname and channel name.
2. Confirm `#chat` exists; otherwise send `403 ERR_NOSUCHCHANNEL`.
3. Confirm `bob` exists; otherwise send `401 ERR_NOSUCHNICK`.
4. Confirm the inviter belongs to `#chat`; otherwise send
   `442 ERR_NOTONCHANNEL`.
5. If `#chat` is invite-only (`+i`), confirm the inviter is a channel
   operator; otherwise send `482 ERR_CHANOPRIVSNEEDED`.
6. Confirm `bob` is not already a member; otherwise send
   `443 ERR_USERONCHANNEL`.
7. Store Bob's file descriptor in the channel's invitation set.
8. Send `341 RPL_INVITING` to the inviter.
9. Send Bob the actual IRC notification:

```text
:alice!user@localhost INVITE bob :#chat
```

### MODE flow

MODE is separated into preparation, syntax parsing, and execution so that each
step has one responsibility:

```text
_handleMode()
    ↓ receives the generic Command produced by _parseCommand()
_prepareModeRequest()
    ↓ finds the channel, answers queries, or validates a change request
_parseModeChanges()
    ↓ converts a combined mode string into individual ModeChange operations
_executeModeChanges()
    ↓ calls _applyMode() for each operation and builds one final broadcast
_applyMode()
    ↓ delegates +k/-k, +o/-o, and +l/-l to their existing helpers
```

For example, this client command:

```text
MODE #chat +kl-o secret 10 bob
```

first reaches `_handleMode()` as the already tokenized command:

```text
name   = "MODE"
params = ["#chat", "+kl-o", "secret", "10", "bob"]
```

#### 1. `_handleMode()` coordinates the work

`_handleMode()` does not parse individual flags or change channel state
directly. It calls the three MODE phases in order and stops when preparation
or parsing cannot produce executable work.

#### 2. `_prepareModeRequest()` finds and validates the target

This phase first requires a channel name and looks up the channel. A missing
name returns `461 ERR_NEEDMOREPARAMS`; an unknown channel returns
`403 ERR_NOSUCHCHANNEL`.

`MODE #chat` is a query rather than a change. The function replies with
`324 RPL_CHANNELMODEIS`, listing active `i`, `t`, `k`, and `l` flags. The key
is represented by `*`, rather than exposing the password, and the limit is
included when one is set.

For a change request, such as `MODE #chat +i`, the sender must belong to the
channel (`442 ERR_NOTONCHANNEL`) and be a channel operator
(`482 ERR_CHANOPRIVSNEEDED`). Only then does the request proceed to parsing.

#### 3. `_parseModeChanges()` interprets flags and parameters

The parser walks the mode string from left to right. `+` and `-` update the
current sign; every following mode letter becomes one `ModeChange` entry.
For the example above it produces:

```text
+k secret
+l 10
-o bob
```

It associates parameters with the modes that need them:

- `i` and `t` never need a parameter.
- `+k` needs a key; `-k` does not.
- `+l` needs a positive limit; `-l` does not.
- `+o` and `-o` need a target nickname.

Unknown flags return `472 ERR_UNKNOWNMODE`, and missing required parameters
return `461 ERR_NEEDMOREPARAMS`. Valid flags elsewhere in the same string are
still retained, allowing requests such as `+ixt` to apply `+i` and `+t` while
reporting `x` as unknown.

#### 4. `_executeModeChanges()` applies valid operations once

Each parsed entry is converted to a single mode string, such as `"+k"` or
`"-o"`, then sent to `_applyMode()`. It collects only operations that actually
changed channel state and sends one MODE message to the channel members.

For example, when all operations succeed, the earlier request produces:

```text
:alice!user@localhost MODE #chat +kl-o secret 10 bob
```

If one operation fails—for example, an invalid `+l` value—the valid operations
before or after it still run. The failed operation is omitted from the final
broadcast.

#### 5. `_applyMode()` enforces each individual rule

`_applyMode()` changes `i` and `t` directly after checking whether the state is
already set. It delegates the parameterized rules to the existing helpers:

- `_applyKeyMode()` sets or removes the key. A second `+k` while a key exists
  returns `467 ERR_KEYSET`.
- `_applyOperatorMode()` checks that the target nickname exists and belongs to
  the channel before adding or removing operator status.
- `_applyLimitMode()` accepts only positive numeric limits for `+l` and clears
  the limit for `-l`.

Reapplying an already active mode, removing an inactive mode, or assigning an
operator state the target already has does not generate a broadcast.

### KICK flow

`KICK` is split into two functions so each has one job:

```text
_handleKick()
    ↓ parse channel, target, and optional comment
    ↓ find the channel and retain its map iterator
_kickFromChannel()
    ↓ validate membership, operator status, and target
    ↓ broadcast KICK and remove target channel state
_handleKick()
    ↓ delete the channel when it became empty
```

This allows `_kickFromChannel()` to work with a `Channel` reference while
`_handleKick()` retains the map iterator required to erase an empty channel
safely. A missing comment defaults to the kicker nickname. The KICK line is
built directly so `KICK #channel nick :` preserves its explicitly empty
trailing comment.

### Shared command parser and registration changes

The server now parses every IRC command once and gives all handlers the same
structured result.

#### How it works now

```text
Socket data
    ↓
Client read buffer
    ↓
Complete line ending in \r\n
    ↓
512-byte limit check
    ↓
_parseCommand()
    ↓
Registration check
    ↓
Command dispatcher
    ↓
JOIN / PRIVMSG / KICK / TOPIC / MODE / ...
```

#### 1. Added the `Command` structure

In `includes/Server.hpp`:

```cpp
struct Command {
    std::string name;
    std::vector<std::string> params;
    bool hasTrailing;
    std::string trailing;
};
```

For example:

```text
privmsg #general :Hello everyone
```

becomes:

```cpp
name = "PRIVMSG"
params = ["#general"]
hasTrailing = true
trailing = "Hello everyone"
```

This is better because handlers no longer need to parse the same raw line
independently.

#### 2. Added `_parseCommand()`

In `src/Server/CommandProcessor.cpp`, the parser:

- Safely ignores empty lines.
- Removes extra spaces between parameters.
- Converts only the command name to uppercase.
- Separates normal parameters.
- Preserves everything following `:` as one trailing parameter.
- Records whether `:` was actually present.

Because the command name is normalized:

```text
join
Join
JOIN
```

all become:

```text
JOIN
```

Messages, passwords, nicknames, and topics are not converted.

#### 3. Centralized command dispatch

Previously, `_processCommand()` extracted only the command and first parameter.
Every handler then parsed the raw line again.

Now it follows this flow:

```cpp
_parseCommand(line, command);
_checkRegistration(fd, command);
_dispatchCommand(fd, command);
```

The dispatcher searches for the uppercase command name and calls the
appropriate handler.

Unsupported commands now receive:

```text
421 <command> :Unknown command
```

The temporary code that accessed `command[0]` was removed, so empty input can
no longer cause unsafe indexing there.

#### 4. Improved registration enforcement

Before, the server mainly checked whether `PASS` succeeded.

Now:

- `PASS` must succeed first.
- `NICK` and `USER` are accepted during registration.
- Normal commands are blocked until all three registration parts are complete.
- Unregistered clients receive `451`.
- Once `PASS`, `NICK`, and `USER` are valid, the client becomes authenticated.

For example, this is rejected:

```text
PASS password
JOIN #room
```

The client must first send:

```text
PASS password
NICK john
USER john 0 * :John Smith
```

#### 5. Converted existing handlers

`PASS`, `NICK`, `USER`, `JOIN`, `PRIVMSG`, `KICK`, `INVITE`, `TOPIC`, and
`MODE` now consume the parsed `Command`.

That immediately improves trailing text:

```text
PRIVMSG bob :Hello there Bob
```

The complete message remains:

```text
Hello there Bob
```

Likewise:

```text
KICK #room bob :Breaking the channel rules
```

preserves the entire reason.

#### 6. Fixed empty-topic distinction

Previously, both of these could behave like a query:

```text
TOPIC #room
TOPIC #room :
```

Now:

- No trailing parameter means "show the topic."
- A present but empty trailing parameter means "clear the topic."

That is why `hasTrailing` is necessary.

#### 7. Added input-size protection

In `src/Server/ClientIO.cpp`, the server now enforces the IRC maximum of 512
bytes, including `\r\n`.

This prevents a client from continuously sending data without terminating a
command and growing its read buffer indefinitely.

Strict IRC `\r\n` line endings are retained for now.

Overall, the server is safer and more consistent, and later fixes to `JOIN`,
`PRIVMSG`, `KICK`, `INVITE`, and `MODE` can use parsed parameters directly. The
parser and registration changes compiled successfully and passed their focused
runtime verification on 2026-08-13.

## Starting a new Codex session

Use this prompt:

> Read `AGENTS.md` and `PROJECT_STATUS.md`, inspect the current Git state, and
> continue with the next unfinished ft_irc item. Explain the planned change and
> wait for my approval before editing or testing.
