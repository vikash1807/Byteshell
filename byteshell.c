
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h> //boolean implementation

int shell_cd(char **args);
int shell_help(char **args);
int shell_exit(char **args);
int shell_history(char **args);
int shell_mapfile(char **args);
int shell_echo(char **args);

#define BUFFER_SIZE 4096 

char **strings;
char *echo_string;
int total = 0;

char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "history",
  "mapfile",
  "echo",
};

int (*builtin_func[]) (char **) = {
  &shell_cd,
  &shell_help,
  &shell_exit,
  &shell_history,
  &shell_mapfile,
  &shell_echo
};

int shell_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}
int shell_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "shell: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("shell");
    }
  }
  return 1;
}
int shell_help(char **args)
{
  int i;
  printf("Enter for commands\n");
  printf("Here are some available Builtin commands\n");

  for (i = 0; i < shell_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  return 1;
}

int shell_exit(char **args)
{
    for (int i = 0; i < total; i++)
    {
        free(strings[i]);
    }
    free(strings);
    free(echo_string);
    return 0;
}
int shell_history(char **args)
{
    printf("\nHistory of all implemented commands:\n\n");
    for (int i = 0; i < total; i++)
    {
        printf("%d %s \n",i+1,strings[i]);
    }
    printf("\n");

    return 1;
}
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 100
int shell_mapfile(char **args)
{
    char* filename = args[3];
    printf("%s\n",filename);
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file: %s\n", filename);
        return 1;
    }
    char* lines[MAX_LINES];
    int lineCount = 0;

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (lineCount >= MAX_LINES) {
            printf("number of lines exceeded!\n");
            break;
        }
        size_t lineLength = strlen(line);
        if (line[lineLength - 1] == '\n') {
            line[lineLength - 1] = '\0'; // Remove the newline character
        }
        lines[lineCount] = strdup(line);
        lineCount++;
    }
    fclose(file);
    char* output = malloc(sizeof(char) * MAX_LINE_LENGTH * lineCount);
    if (output == NULL) {
        printf("Memory allocation error!\n");
        return 1;
    }

    output[0] = '\0';

    for (int i = 0; i < lineCount; i++) {
        strcat(output, lines[i]);
        strcat(output, "\n");
        free(lines[i]);
    }

    // Use another command to display the lines (e.g., echo)
    char command[256];
    snprintf(command, sizeof(command), "echo '%s'", output);
    system(command);
    // Clean dynamic allocated memory
    free(output);

    return 1;
}

int shell_echo(char **args)
{
    int i = 0, x=0;
    bool f = false;
    int length=strlen(echo_string);
    for(int i=0;i<length;i++)
    {
        if(i>0 && echo_string[i]=='e' && echo_string[i-1]=='-')
        {
            x=i,f=true;break;
        }
    }
    if(!f)
    {
        i=0;
        while(echo_string[i]!='o') i++;
        i++;
        while(echo_string[i]==' ') i++;
        while(echo_string[i]!='\0')
        {
            if(echo_string[i]=='"')
            {
                i++;continue;
            }
            printf("%c",echo_string[i]);
            i++;
        }
    }
    else
    {
        i=x+1;
        while(i<length && echo_string[i]==' ') i++;
        while (echo_string[i] != '\0') 
        {
            if(echo_string[i]=='"')
            {
                i++;continue;
            }
            if (echo_string[i] == '\\') 
            {
                switch (echo_string[i + 1]) {
                    case 'n': printf("\n"); break;
                    case 't': printf("\t"); break;
                    case 'r': printf("\r"); break;
                    case 'b': printf("\b"); break;
                    case 'a': printf("\a"); break;
                    case 'v': printf("\v"); break;
                    case 'f': printf("\f"); break;
                    case '\\': printf("\\"); break;
                    // case '0': printf("\0"); break;
                    default: printf("\\%c", echo_string[i + 1]); break;
                }
                i++;
            } 
            else
            {
                putchar(echo_string[i]);
            }
            i++;
        }
    }
    putchar('\n');
    return 1;
}
int shell_launch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    if (execvp(args[0], args) == -1) {
      perror("shell");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    perror("shell");
  } else {
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

int shell_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < shell_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
   return shell_launch(args);
}

char *shell_read_line(void)
{
  #define shell_RL_BUFSIZE 1024
  int bufsize = shell_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    if (c == EOF) {
      exit(EXIT_SUCCESS);
    } 
    else if (c == '\n') 
    {
      buffer[position] = '\0';
      //implementation for history -start
      total=total+1;
      strings = realloc(strings,total * sizeof(char *));
      strings[total-1] = malloc(position * sizeof(char));
      strcpy(strings[total-1], buffer);
      //implemention for history -end
      //implementation of echo command-start
      {
        echo_string=realloc(echo_string,(strlen(buffer)+1)*sizeof(char*));
        strcpy(echo_string,buffer);
      }
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += shell_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
// #endif
}

#define shell_TOK_BUFSIZE 64
#define shell_TOK_DELIM " \t\r\n\a"
char **shell_split_line(char *line)
{
  int bufsize = shell_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  if (!tokens) {
    fprintf(stderr, "shell: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, shell_TOK_DELIM);
  while (token != NULL)
  {
    tokens[position] = token;
    position++;
    if (position >= bufsize) {
      bufsize += shell_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
		free(tokens_backup);
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, shell_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void shell_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("> ");
    line = shell_read_line();
    args = shell_split_line(line);
    status = shell_execute(args);

    free(line);
    free(args);
  } while (status);
}

int main(int argc, char **argv)
{
  shell_loop();
  return EXIT_SUCCESS;
}
