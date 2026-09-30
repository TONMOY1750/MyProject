# Advanced Multi-User Terminal Chat System with End-to-End Encryption (E2EE)

## Project Overview

This project is a secure, real-time, multi-user terminal chat system developed in C.

The system is designed using a Zero-Knowledge Client-Server Relay Architecture, where users can communicate through an encrypted connection while the server works as a relay.

## Key Features

- Multi-user real-time communication
- End-to-End Encryption (E2EE)
- Diffie-Hellman based key exchange
- TCP/IP networking using Winsock2
- Multithreading using Win32 API
- Zero-Knowledge Client-Server Relay
- Terminal-based user interface
- Private messaging between users
- Custom low-level utility functions

## Terminal Commands

```text
/users
/msg <user> <message>
/help
/clear
/exit
