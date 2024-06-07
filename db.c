#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

// establishes how much space to be allocated for usernames and emails
#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255

// defines a quick way to grab the size of an attribute of an object (struct)
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

// Create an InputBuffer object to handle tokenization of user input
typedef struct {
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

// method for initializing an InputBuffer object
InputBuffer* new_input_buffer() {
    InputBuffer* input_buffer = malloc(sizeof(InputBuffer));
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

// print prompt to the output to indicate user input
void print_prompt() { printf("db > "); }

// Reads and stores user input
void read_input(InputBuffer* input_buffer) {
    // Uses getline() to store input into buffer, buffer size stored as well, and we're reading from stdin
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if (bytes_read <= 0) {
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    // Ignore trailing newline or whateva
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
}

void close_input_buffer(InputBuffer* input_buffer) {
    free(input_buffer->buffer);
    free(input_buffer);
}

// define return values for processing meta commands, to be used by do_meta_command()
typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

// define return values for processing statements, to be used by prepare_statement()
typedef enum {
    PREPARE_SUCCESS,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct {
    u_int32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

// The following constants are for determing how row information will be stored in memory. Each row will contain an ID, a username, and an email, all of which will be adjacent in memory.
#define ID_SIZE (u_int32_t)size_of_attribute(Row, id)
#define USERNAME_SIZE (u_int32_t)size_of_attribute(Row, username)
#define EMAIL_SIZE (u_int32_t)size_of_attribute(Row, email)
#define ID_OFFSET (u_int32_t)0
#define USERNAME_OFFSET (u_int32_t)ID_OFFSET+ID_SIZE
#define EMAIL_OFFSET (u_int32_t)USERNAME_OFFSET+USERNAME_SIZE
#define ROW_SIZE (u_int32_t)ID_SIZE+USERNAME_SIZE+EMAIL_SIZE

// parse meta commands
MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// parse sql commands, rn it doesn't really do anything
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement) {
    if (strncmp(input_buffer->buffer, "insert", 6) == 0) {
        int args_assigned = sscanf(
            input_buffer->buffer, "insert %d %s %s", statement->row_to_insert.id, statement->row_to_insert.username, statement->row_to_insert.email
        );
        if (args_assigned < 3) {
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }

    if (strcmp(input_buffer->buffer, "select") == 0) {
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

void execute_statement(Statement* statement) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            printf("This would execute an insert.\n");
            break;
        case (STATEMENT_SELECT):
            printf("This would execute a select.\n");
            break;
    }
}

int main(int arc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();
    while(true) {
        print_prompt();
        read_input(input_buffer);

        if (input_buffer->buffer[0] == '.') {
            switch (do_meta_command(input_buffer))
            {
            case (META_COMMAND_SUCCESS):
                continue;
            case (META_COMMAND_UNRECOGNIZED_COMMAND):
                printf("Unrecognized command '%s'\n", input_buffer->buffer);
                continue;            
            }
        }

        Statement statement;
        switch (prepare_statement(input_buffer, &statement)) {
            case (PREPARE_SUCCESS):
                break;
            case (PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
                continue;
        }

        execute_statement(&statement);
        printf("Executed.\n");
    }
}