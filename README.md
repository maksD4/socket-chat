# socket-chat
Instant messaging application that allows for client-to-client communication over sockets. Socket-chat is built in C with GTK4 desktop client and a server backed by MongoDB and Redis
<p align="center"> 
 <img src="https://github.com/maksD4/socket-chat/blob/master/socket-chat.gif" width=800>
</p>

## Features
- Account creation and password-based login
- Friend list display
- Multiple one-on-one conversation display
- Group conversations (more than 3 users)
- Storage of user data and their converations
- Native desktop GUI (GTK4) for the client

## Project Structure
 
```
socket-chat/
├── client/   # GTK4 client source files
├── server/   # Server source files (socket handling, MongoDB, Redis)
├── lib/      # Shared library code used by both client and server
├── Makefile  # Build rules for the client and server
└── README.md
```

## Requirements
Make sure the following are installed on your system:
 
- `gcc` and `make`
- `pkg-config`
- GTK4 development libraries (for the client)
- MongoDB C driver (`libmongoc-1.0`, `libbson-1.0`)
- `hiredis` (Redis C client)
- A running MongoDB instance
- A running Redis instance
On Debian/Ubuntu-based systems, the development libraries can typically be installed with:
 
```bash
sudo apt install build-essential pkg-config libgtk-4-dev libmongoc-1.0-0 libmongoc-dev libbson-dev libhiredis-dev
```

## Building
 
The project is built using `make`.
 
```bash
# Build both the server and client (default target)
make
# or explicitly
make all
 
# Build just the server
make server
 
# Build just the GUI client
make client
```
 
This produces two binaries:
 
- `s.out` — the server
- `c.out` — the client
