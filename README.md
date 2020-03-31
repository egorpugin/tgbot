# tgbot

C++17 library for Telegram bot API with generated API types and methods.

* Current Bot API v4.7

Fork of https://github.com/reo7sp/tgbot-cpp

Features:

* Generated API structures and methods. No manual typing errors and easily updatable for API changes.
* Upgraded to C++ 17.
* Improved connection settings to fix hangs in original library.
* Lots of cleanups.

License: MIT

# Build

Using SW: https://software-network.org/

Download client, unpack, add to PATH.

Build: `sw build`

(Dev) Generate IDE project: `sw generate`

Dependencies: nlohmann.json, libcurl.
