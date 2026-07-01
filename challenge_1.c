#include <stdio.h>
#include <stdlib.h>

#define READ_SIZE 16

int hex_dump(char *filename)
{
    char table[] = "0123456789abcdef";
    size_t bytes_read;
    size_t offset = 0;

    unsigned char buffer[READ_SIZE];

    FILE *file_pointer = fopen(filename, "rb");
    if (file_pointer == NULL)
    {
        fprintf(stderr, "Cannot open file\n");
        return 1;
    }


    while ((bytes_read = fread(buffer, 1, READ_SIZE, file_pointer)) > 0)
    {
        printf("%08zx: ", offset);
        for (size_t i = 0; i < 16; i++)
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
            if (buffer[j] >= 32 && buffer[j] <= 126)
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
        return 1;
    }

    else if (feof(file_pointer))
    {
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
