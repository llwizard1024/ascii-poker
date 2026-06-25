# ascii-poker

Multiplayer Texas Hold'em poker with an ASCII terminal UI. Native C++20 client and TCP server — no web stack, no external runtime beyond a C++ toolchain.

Cards, table, and controls render in the terminal via [FTXUI](https://github.com/ArthurSonzogni/FTXUI). The server is authoritative for all game logic; the client displays state and sends player actions.

**Languages:** English and Russian (toggle with `L` in the client).

## Quick start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

Or use the helper script (clean rebuild):

```bash
./build.sh              # build everything
./build.sh poker_client # build one target
```

### Run

Terminal 1 — server (default port `12345`):

```bash
./build/server/poker_server
./build/server/poker_server 8080   # custom port
```

Terminal 2 — client:

```bash
./build/client/poker_client
./build/client/poker_client 127.0.0.1 8080   # custom host/port
```

CLI arguments override defaults. The client also reads and writes `~/.ascii-poker.conf` (player name, host, port, language).

## Client

### Screens

| Screen | What you see | Main actions |
|--------|----------------|--------------|
| **Login** | Name, host, port fields | `Join`, `Lang`, `Quit` |
| **Lobby** | Room list with player counts | `Refresh`, `Create`, `Join`, `Leave`, `Quit` |
| **Create room** | Room name and max players | `Confirm`, `Cancel` |
| **Waiting room** | Seated players, host marker | Host: `Start` (≥2 players), `Leave`, `Quit` |
| **In game** | Table, cards, pot, action bar | `Fold` / `Check` / `Call` / `Raise`, presets, `Leave` |
| **Disconnected** | Connection lost message | `Reconnect`, `Quit` |

Rooms marked **(in game)** cannot be joined. After disconnect, **Reconnect** restores the TCP session and re-authenticates with your saved name.

### Hotkeys

Hotkeys work when no text field is focused.

| Context | Keys | Action |
|---------|------|--------|
| Lobby | `j` / `k` or `↑` / `↓` | Select room |
| Lobby | `▲` / `▼` buttons | Same as above |
| In game (your turn) | `f` | Fold |
| In game (your turn) | `c` | Check or Call |
| In game (your turn) | `r` | Focus raise amount |
| Any screen | `[` / `]` | Scroll event log |
| Any screen | `L` | Toggle English ↔ Russian |
| Login | `Enter` | Join (same as `Join` button) |

On your turn, raise presets are available: **Min**, **½ pot**, **Pot**, **All-in**. The Call button shows the amount when a call is required (e.g. `Call 20`).

### Table display

- Red suits: hearts ♥ and diamonds ♦
- Active player: `>>` prefix
- Dealer / blinds: `[D]`, `[SB]`, `[BB]`
- Your seat: `(you)` suffix
- Folded / all-in: `[fold]`, `[all-in]`
- Preflop hand hint (e.g. pocket pair, suited connectors) and made-hand hint after the board deals
- Showdown banner shows winners, pot, and winning hand category

### Settings file

`~/.ascii-poker.conf`:

```ini
player_name=Alice
host=127.0.0.1
port=12345
language=en
```

Use `language=ru` for Russian. Settings are saved after a successful login and when the language is changed.

## Game rules (server)

| Parameter | Value |
|-----------|-------|
| Starting stack | 1000 chips |
| Small blind | 10 |
| Big blind | 20 |

- Blinds are posted automatically at the start of each hand; the dealer button rotates.
- Betting rounds follow standard Hold'em order; side pots are computed for multi-way all-ins.
- The host can **Start** with at least two players; the game also auto-starts when the room is full.
- A player who leaves or disconnects mid-hand is auto-folded. The session continues if at least two players remain in the room; when only one is left, the hand is resolved and the session ends.
- The room list is broadcast to all connected clients when rooms are created, joined, left, or a game starts.

## Protocol

TCP, async I/O (Asio). Messages are length-prefixed JSON: 4-byte big-endian size + UTF-8 body.

```json
{ "type": "join_room", "data": { ... } }
```

Message types and structs: `common/include/poker/protocol.h`  
Error codes: `common/include/poker/error_codes.h`

## Project layout

```
common/       Shared poker logic, protocol, packet framing (poker_core)
server/       TCP server, lobby, rooms, game sessions (poker::server)
client/       FTXUI terminal client (poker::client)
third_party/  Vendored: asio, spdlog, nlohmann/json, FTXUI, Catch2
tests/        Unit and integration tests
scripts/      check-format.sh
```

Build outputs:

- `build/server/poker_server`
- `build/client/poker_client`
- `build/tests/poker_tests`

## Development

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

Check formatting:

```bash
bash scripts/check-format.sh
```

Apply clang-format:

```bash
git ls-files '*.cpp' '*.h' | grep -v '^third_party/' | grep -v '^tests/external/' | grep -v '/ftxui/' | xargs clang-format -i
```

CI (GitHub Actions) configures, builds, and runs the test suite on each push.

## Dependencies

All third-party code is vendored under `third_party/` and committed to the repository. Do not add duplicate copies under `client/` or `server/`.
