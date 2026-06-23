# ascii-poker

ASCII-style Texas Hold'em poker client and server written in C++20.

## Project layout

```
common/       Shared poker logic, protocol, packet framing
server/       TCP server, lobby, rooms, game sessions (poker::server)
client/       FTXUI terminal client (poker::client)
third_party/  Vendored deps: asio, spdlog, json, FTXUI
tests/        Catch2 unit and integration tests
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

## Run

Terminal 1 — server (default port `12345`):

```bash
./build/server/poker_server
# or custom port:
./build/server/poker_server 8080
```

Terminal 2 — client:

```bash
./build/client/poker_client
# or custom host/port:
./build/client/poker_client 127.0.0.1 8080
```

## Client UI

The client uses an interactive terminal UI (FTXUI):

| Screen | Actions |
|--------|---------|
| **Lobby** | `Up`/`Down` select room, `Refresh`, `Create`, `Join`, `Quit` |
| **Create room** | Enter name and max players, `Confirm` / `Cancel` |
| **Waiting room** | Player list, host sees `Start` (min 2 players), `Leave`, `Quit` |
| **In game** | Table with cards and pot; `Fold` / `Check` / `Call` / `Raise` on your turn |

Rooms marked `(in game)` in the lobby cannot be joined.

Red cards = hearts/diamonds. Active player is marked with `>>`.

## Client commands (legacy)

Text commands still work if passed programmatically; the UI uses buttons instead:

| Command | Example | Description |
|---------|---------|-------------|
| `list_rooms` | `list_rooms` | Show available rooms |
| `create_room` | `create_room "My Room" 4` | Create a room (2–255 players) |
| `join_room` | `join_room 1` | Join room by id |
| `leave_room` | `leave_room` | Leave current room |
| `start_game` | `start_game` | Host starts the game (min 2 players) |
| `action` | `action fold` | Player action: `fold`, `check`, `call`, `raise <amount>` |
| `quit` | `quit` | Exit client |

The room host can press **Start** when at least two players are seated. The game also auto-starts when a room reaches `max_players`.

Blinds are posted automatically each hand (SB = 10, BB = 20, starting stack = 1000). Side pots are calculated when players go all-in with different stack sizes.

If a player leaves or disconnects during a hand, they are auto-folded and the game continues when at least two players remain in the room. When only one player is left, the active game session ends and the remaining player stays in the lobby.

The room list is pushed to all connected clients when rooms are created, joined, left, or a game starts.

## Protocol

Messages are length-prefixed JSON (4-byte big-endian header + UTF-8 body).

Envelope format:

```json
{ "type": "join_room", "data": { ... } }
```

Server error codes are defined in `common/include/poker/error_codes.h`.

## Dependencies

All third-party code is vendored under `third_party/` (headers in `third_party/include/`, FTXUI in `third_party/ftxui/`) and stays in the repository.
Do not add duplicate copies under `client/` or `server/`.

## Code style

Project sources use [clang-format](https://clang.llvm.org/docs/ClangFormat.html) (see `.clang-format`). Check before pushing:

```bash
bash scripts/check-format.sh
```

To apply formatting:

```bash
git ls-files '*.cpp' '*.h' | grep -v '^third_party/' | grep -v '^tests/external/' | grep -v '/ftxui/' | xargs clang-format -i
```
