# O.S. Lab 3: A Simple File System

This C program simulates a basic file system stored within a 1.44MB floppy disk image (`floppya.img`). It provides tools to inspect the disk, view its directory, and add new files to it using a simple text/executable file format.

## Getting Started

### Prerequisites

- A C compiler (e.g., `gcc`)
- A blank or initialized `floppya.img` file (1.44MB floppy disk image)

### Compiling

```bash
make build
```

or

```bash
gcc -o filesys filesys.c
```

### Running

- ./filesys D <filename>: Delete the named file from the disk
- ./filesys L: List the files on the disk
- ./filesys M <filename>: Create a text file and store it to disk
- ./filesys P <filename>: Read the named file and print it to screen
