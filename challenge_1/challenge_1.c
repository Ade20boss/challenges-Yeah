#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 16
#define MIN_PRINTABLE 32
#define MAX_PRINTABLE 126

int hex_dump(char *filename)
{
    char table[] = "0123456789abcdef";
    size_t bytes_read;
    size_t offset = 0;

    unsigned char buffer[BUFFER_SIZE];

    FILE *file_pointer = fopen(filename, "rb");
    if (file_pointer == NULL)
    {
        fprintf(stderr, "Cannot open file\n");
        fclose(file_pointer);
        return 1;
    }


    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file_pointer)) > 0)
    {
        printf("%08zx: ", offset);
        for (size_t i = 0; i < BUFFER_SIZE; i++)
        {
            if (i < bytes_read)
            {
                printf("%c%c", table[buffer[i] >> 4], table[buffer[i] & 0x0F]);
            }

            else
            {
                printf("  ");
            }

            if (i % 2 != 0)
            {
                printf(" ");
            }
        }

        printf(" ");

        for (size_t j = 0; j < bytes_read; j++)
        {
            if (buffer[j] >= MIN_PRINTABLE && buffer[j] <= MAX_PRINTABLE)
            {
                printf("%c", buffer[j]);
            }
            else
            {
                printf(".");
            }
        }

        offset += bytes_read;

        printf("\n");
    }


    if (ferror(file_pointer))
    {
        fprintf(stderr, "An error occured while reading the file.\n");
        fclose(file_pointer);
        return 1;
    }

    else if (feof(file_pointer))
    {
        fclose(file_pointer);
        return 0;
    }

    fclose(file_pointer);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(-1);
    }

    if (hex_dump(argv[1]) != 0)
    {
        fprintf(stderr, "An error occured while reading the file.\n");
        exit(-1);
    }

    return 0;
}
