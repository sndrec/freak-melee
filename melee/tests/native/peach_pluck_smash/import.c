/* Console-only entry for the actual importer. Peach has no project registry
 * entry, so this does not manufacture a clone or bypass package validation. */
#include <melee/ft/forward.h>

#include <stdio.h>
#include <stdlib.h>

#include <melee/ft/native_fighter_parameters.h>

static char* read_file(const char* path, size_t* size)
{
    FILE* file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);
    char* bytes = malloc(*size);
    if (!bytes || fread(bytes, 1, *size, file) != *size) {
        abort();
    }
    fclose(file);
    return bytes;
}
static void write_file(const char* path, char* bytes, size_t size)
{
    FILE* file = fopen(path, "wb");
    if (!file || fwrite(bytes, 1, size, file) != size) {
        abort();
    }
    fclose(file);
}
int main(int argc, char** argv)
{
    if (argc != 5) {
        return 2;
    }
    size_t dat_size, source_size, owned_size, parameter_size;
    char *dat = read_file(argv[1], &dat_size),
         *source = read_file(argv[2], &source_size);
    char *owned, *parameters;
    unsigned line = 0;
    if (!dat || !source ||
        !nativeFighterParameters_Import(
            dat, dat_size, "ftDataPeach", Ft_Kind_Peach, source, source_size,
            &owned, &owned_size, &parameters, &parameter_size, &line))
    {
        fprintf(stderr, "Import failed at source line %u\n", line);
        return 1;
    }
    write_file(argv[3], owned, owned_size);
    write_file(argv[4], parameters, parameter_size);
    printf("Actual Peach DAT importer converted %zu parameter bytes.\n",
           parameter_size);
    free(dat);
    free(source);
    free(owned);
    free(parameters);
    return 0;
}
