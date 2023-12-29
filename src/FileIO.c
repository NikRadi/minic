#include "FileIO.h"
#include <stdio.h>
#include <stdlib.h>

void FileIO_FreeFile(struct File *f) {
    free(f->content);
    f->content = NULL;
    f->length = 0;
}

enum FileIOStatus FileIO_ReadFile(struct File *f, char *filename) {
    FILE *stream;
    int error_code = fopen_s(&stream, filename, "rb");
    if (error_code != 0) {
        switch (error_code) {
            case 2: return FILE_IO_ERROR_FILE_NOT_FOUND;
            default: return FILE_IO_ERROR_UNKNOWN;
        }
    }

    fseek(stream, 0, SEEK_END);
    int file_length = ftell(stream);
    rewind(stream);

    char *file_content = (char *) malloc((file_length + 1) * sizeof(char));
    fread((void *) file_content, sizeof(char), file_length, stream);
    fclose(stream);

    file_content[file_length] = '\0';
    f->content = file_content;
    f->length = file_length;
    return FILE_IO_SUCCESS;
}

enum FileIOStatus FileIO_SaveFile(struct File *f, char *filename) {
    FILE *stream;
    if (fopen_s(&stream, filename, "wb") != 0) {
        return FILE_IO_ERROR_UNKNOWN;
    }

    fprintf(stream, f->content);
    fclose(stream);
    return FILE_IO_SUCCESS;
}
