# Last Call runtime

## Authority and data flow

Unity collects input and renders physics/animation; it does not invent narrative state.

    Player command → action validation → world event → per-character perception
    → local memory / beliefs → scheduled decision → validated action → visible player state

The local server binds only 127.0.0.1 on a dynamic port. A fresh bearer token is passed through the child process environment, never command-line arguments. The client uses proxy-free loopback HTTP/WebSocket; model traffic remains HTTPS to the selected gateway. Closing the client closes the managed service's stdin, which saves and exits.

## Modules

| Module | Responsibility |
| --- | --- |
| config / scenarios | Validated declarative character, relationship, card, voice and beat data |
| identity | Deterministic Context Profile extraction and identity-pack selection; per-decision brief (no model calls) |
| world / engine | Clock, seeded random state, facts, events, jobs, lifecycle |
| visibility | Independent hearing, sight, privacy and perception completeness |
| navigation | Exported collision grid, A*, corner checks, reserved destinations |
| commands / decisions | Idempotency, movement acknowledgement, boundaries, evidence validation, relation updates |
| beats | Eight opportunity conditions and closing rhythm |
| view | Permission-filtered dialogue, cards, clues and reflection |
| model | Per-character context, JSON Schema, timeout/retry/concurrency budget fallback |
| store | SQLite snapshots, immutable event sequence, decision journal and player isolation |
| server | Authenticated REST and WebSocket lifecycle |

Actor relationships are directed and six-dimensional. Boundary and initiative remain separate actor policies, not extra affinity scores. The rule adapter consumes the same actor memory and perception as the model adapter. It is labeled as rules mode, not simulated online generation.

An action suggestion cannot add actors, change the timeline, create facts, or mutate arbitrary relationships. Evidence must be in that character's memory; distant on-site speech becomes pending movement first. C/D entry is determined by world conditions, not model text.

## Protocol v1

- GET /api/bootstrap?playerId=...: public scenario choices and that player's save summaries.
- POST /api/model-config: loopback-authenticated write of compatible API base/model/key; returns configured status, base and model, never the key.
- POST /api/session: new / resume / next, playerId, role, intent, style, seed and online.
- GET /api/state and /api/reflection: filtered state / public reflection.
- POST /api/command: command id, type, optional sessionId, version and cursor.
- POST /api/save and /api/shutdown: save / stop this service.
- WS /api/events: filtered state snapshots, reliable command IDs and acknowledgements.

The initial or reconnected snapshot includes all player-visible history; regular updates use the latest 45 visible entries. The client merges entries by event ID. No full world log, belief text or numerical relationship map is sent to normal UI.

Position reports are coalesced into a latest-only batch and are not replayed. A single sender gives interactive commands priority over telemetry, preventing frame-rate-dependent queue growth. Other commands are retried in their original client order and deduplicated per world. Rejected commands are acknowledged as errors and removed from the retry queue. Commands from a previous world are rejected.

## Persistence and reproduction

Normal storage is outside the project in LOCALAPPDATA/LALAGAME. The managed verification launcher uses a separate Verification database and config directory. Normal player model settings are written to the current profile's `.lalagame/private/model.env`; the endpoint accepts HTTPS remotes and HTTP loopback only. The key is never returned to Unity, bundled, added to saves, or logged with authorization headers, environment dumps or provider error bodies.

Snapshots include the PRNG state, actor memories, pending actions, route revisions, clock, commands and event cursor. Model decisions are journaled privately with source event and acceptance status. Reopening preserves already-observed outcomes without calling the model again for the past. A fresh model call is not promised to reproduce identical text.

Rules-mode reproduction is checked over whole event sequences, not just the first response. The live-night harness acknowledges movement along the exported grid and completes all 720 effective seconds; native-window verification separately checks actual CharacterController traversal.

## Editing boundaries

The canonical actor IDs and eight beat-effect names form the scenario contract. Change prose, goals, relationships, colors, opportunities and existing conditions through JSON; introducing new mechanics or new effect types requires code plus tests. Rebuilding scene furniture requires a fresh navigation export.

Personas and identity packs are also JSON-editable declarative data: `personas` carries the four characters' invariant cores, voices and the shared 80/20 humanity rule, while `identityPacks` carries the occupational shells (`urban_capital_default` plus the domain-specific variants), their topic mix and scene-skin prose. Pack selection is a deterministic function of the player's three choice answers plus free-text background, never a model call; the raw player text never reaches a Character Agent, only a sanitised Context Profile. A mid-game revision is staged as `pendingPackRevision` and applied at the next beat boundary so established relationship state and memory survive. Adding a new identity pack or a new choice option stays in JSON; adding a new actor ID or beat effect still requires code plus tests.

The three memory tiers are bounded short, relationship and long-term lists. Beliefs retain evidence provenance and uncertainty. No psychological diagnosis, good/bad ending score, actual photography, voice recorder or external multiplayer service is present.
