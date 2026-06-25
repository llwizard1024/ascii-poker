# ascii-poker

Multiplayer Texas Hold'em poker with an ASCII terminal UI. Native C++20 client and authoritative TCP server — no web stack, no external runtime beyond a C++ toolchain.

Cards, table, and controls render in the terminal via [FTXUI](https://github.com/ArthurSonzogni/FTXUI). The server owns all game logic; the client displays state and sends player actions.

**Languages:** English and Russian (toggle with `L` in the client).

**Русская документация:** [README.ru.md](README.ru.md)

## Features

- Account login with username and password; new players receive a starting stack automatically
- Chip balances stored in SQLite and synced at hand boundaries (after blinds and after each pot award)
- Lobby with create/join/leave, waiting room, and in-table play
- Side pots, all-in handling, standard Hold'em betting rounds
- English / Russian UI with persisted language preference
- Unit and integration tests (Catch2)

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

**System packages** (Ubuntu/Debian): `libsqlite3-dev`, `libssl-dev`

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

CLI arguments override defaults. The client reads and writes `~/.ascii-poker.conf` (username, host, port, language — password is never stored).

## Accounts and database

The server persists player accounts in **SQLite**. Passwords are hashed with **PBKDF2-SHA256** (OpenSSL).

| Setting | Default | Override |
|---------|---------|----------|
| Database file | `ascii-poker.db` next to the `poker_server` binary | `ASCII_POKER_DB=/path/to/file.db` |

Schema migrations run automatically on server start (current schema version: **2**).

### Login

Clients authenticate with a `login` message (username + password). On first login, an account is created with **1000 chips**. The server responds with `welcome` and the current balance.

**Username:** 1–32 characters, no control characters.

**Password:** 4–128 characters, at least one letter and one digit, no control characters.

### Chip balance

- Stacks are loaded from the database when a game starts.
- Balances are written back after blinds are posted and after each hand is resolved (including when a player leaves mid-hand).
- You need at least **20 chips** (big blind) to create or join a room.
- A balance of **0** means bankruptcy: you can still log in and stay in the lobby, but you cannot join a table until you have enough chips again.

## Client

### Screens

| Screen | What you see | Main actions |
|--------|----------------|--------------|
| **Login** | Username, password, host, port | `Join`, `Lang`, `Quit` |
| **Lobby** | Room list with player counts | `Refresh`, `Create`, `Join`, `Leave`, `Quit` |
| **Create room** | Room name and max players | `Confirm`, `Cancel` |
| **Waiting room** | Seated players, host marker | Host: `Start` (≥2 players), `Leave`, `Quit` |
| **In game** | Table, cards, pot, action bar | `Fold` / `Check` / `Call` / `Raise`, presets, `Leave` |
| **Disconnected** | Connection lost message | `Reconnect`, `Quit` |

Rooms marked **(in game)** cannot be joined. **Reconnect** opens a new TCP session and logs in again if the password is still in the login field.

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

On your turn, raise presets: **Min**, **½ pot**, **Pot**, **All-in**. The Call button shows the amount when required (e.g. `Call 20`).

### Table display

- Dark slate panel background; muted focus highlights
- Red suits (♥ ♦): coral; black suits (♣ ♠): light gray
- Active player: `>>` prefix in gold
- Dealer / blinds: `[D]`, `[SB]`, `[BB]`
- Your seat: `(you)` suffix
- Folded / all-in: `[fold]`, `[all-in]`
- Preflop hand hint and made-hand hint after the board deals
- Showdown banner: winners, pot size, winning hand category

### Settings file

`~/.ascii-poker.conf`:

```ini
player_name=Alice
host=127.0.0.1
port=12345
language=en
```

Use `language=ru` for Russian. Settings are saved after a successful login and when the language is changed. The password stays in memory for the session only.

## Game rules (server)

| Parameter | Value |
|-----------|-------|
| Starting stack (new accounts) | 1000 chips |
| Small blind | 10 |
| Big blind | 20 |

- Blinds are posted at the start of each hand; the dealer button rotates.
- Betting follows standard Hold'em order; side pots apply for multi-way all-ins.
- The host can **Start** with at least two players; the game also starts automatically when the room reaches max capacity.
- A player who leaves or disconnects mid-hand is auto-folded. The session continues if at least two players remain; when fewer than two players have chips, the session ends.
- The room list is broadcast when rooms are created, joined, left, or a game starts.

## Protocol

TCP with async I/O (Asio). Messages are length-prefixed JSON: 4-byte big-endian size + UTF-8 body.

```json
{ "type": "login", "data": { "username": "Alice", "password": "secret1" } }
```

Reference:

- Message types: `common/include/poker/protocol.h`
- Error codes: `common/include/poker/error_codes.h`
- Auth validation (shared client/server): `common/include/poker/auth.h`

## Project layout

```
common/       Shared poker logic, protocol, auth, packet framing (poker_core)
server/       TCP server, lobby, rooms, game sessions, SQLite storage (poker::server)
client/       FTXUI terminal client (poker::client)
third_party/  Vendored: asio, spdlog, nlohmann/json, FTXUI, Catch2
tests/        Unit and integration tests
scripts/      check-format.sh
```

Build outputs:

- `build/server/poker_server`
- `build/client/poker_client`
- `build/tests/poker_tests`

Runtime SQLite files (`*.db`, `*.db-wal`, `*.db-shm`) are gitignored and created next to the server binary by default.

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

CI (GitHub Actions) builds and runs the test suite on each push.

## Dependencies

| Component | Purpose |
|-----------|---------|
| SQLite3 | Account and chip persistence |
| OpenSSL (libcrypto) | PBKDF2 password hashing |

Other third-party code is vendored under `third_party/` and committed to the repository.

## License

See [LICENSE](LICENSE).
