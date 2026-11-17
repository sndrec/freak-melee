## Gameplay

- Fixed grounded C-stick smash attacks sometimes failing with an input buffer enabled, especially when the stick reaches full travel over several frames.

## Online

- Added heartbeats and automatic recovery for temporary lobby and chat connection failures.
- Missing players disappear from chat after 45 seconds. Lobby seats and settings are reserved for another two minutes so a brief outage can recover without destroying the lobby.
- Lobbies stay available while other players remain connected. If the host times out, another participant takes over.
- Joining a new lobby moves you out of your previous lobby. Reconnecting replaces your old presence instead of creating a duplicate player.
- Recovered players must ready up again before a match can start.
- Chat now shows Offline when another connection replaces yours.

Everyone playing together should update to this build. Older guest clients need the update to connect to the updated online service.
