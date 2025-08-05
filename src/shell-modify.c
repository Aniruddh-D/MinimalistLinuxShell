/*================================================================================================================================================================*/
//@file shell - modify.c @brief A simple shell implementation in C with I / O redirection and pipes.@author Team - 12 Sasken
/*================================================================================================================================================================*/

/************************************************************************  Includes *******************************************************************************/
#include <sys/wait.h>  // Wait for the child process to terminate.
#include <unistd.h>    // Standard symbolic constants and types.
#include <stdlib.h>    // Standard library definitions.
#include <string.h>    // String operations.
#include <stdio.h>     // Standard input/output definitions.
#include <sys/types.h> // Data types used in system calls.
#include <fcntl.h>     // File control operations.
#include <errno.h>     // Error number definitions.
#include <ctype.h>     // For character type checking
/************************************************************************  Define constants **********************************************************************/
#define LSH_RL_BUFSIZE 1024         // Buffer size for reading the command.
#define LSH_TOK_BUFSIZE 64          // Buffer size for storing the tokens.
#define LSH_TOK_DELIM " \t\r\n\a"   // Delimiters for the tokens.
#define REDIRECT_INPUT "<"          // Input redirection symbol.
#define REDIRECT_OUTPUT ">"         // Output redirection symbol.
#define REDIRECT_OUTPUT_APPEND ">>" // Output redirection append symbol.
#define PIPE_TOKEN "|"              // Pipe symbol.
/**********************************************************************  Function Prototypes **********************************************************************/

int lsh_cd(char **args);             // Change directory.
int lsh_help(char **args);           // Display help information.
int lsh_exit(char **args);           // Exit the shell.
int lsh_pwd(char **args);            // Print working directory.
int lsh_echo(char **args);           // Echo arguments.
int handle_redirection(char **args); // Handle input/output redirection.
int find_pipe(char **args);          // Find pipe in the command.
int execute_pipeline(char **args);   // Execute command pipeline.
int lsh_launch(char **args);         // Launch a new process.
int lsh_execute(char **args);        // Execute a command.
char **lsh_split_line(char *line);   // Split a line into tokens.
char *lsh_read_line(void);           // Read a line from input.
int lsh_num_builtins();              // Return the number of built-in commands.

/**********************************************************************  Built-in command names and function pointers **********************************************************************/
char *builtin_str[] = {"cd", "help", "exit", "pwd", "echo"};                           // Built-in command names
int (*builtin_func[])(char **) = {&lsh_cd, &lsh_help, &lsh_exit, &lsh_pwd, &lsh_echo}; // Built-in command functions

/**********************************************************************  Return the number of built-in commands **********************************************************************/
int lsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

/**********************************************************************  Change directory built-in command **********************************************************************/
int lsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "minishell: expected argument to \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("minishell");
        }
    }
    return 1;
}

/**********************************************************************  Help built-in command **********************************************************************/
int lsh_help(char **args)
{
    int i;
    printf("\t  __Minimalist Shell__\n\t\tSasken\n\n Team Members:\n1.Aniruddh Dubey\n2.Aryan\n3.Aryan Saxena\n4.Abhilash Dalai\n5.Vishesh Kumar Bhagat\n6.Smritparna Mahanty\n");
    printf("\n\nType program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i = 0; i < lsh_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }

    printf("\nUse redirection symbols:\n");
    printf("  < to redirect input\n");
    printf("  > to redirect output (overwrites file)\n");
    printf("  >> to append output to file\n");
    printf("Use | to pipe commands together.\n");
    printf("Use the man command for information on other programs.\n");
    return 1;
}

/**********************************************************************  Exit built-in command **********************************************************************/
int lsh_exit(char **args)
{
    return 0;
}

/**********************************************************************  Print working directory built-in command **********************************************************************/
int lsh_pwd(char **args)
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("%s\n", cwd);
    }
    else
    {
        perror("minishell");
    }
    return 1;
}

/**********************************************************************  Echo built-in command **********************************************************************/
int lsh_echo(char **args)
{
    // Print all arguments
    for (int i = 1; args[i] != NULL; i++)
    {
        fprintf(stdout, "%s", args[i]);
        // Only add a space if not the last argument
        if (args[i + 1] != NULL)
        {
            fprintf(stdout, " ");
        }
    }
    fprintf(stdout, "\n");
    fflush(stdout); // Ensure output is written
    return 1;
}

/**********************************************************************  Read a line from standard input **********************************************************************/
char *lsh_read_line(void)
{
    int bufsize = LSH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "minishell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }
        else
        {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize)
        {
            bufsize += LSH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                fprintf(stderr, "minishell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/**********************************************************************  Tokenisation (Split a line into tokens) **********************************************************************/
char **lsh_split_line(char *line)
{
    int bufsize = LSH_TOK_BUFSIZE;
    int position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));

    if (!tokens)
    {
        fprintf(stderr, "minishell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Make a copy of the line for tokenization
    char *line_copy = strdup(line);
    if (!line_copy)
    {
        fprintf(stderr, "minishell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Now proceed with regular tokenization
    int i = 0;
    int start = 0;
    int in_quote = 0;
    char quote_char = 0;
    int len = strlen(line_copy);

    while (i <= len)
    {
        // Handle quotes
        if ((line_copy[i] == '"' || line_copy[i] == '\'') && (i == 0 || line_copy[i - 1] != '\\'))
        {
            if (!in_quote)
            {
                in_quote = 1;
                quote_char = line_copy[i];
                start = i + 1; // Skip the opening quote
            }
            else if (line_copy[i] == quote_char)
            {
                // End of quoted string
                line_copy[i] = '\0'; // Replace closing quote with null terminator
                tokens[position++] = strdup(&line_copy[start]);
                start = i + 1;
                in_quote = 0;
            }
        }
        // Check for >> sequence first
        else if (!in_quote && i < len - 1 && line_copy[i] == '>' && line_copy[i + 1] == '>')
        {
            // If there's text before the >> operator, add it as a token
            if (i > start)
            {
                line_copy[i] = '\0';
                tokens[position++] = strdup(&line_copy[start]);
            }

            // Add the >> as a token
            tokens[position++] = strdup(REDIRECT_OUTPUT_APPEND);
            i += 2; // Skip both > characters
            start = i;
            continue; // Skip the increment at the end of the loop
        }
        // Handle other special characters when not in quotes
        else if (!in_quote && (line_copy[i] == '<' || line_copy[i] == '>' || line_copy[i] == '|'))
        {
            // If there's text before the special character, add it as a token
            if (i > start)
            {
                char temp = line_copy[i];
                line_copy[i] = '\0';
                tokens[position++] = strdup(&line_copy[start]);
                line_copy[i] = temp;
            }

            // Add the special character as its own token
            char special[2] = {line_copy[i], '\0'};
            tokens[position++] = strdup(special);
            start = i + 1;
        }
        // Handle whitespace outside quotes
        else if (!in_quote && (isspace(line_copy[i]) || line_copy[i] == '\0'))
        {
            // If there's text before the whitespace, add it as a token
            if (i > start)
            {
                line_copy[i] = '\0';
                tokens[position++] = strdup(&line_copy[start]);
            }
            start = i + 1;
        }

        // Check if we need to resize tokens array
        if (position >= bufsize)
        {
            bufsize += LSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "minishell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        i++;
    }

    tokens[position] = NULL;
    free(line_copy);
    return tokens;
}

/**********************************************************************  Handle redirection **********************************************************************/
int handle_redirection(char **args)
{
    int i;

    // Look for redirection symbols
    for (i = 0; args[i] != NULL; i++)
    {
        // Output redirection
        if (strcmp(args[i], REDIRECT_OUTPUT) == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "minishell: expected file after %s\n", REDIRECT_OUTPUT);
                return -1;
            }

            char *filename = args[i + 1];

            // Create file with proper mode
            FILE *fp = fopen(filename, "w");
            if (fp == NULL)
            {
                fprintf(stderr, "minishell: cannot open %s for writing: %s\n",
                        filename, strerror(errno));
                return -1;
            }

            // Get file descriptor and redirect stdout
            int fd = fileno(fp);
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                fprintf(stderr, "minishell: failed to redirect output: %s\n", strerror(errno));
                fclose(fp);
                return -1;
            }

            // Remove redirection tokens from args
            args[i] = NULL;
            i++; // Skip the filename
        }

        // Output append redirection
        else if (strcmp(args[i], REDIRECT_OUTPUT_APPEND) == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "minishell: expected file after %s\n", REDIRECT_OUTPUT_APPEND);
                return -1;
            }

            char *filename = args[i + 1];

            // Open file in append mode
            FILE *fp = fopen(filename, "a");
            if (fp == NULL)
            {
                fprintf(stderr, "minishell: cannot open %s for appending: %s\n",
                        filename, strerror(errno));
                return -1;
            }

            // Get file descriptor and redirect stdout
            int fd = fileno(fp);
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                fprintf(stderr, "minishell: failed to redirect output: %s\n", strerror(errno));
                fclose(fp);
                return -1;
            }

            // Remove redirection tokens from args
            args[i] = NULL;
            i++; // Skip the filename
        }

        // Input redirection
        else if (strcmp(args[i], REDIRECT_INPUT) == 0)
        {
            if (args[i + 1] == NULL)
            {
                fprintf(stderr, "minishell: expected file after %s\n", REDIRECT_INPUT);
                return -1;
            }

            char *filename = args[i + 1];

            FILE *fp = fopen(filename, "r");
            if (fp == NULL)
            {
                fprintf(stderr, "minishell: cannot open %s for reading: %s\n",
                        filename, strerror(errno));
                return -1;
            }

            // Get file descriptor and redirect stdin
            int fd = fileno(fp);
            if (dup2(fd, STDIN_FILENO) == -1)
            {
                fprintf(stderr, "minishell: failed to redirect input: %s\n", strerror(errno));
                fclose(fp);
                return -1;
            }

            // Remove redirection tokens from args
            args[i] = NULL;
            i++; // Skip the filename
        }
    }
    return 0;
}

// Find pipe in arguments
int find_pipe(char **args)
{
    int i;
    for (i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], PIPE_TOKEN) == 0)
        {
            return i;
        }
    }
    return -1;
}

/**********************************************************************  Execute a pipeline of commands **********************************************************************/
int execute_pipeline(char **args)
{
    int pipe_pos = find_pipe(args);

    // If no pipe is found, execute as normal command
    if (pipe_pos == -1)
    {
        return lsh_launch(args);
    }

    // Split command at pipe
    args[pipe_pos] = NULL;
    char **next_cmd = &args[pipe_pos + 1];

    int pipefd[2];
    pid_t pid1, pid2;

    // Create pipe
    if (pipe(pipefd) == -1)
    {
        perror("minishell");
        return 1;
    }

    // Fork first process
    pid1 = fork();
    if (pid1 < 0)
    {
        perror("minishell");
        return 1;
    }

    if (pid1 == 0) // First child process
    {
        // Redirect stdout to pipe
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1)
        {
            perror("minishell");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);

        // Handle any redirections in the first command
        if (handle_redirection(args) == -1)
        {
            exit(EXIT_FAILURE);
        }

        // Execute first command
        if (execvp(args[0], args) == -1)
        {
            perror("minishell");
            exit(EXIT_FAILURE);
        }
    }

    // Fork second process
    pid2 = fork();
    if (pid2 < 0)
    {
        perror("minishell");
        return 1;
    }

    if (pid2 == 0) // Second child process
    {
        // Redirect stdin to pipe
        close(pipefd[1]);
        if (dup2(pipefd[0], STDIN_FILENO) == -1)
        {
            perror("minishell");
            exit(EXIT_FAILURE);
        }
        close(pipefd[0]);

        // Handle any redirections in the second command
        if (handle_redirection(next_cmd) == -1)
        {
            exit(EXIT_FAILURE);
        }

        // Execute second command
        if (execvp(next_cmd[0], next_cmd) == -1)
        {
            perror("minishell");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both processes to finish
    int status;
    waitpid(pid1, &status, 0);
    waitpid(pid2, &status, 0);

    // Check if there are more pipes in the second command
    if (find_pipe(next_cmd) != -1)
    {
        return execute_pipeline(next_cmd);
    }

    return 1;
}

/**********************************************************************  Launch an external command **********************************************************************/
int lsh_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) // Child process
    {
        // Handle any redirections
        if (handle_redirection(args) == -1)
        {
            perror("minishell");
            exit(EXIT_FAILURE);
        }

        // Execute command
        if (execvp(args[0], args) == -1)
        {
            perror("minishell");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0) // Error forking
    {
        perror("minishell");
    }
    else // Parent process
    {
        do
        {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

/**********************************************************************  Command execution **********************************************************************/
int lsh_execute(char **args)
{
    int i;
    int has_redirection = 0;

    if (args[0] == NULL)
    {
        return 1;
    }

    // Check if there's any redirection in the command
    for (i = 0; args[i] != NULL; i++)
    {
        if (strcmp(args[i], REDIRECT_INPUT) == 0 ||
            strcmp(args[i], REDIRECT_OUTPUT) == 0 ||
            strcmp(args[i], REDIRECT_OUTPUT_APPEND) == 0)
        {
            has_redirection = 1;
            break;
        }
    }

    /**********************************************************************  Built-in command handling **********************************************************************/
    for (i = 0; i < lsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0)
        {
            if (has_redirection)
            {
                // For built-ins with redirection, fork a child process
                pid_t pid = fork();

                if (pid == 0)
                {
                    // Child process - set up redirection
                    if (handle_redirection(args) == -1)
                    {
                        exit(EXIT_FAILURE);
                    }

                    // Execute the built-in
                    exit((*builtin_func[i])(args) ? EXIT_SUCCESS : EXIT_FAILURE);
                }
                else if (pid < 0)
                {
                    perror("minishell");
                    return 1;
                }
                else
                {
                    // Parent process - wait for child
                    int status;
                    waitpid(pid, &status, 0);
                    return 1;
                }
            }
            else
            {
                // No redirection, just run the built-in directly
                return (*builtin_func[i])(args);
            }
        }
    }

    // Check for pipes and handle accordingly
    if (find_pipe(args) != -1)
    {
        return execute_pipeline(args);
    }

    // Otherwise execute as a regular command
    return lsh_launch(args);
}

/**********************************************************************  Main shell loop **********************************************************************/
void lsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do
    {
        printf("T-12_MiniShell_Sasken >");
        line = lsh_read_line();
        args = lsh_split_line(line);
        status = lsh_execute(args);

        free(line);
        free(args);
    } while (status);
}

/**********************************************************************  Main entry point **********************************************************************/
int main(int argc, char **argv)
{
    // Run command loop
    lsh_loop();

    // Perform any shutdown/cleanup
    return EXIT_SUCCESS;
}
