# O.S. Lab 3: A Simple File System

This C program simulates a basic file system stored within a 1.44MB floppy disk image (`floppya.img`). It provides tools to inspect the disk, view its directory, and add new files to it using a simple text/executable file format.

## Features

- View disk usage map (sectors marked as used or free)
- Display contents of the directory
- Add files to the disk image (`add` command)

## Getting Started

### Prerequisites

- A C compiler (e.g., `gcc`)
- A blank or initialized `floppya.img` file (1.44MB floppy disk image)

### Compiling

```bash
gcc -o filesys filesys.c
```

### Running

#### View Disk Info

```bash
./filesys
```

#### Add a file

```bash
./filesys add myfile.txt
```

### File Format

Each directory is 16 bytes:
- Bytes 0-7: Filename (max 8 characters)
- Byte 8: File type ('t' for text, 'e' for exec)
- Byte 9: Starting sector
- Byte 10: Length in sectors
- Bytes 11-15: Unused/reserved

The disk map (sector 256) is 512 bytes long:
- Each byte represents a sector
- `0xFF` means "used", any other values is "free".
The directory (sector 257) is 512 bytes and holds up to 32 file entries.
