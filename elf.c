#include <stdio.h>
#include <string.h>

int
main (int argc, char **argv)
{
    const char *command = "help";
    if (argc > 1)
        command = argv[1];

    if (strcmp (command, "compile") == 0) {
        printf ("NOT IMPLEMENTED\n");
        return 1;
    }
    else if (strcmp (command, "version") == 0) {
        printf ("0\n");
        return 0;
    }
    else if (strcmp (command, "help") == 0) {
        printf ("Elf is a programming languge designed for teching how memory works.\n"
                "\n"
                "Usage:\n"
                "  elf compile <file>  - Compile an elf program\n"
                "  elf version         - Show the version of the Elf tool\n"
                "  elf help            - Show help information\n");
        return 0;
    }
    else {
        printf ("Unknown command \"%s\", run elf to see supported commands.\n", command);
        return 1;
    }

    return 0;
}
