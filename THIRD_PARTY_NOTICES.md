# Third-Party Notices

This project builds on the following open-source libraries, each under its own
license. Full license texts are available at the linked sources.

## Direct dependencies

| Library | Version | License | Source |
|---------|---------|---------|--------|
| PubSubClient | ^2.8 | MIT | https://github.com/knolleary/pubsubclient |
| ArduinoJson | ^7.3.0 | MIT | https://github.com/bblanchon/ArduinoJson |
| ESPAsyncWebServer | ^1.2.4 | LGPL-3.0 | https://github.com/me-no-dev/ESPAsyncWebServer |
| AsyncTCP | ^1.1.1 | LGPL-3.0 | https://github.com/me-no-dev/AsyncTCP |
| FastLED | ^3.9.4 | MIT | https://github.com/FastLED/FastLED |

## LGPL notice

`ESPAsyncWebServer` and `AsyncTCP` are licensed under LGPL-3.0. The firmware
links them statically, so the LGPL requires that recipients be able to modify
and relink these components against a modified version. The compiled object
files for these libraries are made available on request to satisfy this
obligation.

## Upstream attribution

The dongle hardware is pin-compatible with, and inspired by, the open-source
[plan-d-io P1-dongle](https://github.com/plan-d-io/P1-dongle) project
(MIT License, Copyright (c) 2021 plan-d-io).
