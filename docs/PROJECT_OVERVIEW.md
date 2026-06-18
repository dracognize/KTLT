# KTLT Banking Terminal - Project Overview

## Introduction
The KTLT Banking Terminal is a high-performance, client-server banking application built with C++26. It features a modern Terminal User Interface (TUI) powered by **FTXUI** and a robust, asynchronous networking layer using **ASIO**.

The project is designed with a strong focus on performance and customizability, evidenced by its bespoke standard library implementation for core data structures.

## System Architecture
The system follows a classic client-server model:
- **Client (`app/`)**: A TUI application that allows users to create accounts, login, check balances, deposit/withdraw funds, and transfer money to other users.
- **Server (`server/`)**: A multi-threaded server that manages a flat-file database, handles client requests, and maintains a transaction log.
- **Network Protocol (`libs/network/`)**: A custom packet-based protocol for communication between client and server.
- **Base Library (`libs/base/`)**: A collection of custom-built data structures (Vector, List, Map, Hash Table, etc.) and utilities (SHA-256) used throughout the project to minimize external dependencies and optimize performance.

## Key Features
- **Secure Authentication**: Password storage using SHA-256 hashing with salt.
- **Asynchronous Database Operations**: The server processes database requests in a dedicated worker thread to ensure high responsiveness.
- **Responsive TUI**: Interactive terminal interface with keyboard shortcuts and mouse support.
- **Transaction History**: Persistent logging of all financial transactions.
- **Custom High-Performance Containers**: STL-like containers optimized for the project's specific needs.

## Directory Structure
- `app/`: Source code for the client application and its TUI pages.
- `server/`: Source code for the banking server and database manager.
- `libs/base/`: Custom data structures and utility headers.
- `libs/network/`: Network packet definitions and communication logic.
- `data/`: Directory where the server stores account information and transaction logs.
- `docs/`: Technical documentation (this directory).
- `tests/`: Utilities for generating test data.

## Technology Stack
- **Language**: C++26
- **Build System**: CMake 4.2+
- **TUI Library**: FTXUI
- **Networking**: ASIO (header-only version)
- **Concurrency**: C++ Threads and Mutexes
