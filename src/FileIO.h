#ifndef MINIC_FILE_IO_H
#define MINIC_FILE_IO_H

struct File {
    char *content;
    int length;
};

enum FileIOStatus {
    FILE_IO_SUCCESS,
    FILE_IO_ERROR_FILE_NOT_FOUND,
    FILE_IO_ERROR_UNKNOWN,
};

void FileIO_FreeFile(struct File *f);

enum FileIOStatus FileIO_ReadFile(struct File *f, char *filename);

enum FileIOStatus FileIO_SaveFile(struct File *f, char *filename);

#endif // MINIC_FILE_IO_H
