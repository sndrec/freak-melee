# Signaling service dependencies

The service uses the Cloudflare Workers runtime and its built-in Node crypto
module for Ed25519 signature verification. The package lock records the
licenses of development tools and their dependencies.

The native client uses Monocypher 4.0.2 with its standard Ed25519 module.
Its BSD-2-Clause / CC0 license is included in `melee/licenses/monocypher.txt`
and prepared native releases.
