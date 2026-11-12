# C IRC Server

A multi-threaded IRC (Internet Relay Chat) server built from scratch in C, compliant with classic RFC 1459 and RFC 2812 specifications.

## Features
- **Multi-threaded client handling** using `pthread`
- **Robust socket management** with address reuse and explicit error handling
- **Cross-channel user search** functionality via a clean `ClientContext` and `Channel` data structure
- **Optimized build system** supporting a Makefile with precompiled header structure (`pch.h`)

## NOTICE
The server runs on port 6667 by default. If you want to change the port look inside the main.c source file.

## Getting Started

### Prerequisites
Make sure you have `gcc` and `make` installed on your system.

### Building the Project
Clone the repository and run `make`:

```bash
1. clone the repo
2. make
3. ./IRCserver <your ip>
