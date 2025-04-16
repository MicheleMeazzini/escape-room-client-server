# Escape Room Client-Server Application

This repository contains a client-server application simulating an escape room game.  
The project was developed as part of the *Computer Networks* course in the Computer Engineering Bachelor's Degree program.

## 🎯 Overview

The goal of this project is to implement a networked escape room game where a client interacts with a server to solve puzzles and advance through game stages.  
The communication between client and server is handled via TCP sockets.

## 🧩 Features

- Client-server architecture using TCP sockets  
- Interactive escape room gameplay with multiple puzzles  
- Modular C code with separate files for client and server logic

## 📁 Project Structure

The repository includes the following files:

- **`client.c`**: Source code for the client-side application  
- **`server.c`**: Source code for the server-side application  
- **`makefile`**: Makefile to compile both client and server  
- **`exec2024.sh`**: Shell script to execute the compiled programs  
- **`Documentazione_Meazzini_635889.pdf`**: Full documentation including design decisions, protocol description, and usage instructions

## ⚙️ Getting Started

### Prerequisites

- GCC compiler or any C99-compliant compiler  
- Linux environment (recommended for socket compatibility)  
- `make` utility installed

### Compilation

Open a terminal in the project directory and run:

```bash
make
```

This will compile both `client` and `server` binaries.

### Execution

In two separate terminals:

1. Start the server:

```bash
./server
```

2. Start the client:

```bash
./client
```

You can also use the provided `exec2024.sh` script for automated execution.

For further details on commands, interaction, and protocol, see the included `Documentazione_Meazzini_635889.pdf`.

## 📄 License

This project is licensed under the MIT License.

