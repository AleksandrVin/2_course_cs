# Task 1 "File transfer FIFO"

## Description

- This program is used for sending file from one terminal to other in one OS
- Data transfer is reliable
- Program uses FIFO as main method
- Print file to stdout by default
- FIFO pathname located in /tmp/file_transfer_pipes

> [!IMPORTANT]
> No functions like "look" allowed

## How to use

- launch with

```console
./file_transfer -send ${fileDirName} XXXX
```

> id = XXXX

- If sender ready to transfer open in other terminal with

```console
./file_transfer -receive
```
>this call will tell you ID 
---

## Planing

### Stage 1

- [X] By using FIFO make file transfer

### Stage 2

- [ ] Add size of file chack 

- [ ] Make program reliable


- [ ] try to show it ot teacher

## Data

[Знакомство с межпроцессным взаимодействием на Linux](https://habr.com/ru/post/122108/)
