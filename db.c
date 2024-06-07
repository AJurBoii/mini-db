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

// The following constants are for determing how row information will be stored in memory. Each row will contain an ID, a username, and an email, all of which will be adjacent in memory.
#define ID_SIZE (u_int32_t)size_of_attribute(Row, id)
#define USERNAME_SIZE (u_int32_t)size_of_attribute(Row, username)
#define EMAIL_SIZE (u_int32_t)size_of_attribute(Row, email)
#define ID_OFFSET (u_int32_t)0
#define USERNAME_OFFSET (u_int32_t)ID_OFFSET+ID_SIZE
#define EMAIL_OFFSET (u_int32_t)USERNAME_OFFSET+USERNAME_SIZE
#define ROW_SIZE (u_int32_t)ID_SIZE+USERNAME_SIZE+EMAIL_SIZE

// now we do more memory shenanigans to create the Table structure
#define PAGE_SIZE (u_int32_t)4096
#define TABLE_MAX_PAGES 100
#define ROWS_PER_PAGE (u_int32_t)PAGE_SIZE/ROW_SIZE
#define TABLE_MAX_ROWS (u_int32_t)ROWS_PER_PAGE*TABLE_MAX_PAGES

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

// define return values for executing commands
typedef enum {
    EXECUTE_TABLE_FULL,
    EXECUTE_SUCCESS
} ExecuteResult;

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

typedef struct {
    u_int32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
} Table;

// function that converts Row objects to our compact memory setup
void serialize_row(Row* source, void* destination) {
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

// does literally the opposite
void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

// This properly locates rows in the page. It's more memory shenanigans
void* row_slot(Table* table, u_int32_t row_num) {
    u_int32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    // only allocate memory when necessary
    if (page == NULL) {
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }

    u_int32_t row_offset = row_num % ROWS_PER_PAGE;
    u_int32_t byte_offset = row_offset * ROW_SIZE;

    return page + byte_offset;
}

// parse meta commands
MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        exit(EXIT_SUCCESS);
    } else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// parse sql commands
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

// execute the insert command!! takes the information (id, username, email) from the statement and inserts it into the table
ExecuteResult execute_insert(Statement* statement, Table* table) {
    if (table->num_rows >= TABLE_MAX_ROWS) {
        return EXECUTE_TABLE_FULL;
    }

    Row* row_to_insert = &(statement->row_to_insert);

    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            printf("This would execute a select.\n");
            break;
    }
}

// function that initializes a table
Table* new_table() {
    Table* table = (Table*)malloc(sizeof(Table));
    table->num_rows = 0;
    for (u_int32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pages[i] = NULL;
    }

    return table;
}

// frees all the memory used to create the table
void free_table(Table* table) {
    for (u_int32_t i = 0; table->pages[i]; i++) {
        free(table->pages[i]);
    }
    free(table);
}

int main(int arc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();
    Table* table = new_table();
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
            case(PREPARE_SYNTAX_ERROR):
                printf("Syntax error. Could not parse statement.\n");
                continue;
            case (PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
                continue;
        }

        switch(execute_statement(&statement, table)) {
            case (EXECUTE_SUCCESS):
                printf("Executed.\n");
                break;
            case (EXECUTE_TABLE_FULL):
                printf("Error: Table full.\n");
        }
    }
}