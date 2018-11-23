#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glob.h>
#include <assert.h>
#include <fcntl.h>
#include "history.h"

// This is defined in string.h
// BUT ONLY if you use -std=gnu99
//extern char *strdup(char *);

// Function forward references

void trim(char *);
int strContains(char *, char *);
char **tokenise(char *, char *);
char **fileNameExpand(char **);
void freeTokens(char **);
char *findExecutable(char *, char **);
int isExecutable(char *);
void execute(char **, char **, char **);
void prompt(void);


// Global Constants

#define MAXLINE 200

// Global Data

/* none ... unless you want some */

#define HISTFILE ".mymysh_history"


// Main program
// Set up enviroment and then run main loop
// - read command, execute command, repeat

int main(int argc, char *argv[], char *envp[])
{
   pid_t pid;   // pid of child process
   int stat;    // return status of child
   char **path; // array of directory names
   int cmdNo;   // command number
   int i;       // generic index

   // set up command PATH from environment variable
   for (i = 0; envp[i] != NULL; i++) {
      if (strncmp(envp[i], "PATH=", 5) == 0) break;
   }
   if (envp[i] == NULL)
      path = tokenise("/bin:/usr/bin",":");
   else
      // &envp[i][5] skips over "PATH=" prefix
      path = tokenise(&envp[i][5],":");
#ifdef DBUG
   for (i = 0; path[i] != NULL;i++)
      printf("path[%d] = %s\n",i,path[i]);
#endif

   // initialise command history
   // - use content of ~/.mymysh_history file if it exists
   cmdNo = initCommandHistory();
   printf("%d\n" , cmdNo);

   // main loop: print prompt, read line, execute command

   static int command_counter = 0;
   char line[MAXLINE];
   prompt();
   while (fgets(line, MAXLINE, stdin) != NULL) {

      trim(line); // remove leading/trailing space

      if (strcmp(line,"exit") == 0) break;
      if (strcmp(line,"") == 0) { printf("mysh$ "); continue; }

      int expansion = strContains(line , "*?[~");
      int redirection_input = strContains(line , "<");
      int redirection_output = strContains(line , ">");

      char ** command = tokenise(line , " ");
      char container_path[MAXLINE];

      //printf("--------------------\n");
      if(expansion == 1){

         char ** files = fileNameExpand(command);
         addToCommandHistory(line , command_counter);
         command_counter++;
         pid = fork();

         if (pid == 0){
            execute(files  , path , envp);
         } else {
            wait(&stat);
            freeTokens(files);
            freeTokens(command);
         }
         //printf("--------------------\n");

      } else if (strcmp(command[0] , "cd") == 0){

            // ----- CHANGE DIRECTORY ----- //

         char * dir_name = command[1];
         char * cd = getcwd(container_path , MAXLINE);
         int byte_needed = strlen(cd) + strlen("/") + strlen(dir_name) + 1;
         char * new_path = malloc(byte_needed * sizeof(char));
         new_path = strcpy(new_path , cd);
         strcat(new_path , "/");
         strcat(new_path , dir_name);
         int change_dir = chdir(new_path);
         free(new_path);

         if (change_dir == -1){
            printf("Directory does not exist!\n");
         } else {
            addToCommandHistory(line , command_counter);
            command_counter++;
         }

            // ----- PRINT WORKING DIRECTORY ----- //

      } else if (strcmp(command[0] , "pwd") == 0){

         addToCommandHistory(line , command_counter);
         command_counter++;
         char * cd = getcwd(container_path , MAXLINE);
         printf("%s\n" , cd);

            // ----- DISPLAY HISTORY LIST ----- //

      } else if (strcmp(command[0] , "h") == 0){

         char * PATH = getenv("HOME");
         char * full_path = malloc((strlen(PATH) + strlen(HISTFILE) + strlen("/")) * sizeof(char));
         strcpy(full_path , PATH);
         strcat(full_path , "/");
         strcat(full_path , HISTFILE);

         FILE * fd = fopen(full_path , "r");

         if(fd == NULL){
            printf("Error reading file!\n");
         }

         char command_array[MAXLINE];
         addToCommandHistory(line , command_counter);

         while(fgets(command_array , MAXLINE , fd) != NULL){
            printf("%s" , command_array);
         }

         fclose(fd);
         free(full_path);


      } else if (redirection_input == 1){

           // PRINT FROM FILE TO TERMINAL USING REDIRECTION
           //printf("--------------------\n");

           char * input = "";

           int i = 0;
           while(command[i] != NULL){
               trim(command[i]);
               i++;
           }

           char ** command = tokenise(line , "<");

           int j = 0;
           while(command[j] != NULL){
               trim(command[j]);
               j++;
           }

           for(int i = 0; *command[i] != '\0'; i++) {

            if(strcmp(command[i] , "cat") == 0 && command[i + 1] != NULL) {

               input = malloc(strlen(command[i + 1]) * sizeof(char));
               strcpy(input , command[i+1]);

               pid = fork();

               if(pid  < 0){
                   printf("Error while forking!\n");
               } else if (pid == 0) {
                   int fd0 = open(input , O_RDONLY);
                   if(fd0 == -1){
                       printf("Error opening file!\n");
                   } else {
                       dup2(fd0, STDIN_FILENO);
                       close(fd0);
                       execute(command , path , envp);
                       exit(1);
                   }
               } else {
                   wait(&stat);
                   freeTokens(command);
                   free(input);
               }
               //printf("--------------------\n");
            }
         }
      } else if (redirection_output == 1) {

           //printf("--------------------\n");

           int l = 0;
           int no_of_tokens = 0;
           while(command[l] != NULL){
            //printf("%s\n" , command[l]);
            l++;
            no_of_tokens++;
           }

           char ** new_command = malloc((no_of_tokens + 1) * sizeof(char *)); //EXTRA SLOT FOR NULL


           //MAKE A SET OF COMMANDS WITHOUT THE REDIRECT OPERATOR
           int index = 0;
           int j = 0;
           for(int i = 0; i < no_of_tokens; i++){
               if(strcmp(command[i] , ">") != 0){
                  new_command[j] = malloc(strlen(command[i]) * sizeof(char));
                  strcpy(new_command[j] , command[i]);
                  j++;
                  index = j;
               }
           }
           index--;

           new_command[index + 1] = NULL; //SETTING EXTRA SLOT TO NULL

           int i = 0;
           while(new_command[i] != NULL){
               //printf("%s\n" , new_command[i]);
               i++;
           }

           // SAVE FILE NAME
           char * input = malloc(strlen(new_command[index]) * sizeof(char));
           strcpy(input , new_command[index]);

           // FREE THE FILE NAME FROM LIST OF COMMANDS (NOT PASSED TO EXECUTE FUNCTION)
           free(new_command[index]);
           new_command[index] = NULL;

           pid = fork();

           if(pid  < 0){
             printf("Error while forking!\n");
           } else if (pid == 0) {
             int fd0 = open(input , O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC);
             if(fd0 == -1){
               printf("Error!\n");
             } else {
               dup2(fd0 , STDOUT_FILENO);
               close(fd0);
               execute(new_command , path , envp);
             }
           } else {
               wait(&stat);
               freeTokens(command);
               free(input);
           }
           //printf("--------------------\n");
      } else {

         addToCommandHistory(line , command_counter);
         command_counter++;

         pid = fork();

         if (pid == 0){
            execute(command  , path , envp);
         } else {
            wait(&stat);
            freeTokens(command);
         }
         //printf("--------------------\n");
      }

      prompt();

   }
   saveCommandHistory();
   cleanCommandHistory();
   printf("\n");
   return(EXIT_SUCCESS);
}

void execute(char **args, char **path, char **envp)
{

   char path_array[BUFSIZ];

   int is_executable = 0;

   if(args[0][0] == '/' || args[0][0] == '.'){
      //IF FILE NAME STARTS WITH '/' OR '.' , CHECK IF IT IS EXECUTABLE.
      //IF IT IS USE IT AS COMMAND FOR EXECVE
     is_executable = isExecutable(args[0]);
     execve(args[0] , args,  envp);
   } else {
      int i = 0;
      for(i = 0; i < 5; i++){
         //IF A SHELL PROGRAM IS TO BE RUN , ITERATE THROUGH DIRECTORIES
         //AND LOOK FOR IT. STORES ITS ABSOLUTE PATH.
         snprintf(path_array , BUFSIZ , "%s/%s" , path[i] , args[0]);
         is_executable = isExecutable(path_array);
         if(is_executable == 1){
            break;
         }
      }
   }

   if(is_executable != 1){

      printf("Command not found!\n");

   } else {

      printf("Running %s\n" , path_array);

      //USING THE PATH DETERMINED ABOVE , CALL EXECVE TO RUN THE PRORGAM
      //BY CONVENTION ARGS[0] IS THE SAME AS PATH_ARRAY
      //(ARGS IS A PONTER TO THE 2D-ARRAY THAT STORES THE COMMANDS FROM COMMAND LINE AS AN ARRAY OF STRINGS)
      //PATH_ARRAY = ABSOLUTE PATH OF PROGRAM TO BE EXECUTED

      //printf("PATH ARRAY: %s\n" , path_array);
      //printf("----- %s -----\n" , args[1]);

      execve(path_array , args,  envp);

   }
   exit(1);
}


// fileNameExpand: expand any wildcards in command-line args
// - returns a possibly larger set of tokens
char **fileNameExpand(char **tokens)
{

   int no_of_toks = 0;
   int i = 0;
   while(tokens[i] != NULL){
      no_of_toks = no_of_toks + 1;
      i++;
   }

   //DECLARE A GLOB STRUCT TO HOLD RELEVANT DATA
   glob_t glob_results;

   //INITIAL CALL TO GLOB DOES NOT HAVE GLOB_APPEND FLAG SINCE AN INSTANCE OF THE GLOB STRUCT MUST FIRST BE MADE
   glob(tokens[0] , GLOB_NOCHECK | GLOB_TILDE , NULL , &glob_results);

   //EVERY CONSECUTIVE TOKEN IS GLOBBED , AND THE RESULTS ARE APPENDED TO THE SAME STRUCT
   for(int i = 1; i < no_of_toks; i++){
     glob(tokens[i] , GLOB_NOCHECK | GLOB_TILDE | GLOB_APPEND , NULL , &glob_results);
   }

   char ** s = malloc((glob_results.gl_pathc + 1) * sizeof(char *));

   int index = 0;
   for(int i = 0; i < glob_results.gl_pathc; i++){
      s[i] = malloc(strlen(glob_results.gl_pathv[i]));
      strcpy(s[i] , glob_results.gl_pathv[i]);
      index =  i;
   }

   //FINAL STRING POINTS TO NULL
   s[index + 1] = NULL;

   globfree(&glob_results);

   return s;

}

// findExecutable: look for executable in PATH
char *findExecutable(char *cmd, char **path)
{
      char executable[MAXLINE];
      executable[0] = '\0';
      if (cmd[0] == '/' || cmd[0] == '.') {
         strcpy(executable, cmd);
         if (!isExecutable(executable))
            executable[0] = '\0';
      }
      else {
         int i;
         for (i = 0; path[i] != NULL; i++) {
            sprintf(executable, "%s/%s", path[i], cmd);
            if (isExecutable(executable)) break;
         }
         if (path[i] == NULL) executable[0] = '\0';
      }
      if (executable[0] == '\0')
         return NULL;
      else
         return strdup(executable);
}

// isExecutable: check whether this process can execute a file
int isExecutable(char *cmd)
{
   struct stat s;
   // must be accessible
   if (stat(cmd, &s) < 0)
      return 0;
   // must be a regular file
   //if (!(s.st_mode & S_IFREG))
   if (!S_ISREG(s.st_mode))
      return 0;
   // if it's owner executable by us, ok
   if (s.st_uid == getuid() && s.st_mode & S_IXUSR)
      return 1;
   // if it's group executable by us, ok
   if (s.st_gid == getgid() && s.st_mode & S_IXGRP)
      return 1;
   // if it's other executable by us, ok
   if (s.st_mode & S_IXOTH)
      return 1;
   return 0;
}

// tokenise: split a string around a set of separators
// create an array of separate strings
// final array element contains NULL
char **tokenise(char *str, char *sep)
{
   // temp copy of string, because strtok() mangles it
   char *tmp;
   // count tokens
   tmp = strdup(str);
   int n = 0;
   strtok(tmp, sep); n++;
   while (strtok(NULL, sep) != NULL) n++;
   free(tmp);
   // allocate array for argv strings
   char **strings = malloc((n+1)*sizeof(char *));
   assert(strings != NULL);
   // now tokenise and fill array
   tmp = strdup(str);
   char *next; int i = 0;
   next = strtok(tmp, sep);
   strings[i++] = strdup(next);
   while ((next = strtok(NULL,sep)) != NULL)
      strings[i++] = strdup(next);
   strings[i] = NULL;
   free(tmp);
   return strings;
}

// freeTokens: free memory associated with array of tokens
void freeTokens(char **toks)
{
   for (int i = 0; toks[i] != NULL; i++)
      free(toks[i]);
   free(toks);
}

// trim: remove leading/trailing spaces from a string
void trim(char *str)
{
   int first, last;
   first = 0;
   while (isspace(str[first])) first++;
   last  = strlen(str)-1;
   while (isspace(str[last])) last--;
   int i, j = 0;
   for (i = first; i <= last; i++) str[j++] = str[i];
   str[j] = '\0';
}

// strContains: does the first string contain any char from 2nd string?
int strContains(char *str, char *chars)
{
   for (char *s = str; *s != '\0'; s++) {
      for (char *c = chars; *c != '\0'; c++) {
         if (*s == *c) return 1;
      }
   }
   return 0;
}

// prompt: print a shell prompt
// done as a function to allow switching to $PS1
void prompt(void)
{
   printf("mymysh$ ");
}
