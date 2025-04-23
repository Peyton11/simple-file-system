// ACADEMIC INTEGRITY PLEDGE
//
// - I have not used source code obtained from another student nor
//   any other unauthorized source, either modified or unmodified.
//
// - All source code and documentation used in my program is either
//   my original work or was derived by me from the source code
//   published in the textbook for this course or presented in
//   class.
//
// - I have not discussed coding details about this project with
//   anyone other than my instructor. I understand that I may discuss
//   the concepts of this program with other students and that another
//   student may help me debug my program so long as neither of us
//   writes anything during the discussion or modifies any computer
//   file during the discussion.
//
// - I have violated neither the spirit nor letter of these restrictions.
//
//
//
// Signed: Peyton Gardner Date: 4/15/2025

// filesys.c
// Based on a program by Michael Black, 2007
// Revised 11.3.2020 O'Neil

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Macros for common values
#define SECTOR_SIZE 512
#define DISK_MAP_SECTOR 256
#define DIR_SECTOR 257
#define MAX_SECTORS 2880
#define MAX_FILE_SIZE 26
#define DIR_ENTRY_SIZE 16

// Function prototypes
void readSector(FILE* disk, int sector, char* buffer);
void writeSector(FILE* disk, int sector, const char* buffer);
void printMap(const char* map);
void printDirectory(const char* dir);
int findFreeDirectoryEntry(char* dir);
int findFreeSectors(const char* map, int needed, int* sectorsOut);
void markSectors(char* map, const int* sectors, int count);
int addFile(FILE* disk, char* map, char* dir, const char* filename);

int main(int argc, char* argv[])
{
	int i;
	int j;

	// Open the floppy image
	FILE* floppy = fopen("floppya.img", "r+");
	if (floppy == 0)
	{
		printf("floppya.img not found\n");
		return 0;
	}

	// Load the disk map from sector 256
	char map[512];
	fseek(floppy, 512 * 256, SEEK_SET);
	for (i = 0; i < 512; i++)
		map[i] = fgetc(floppy);

	// Load the directory from sector 257
	char dir[512];
	fseek(floppy, 512 * 257, SEEK_SET);
	for (i = 0; i < 512; i++)
		dir[i] = fgetc(floppy);

	// Print disk map
	printf("Disk usage map:\n");
	printf("      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
	printf("     --------------------------------\n");
	for (i = 0; i < 16; i++) {
		switch (i) {
			case 15: printf("0xF_ "); break;
			case 14: printf("0xE_ "); break;
			case 13: printf("0xD_ "); break;
			case 12: printf("0xC_ "); break;
			case 11: printf("0xB_ "); break;
			case 10: printf("0xA_ "); break;
			default: printf("0x%d_ ", i); break;
		}
		for (j = 0; j < 16; j++) {
			if ((unsigned char)map[16 * i + j] == 0xFF) printf(" X");
			else printf(" .");
		}
		printf("\n");
	}

	// Print directory
	printf("\nDisk directory:\n");
	printf("Name    Type Start Length\n");
	for (i = 0; i < 512; i += 16) {
		if (dir[i] == 0) break;
		for (j = 0; j < 8; j++) {
			if (dir[i + j] == 0) printf(" ");
			else printf("%c", dir[i + j]);
		}
		if ((dir[i + 8] == 't') || (dir[i + 8] == 'T')) printf("text");
		else printf("exec");
		printf(" %5d %6d bytes\n", (unsigned char)dir[i + 9], 512 * (unsigned char)dir[i + 10]);
	}

	// Add file if requested
	if (argc == 3 && strcmp(argv[1], "add") == 0) {
		char* inputFilename = argv[2];
		FILE* input = fopen(inputFilename, "rb");
		if (!input) {
			printf("Cannot open input file: %s\n", inputFilename);
			fclose(floppy);
			return 1;
		}

		// Get file size
		fseek(input, 0, SEEK_END);
		long fileSize = ftell(input);
		rewind(input);

		int neededSectors = (fileSize + 511) / 512;
		int sectors[26]; // max 26 sectors for safety
		int found = 0;

		for (i = 0; i < 512 && found < neededSectors; i++) {
			if ((unsigned char)map[i] != 0xFF) {
				sectors[found++] = i;
			}
		}

		if (found < neededSectors) {
			printf("Not enough free sectors on the disk.\n");
			fclose(input);
			fclose(floppy);
			return 1;
		}

		// Find free directory slot
		int entry = -1;
		for (i = 0; i < 512; i += 16) {
			if (dir[i] == 0) {
				entry = i;
				break;
			}
		}

		if (entry == -1) {
			printf("No free directory entry available.\n");
			fclose(input);
			fclose(floppy);
			return 1;
		}

		// Write file content to disk
		for (i = 0; i < neededSectors; i++) {
			char buffer[512] = {0};
			fread(buffer, 1, 512, input);
			fseek(floppy, sectors[i] * 512, SEEK_SET);
			fwrite(buffer, 1, 512, floppy);
			map[sectors[i]] = (char)0xFF; // mark as used
		}

		// Write to directory
		for (i = 0; i < 8; i++) {
			dir[entry + i] = (i < strlen(inputFilename)) ? inputFilename[i] : 0x00;
		}
		dir[entry + 8] = 't'; // type = text
		dir[entry + 9] = (char)sectors[0]; // starting sector
		dir[entry + 10] = (char)neededSectors; // sector count

		// Save map and directory
		fseek(floppy, 512 * 256, SEEK_SET);
		for (i = 0; i < 512; i++) fputc(map[i], floppy);
		fseek(floppy, 512 * 257, SEEK_SET);
		for (i = 0; i < 512; i++) fputc(dir[i], floppy);

		printf("\nFile '%s' added successfully (%ld bytes, %d sectors).\n", inputFilename, fileSize, neededSectors);

		fclose(input);
	}

	fclose(floppy);
	return 0;
}

// Utility function to read a sectory
void readSector(FILE* disk, int sector, char* buffer) {
	fseek(disk, sector * SECTOR_SIZE, SEEK_SET);
	fread(buffer, 1, SECTOR_SIZE, disk);
}

// Utility function to write a sector
void writeSector(FILE* disk, int sector, const char* buffer) {
    fseek(disk, sector * SECTOR_SIZE, SEEK_SET);
    fwrite(buffer, 1, SECTOR_SIZE, disk);
}

// Prints the disk usage map
void printMap(const char* map) {
    printf("Disk usage map:\n");
    printf("      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
    printf("     --------------------------------\n");
    for (int i = 0; i < 16; i++) {
        printf("0x%X_ ", i);
        for (int j = 0; j < 16; j++) {
            unsigned char val = (unsigned char)map[16 * i + j];
            printf(val == 0xFF ? " X" : " .");
        }
        printf("\n");
    }
}

// Prints the directory
void printDirectory(const char* dir) {
    printf("\nDisk directory:\n");
    printf("Name     Type Start Length\n");
    for (int i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE) {
        if (dir[i] == 0x00) continue;
        char name[9] = {0};
        memcpy(name, &dir[i], 8);
        char type = dir[i + 8];
        int start = (unsigned char)dir[i + 9];
        int length = (unsigned char)dir[i + 10];

        printf("%-8s %4s %5d %6d bytes\n", name, type == 't' ? "text" : "exec", start, length * SECTOR_SIZE);
    }
}

// Finds a free directory entry
int findFreeDirectoryEntry(char* dir) {
    for (int i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE) {
        if (dir[i] == 0x00) return i;
    }
    return -1;
}

// Finds free sectors on disk
int findFreeSectors(const char* map, int needed, int* sectorsOut) {
    int found = 0;
    for (int i = 0; i < MAX_SECTORS && found < needed; i++) {
        if ((unsigned char)map[i] == 0x00) {
            sectorsOut[found++] = i;
        }
    }
    return found == needed;
}

// Marks sectors in the map
void markSectors(char* map, const int* sectors, int count) {
    for (int i = 0; i < count; i++) {
        map[sectors[i]] = (char)0xFF;
    }
}

// Adds a text file to the disk
int addFile(FILE* disk, char* map, char* dir, const char* filename) {
    FILE* input = fopen(filename, "rb");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", filename);
        return 0;
    }

    fseek(input, 0, SEEK_END);
    long fileSize = ftell(input);
    fseek(input, 0, SEEK_SET);

    int neededSectors = (fileSize + SECTOR_SIZE - 1) / SECTOR_SIZE;
    if (neededSectors > MAX_FILE_SIZE) {
        fprintf(stderr, "Error: File too large (max %d sectors)\n", MAX_FILE_SIZE);
        fclose(input);
        return 0;
    }

    int sectors[neededSectors];
    if (!findFreeSectors(map, neededSectors, sectors)) {
        fprintf(stderr, "Error: Not enough free sectors\n");
        fclose(input);
        return 0;
    }

    int dirPos = findFreeDirectoryEntry(dir);
    if (dirPos < 0) {
        fprintf(stderr, "Error: No free directory entry\n");
        fclose(input);
        return 0;
    }

    // Set directory entry
    memset(&dir[dirPos], 0, DIR_ENTRY_SIZE);
    strncpy(&dir[dirPos], filename, 8);
    dir[dirPos + 8] = 't'; // file type
    dir[dirPos + 9] = (char)sectors[0]; // start
    dir[dirPos + 10] = (char)neededSectors; // length

    // Write file content to sectors
    for (int i = 0; i < neededSectors; i++) {
        char buffer[SECTOR_SIZE] = {0};
        fread(buffer, 1, SECTOR_SIZE, input);
        writeSector(disk, sectors[i], buffer);
    }

    markSectors(map, sectors, neededSectors);
    fclose(input);
    return 1;
}
