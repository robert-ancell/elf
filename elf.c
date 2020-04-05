#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum {
    TOKEN_TYPE_WORD,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_TEXT,
    TOKEN_TYPE_ASSIGN,
    TOKEN_TYPE_ADD,
    TOKEN_TYPE_SUBTRACT,
    TOKEN_TYPE_MULTIPLY,
    TOKEN_TYPE_DIVIDE,
    TOKEN_TYPE_OPEN_PAREN,
    TOKEN_TYPE_CLOSE_PAREN,
    TOKEN_TYPE_OPEN_BRACE,
    TOKEN_TYPE_CLOSE_BRACE,
} TokenType;

typedef struct {
    TokenType type;
    size_t offset;
    size_t length;
} Token;

static bool
is_number_char (char c)
{
    return c >= '0' && c <= '9';
}

static char
ending_char_is_escaped (const char *data, Token *token)
{
    if (token->length < 3)
        return false;

    return data[token->offset + token->length - 2] == '\\';
}

static char
string_is_complete (const char *data, Token *token)
{
    // Need at least an open and closing quote
    if (token->length < 2)
        return false;

    // Both start and end characters need to be the same
    char start_char = data[token->offset];
    char end_char = data[token->offset + token->length - 1];
    if (end_char != start_char)
        return false;

    // Closing quote needs to be non-escaped
    return !ending_char_is_escaped (data, token);
}

static bool
token_is_complete (const char *data, Token *token, char next_c)
{
   switch (token->type) {
   case TOKEN_TYPE_WORD:
       return isspace (next_c);
   case TOKEN_TYPE_NUMBER:
       return !is_number_char (next_c);
   case TOKEN_TYPE_TEXT:
       return string_is_complete (data, token);
   case TOKEN_TYPE_ASSIGN:
   case TOKEN_TYPE_ADD:
   case TOKEN_TYPE_SUBTRACT:
   case TOKEN_TYPE_MULTIPLY:
   case TOKEN_TYPE_DIVIDE:
   case TOKEN_TYPE_OPEN_PAREN:
   case TOKEN_TYPE_CLOSE_PAREN:
   case TOKEN_TYPE_OPEN_BRACE:
   case TOKEN_TYPE_CLOSE_BRACE:
       return true;
   }

   return false;
}

static int
parse_elf_source (const char *data, size_t data_length)
{
    Token **tokens = NULL;
    size_t tokens_length = 0;
    Token *current_token = NULL;

    for (size_t offset = 0; offset < data_length; offset++) {
        // FIXME: Support UTF-8
        char c = data[offset];

        if (current_token != NULL && token_is_complete (data, current_token, c))
            current_token = NULL;

        if (current_token == NULL) {
            // Skip whitespace
            if (isspace (c))
                continue;

            tokens_length++;
            tokens = realloc (tokens, sizeof (Token *) * tokens_length); // FIXME: Double size each time
            Token *token = tokens[tokens_length - 1] = malloc (sizeof (Token));
            memset (token, 0, sizeof (Token));
            token->type = TOKEN_TYPE_WORD;
            token->offset = offset;
            token->length = 1;

            if (c == '(')
                token->type = TOKEN_TYPE_OPEN_PAREN;
            else if (c == ')')
                token->type = TOKEN_TYPE_CLOSE_PAREN;
            else if (c == '{')
                token->type = TOKEN_TYPE_OPEN_BRACE;
            else if (c == '}')
                token->type = TOKEN_TYPE_CLOSE_BRACE;
            else if (c == '=')
                token->type = TOKEN_TYPE_ASSIGN;
            else if (c == '+')
                token->type = TOKEN_TYPE_ADD;
            else if (c == '-')
                token->type = TOKEN_TYPE_SUBTRACT;
            else if (c == '*')
                token->type = TOKEN_TYPE_MULTIPLY;
            else if (c == '/')
                token->type = TOKEN_TYPE_DIVIDE;
            else if (is_number_char (c))
                token->type = TOKEN_TYPE_NUMBER;
            else if (c == '"' || c == '\'')
                token->type = TOKEN_TYPE_TEXT;
            else
                token->type = TOKEN_TYPE_WORD;

            current_token = token;
        }
        else {
            current_token->length++;
        }
    }

    for (size_t i = 0; i < tokens_length; i++) {
        Token *token = tokens[i];
        printf ("%.*s\n", (int) token->length, data + token->offset);
        free (token);
    }
    free (tokens);

    return 0;
}

static int
compile_elf_source (const char *filename)
{
    int fd = open (filename, O_RDONLY);
    if (fd < 0) {
        printf ("Failed to open file \"%s\": %s\n", filename, strerror (errno));
        return 1;
    }

    struct stat file_info;
    if (fstat (fd, &file_info) < 0) {
        close (fd);
        printf ("Failed to get information: %s\n", strerror (errno));
        return 1;
    }
    size_t data_length = file_info.st_size;

    char *data = mmap (NULL, data_length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        close (fd);
        printf ("Failed to map file: %s\n", strerror (errno));
        return 1;
    }

    int result = parse_elf_source (data, data_length);

    munmap (data, data_length);
    close (fd);

    return result;
}

int
main (int argc, char **argv)
{
    const char *command = "help";
    if (argc > 1)
        command = argv[1];

    if (strcmp (command, "compile") == 0) {
        if (argc < 3) {
            printf ("Need file to compile, run elf help for more information\n");
            return 1;
        }
        const char *filename = argv[2];

        return compile_elf_source (filename);
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
