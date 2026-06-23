# Local Lab

This folder contains optional local testing helpers. It is for development and may not need to be included in the final upstream PR unless maintainers want a reproducible lab setup.

Recommended first test:

1. Run Mosquitto on the Mac with Homebrew.
2. Configure TC002 to connect to the Mac LAN IP.
3. Publish a manual payload with `mosquitto_pub`.
4. Add Home Assistant after the broker-to-TC002 path works.

See `docs/README.md`.
