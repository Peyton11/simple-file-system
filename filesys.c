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
int findFreeDirectoryEntry(char* dir);
int findFreeSectors(const char* map, int needed, int* sectorsOut);
void markSectors(char* map, const int* sectors, int count);
void listFiles(const char* dir, const char* map);
void printFile(FILE* disk, const char* dir, const char* filename);
void createFile(FILE* disk, char* map, char* dir, const char* filename);
void deleteFile(FILE* disk, char* map, char* dir, const char* filename);
void printHelp();

int main(int argc, char* argv[])
{
    int i;

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

    // Handle commands
    if (argc == 2 && strcmp(argv[1], "L") == 0) {
        listFiles(dir, map);
    } else if (argc == 3 && strcmp(argv[1], "P") == 0) {
        printFile(floppy, dir, argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "M") == 0) {
        createFile(floppy, map, dir, argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "D") == 0) {
        deleteFile(floppy, map, dir, argv[2]);
    } else {
        printHelp();
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

// List the files on the disk
void listFiles(const char* dir, const char* map) {
    int totalUsed = 0;
    int totalFree = 0;

    printf("\nFiles on disk:\n");
    for (int i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE) {
        if (dir[i] == 0x00) continue; // Skip empty entries

        char name[9] = {0};
        memcpy(name, &dir[i], 8);
        char type = dir[i + 8];
        int length = (unsigned char)dir[i + 10];

        printf("%s.%c %d bytes\n", name, type, length * SECTOR_SIZE);
        totalUsed += length * SECTOR_SIZE;
    }

    for (int i = 0; i < MAX_SECTORS; i++) {
        if ((unsigned char)map[i] == 0x00) {
            totalFree += SECTOR_SIZE;
        }
    }

    printf("\nTotal space used: %d bytes\n", totalUsed);
    printf("Total free space: %d bytes\n", totalFree);
}

// Read the named file and print it to the screen
void printFile(FILE* disk, const char* dir, const char* filename) {
    for (int i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE) {
        if (dir[i] == 0x00) continue; // Skip empty entries

        char name[9] = {0};
        memcpy(name, &dir[i], 8);

        if (strncmp(name, filename, 8) == 0) {
            if (dir[i + 8] != 't') {
                printf("Error: File is not a text file.\n");
                return;
            }

            int start = (unsigned char)dir[i + 9];
            int length = (unsigned char)dir[i + 10];
            char buffer[SECTOR_SIZE];

            for (int j = 0; j < length; j++) {
                readSector(disk, start + j, buffer);
                for (int k = 0; k < SECTOR_SIZE; k++) {
                    if (buffer[k] == 0x00) return; // End of file
                    putchar(buffer[k]);
                }
            }
            return;
        }
    }
    printf("Error: File not found.\n");
}

// Create a text file and store it to disk
void createFile(FILE* disk, char* map, char* dir, const char* filename) {
    // Check for duplicate file name and find a free directory entry
    int entry = findFreeDirectoryEntry(dir);
    if (entry == -1) {
        printf("Error: No free directory entry available.\n");
        return;
    }
    for (int i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE) {
        if (strncmp(&dir[i], filename, 8) == 0) {
            printf("Error: Duplicate or invalid file name.\n");
            return;
        }
    }

    // Find a free sector
    int sector = -1;
    for (int i = 0; i < MAX_SECTORS; i++) {
        if ((unsigned char)map[i] == 0x00) {
            sector = i;
            break;
        }
    }
    if (sector == -1) {
        printf("Error: No free sectors available.\n");
        return;
    }

    // Prompt user for file content
    char buffer[SECTOR_SIZE] = {0};
    printf("Enter file content (max 512 characters): ");
    fgets(buffer, SECTOR_SIZE, stdin);

    // Write file content to disk
    writeSector(disk, sector, buffer);
    map[sector] = (char)0xFF;

    // Write to directory
    memset(&dir[entry], 0, DIR_ENTRY_SIZE);
    strncpy(&dir[entry], filename, 8);
    dir[entry + 8] = 't'; // File type
    dir[entry + 9] = (char)sector; // Starting sector
    dir[entry + 10] = 1; // Length in sectors

    // Save map and directory
    fseek(disk, 512 * 256, SEEK_SET);
    fwrite(map, 1, SECTOR_SIZE, disk);
    fseek(disk, 512 * 257, SEEK_SET);
    fwrite(dir, 1, SECTOR_SIZE, disk);

    printf("File '%s' created successfully.\n", filename);
}

// Delete the named file from disk
void deleteFile(FILE* disk, char* map, char* dir, const char* filename) {
    for (int i = 0; i < SECTOR_SIZE; i += DIR_ENTRY_SIZE) {
        if (dir[i] == 0x00) continue; // Skip empty entries

        char name[9] = {0};
        memcpy(name, &dir[i], 8);

        if (strncmp(name, filename, 8) == 0) {
            int start = (unsigned char)dir[i + 9];
            int length = (unsigned char)dir[i + 10];

            // Free the sectors in the map
            for (int j = 0; j < length; j++) {
                map[start + j] = 0x00;
            }

            // Mark the directory entry as free
            dir[i] = 0x00;

            // Save map and directory
            fseek(disk, 512 * 256, SEEK_SET);
            fwrite(map, 1, SECTOR_SIZE, disk);
            fseek(disk, 512 * 257, SEEK_SET);
            fwrite(dir, 1, SECTOR_SIZE, disk);

            printf("File '%s' deleted successfully.\n", filename);
            return;
        }
    }
    printf("Error: File not found.\n");
}

// Command usage
void printHelp() {
	printf("Invalid command. Usage:\n");
	printf("./filesys L: List files on disk\n");
	printf("./filesys P <filename>: Read the named file and print it to the screen\n");
	printf("./filesys M <filename>: Create a text file and store it to disk\n");
	printf("./filesys D <filename>: Delete the named file from the disk\n");
}
