// COMP1521 18s2 mysh ... command history
// Implements an abstract data object

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

// This is defined in string.h
// BUT ONLY if you use -std=gnu99
//extern char *strdup(const char *s);

// Command History
// array of command lines
// each is associated with a sequence number

#define MAXHIST 20
#define MAXSTR  200

#define HISTFILE ".mymysh_history"

typedef struct _history_entry {
   int   seqNumber;
   char *commandLine;
} HistoryEntry;

typedef struct _history_list {
   int nEntries;
   HistoryEntry commands[MAXHIST];
} HistoryList;

HistoryList CommandHistory;

// initCommandHistory()
// - initialise the data structure
// - read from .history if it exists

int initCommandHistory()
{
   char * PATH = getenv("HOME");
   char * full_path = malloc((strlen(PATH) + strlen(HISTFILE) + strlen("/")) * sizeof(char));
   strcpy(full_path , PATH);
   strcat(full_path , "/");
   strcat(full_path , HISTFILE);

   FILE * fd = fopen(full_path , "r");

   if(fd == NULL){
      printf("Error reading file!\n");
   } else {
      char command[MAXSTR];
      int i = 0;
      while(fgets(command , MAXSTR, fd) != NULL){
         sscanf(command , "%d %s" , &CommandHistory.commands[i].seqNumber , CommandHistory.commands[i].commandLine);
         i++;
      }
      fclose(fd);
   }

   //for(int i = 0; i < MAXHIST; i++){
   //   printf("%d %s\n" ,  CommandHistory.commands[i].seqNumber , CommandHistory.commands[i].commandLine);
   //}

   return 0;
}



// addToCommandHistory()
// - add a command line to the history list
// - overwrite oldest entry if buffer is full

void addToCommandHistory(char *cmdLine, int seqNo)
{
   char * dup = strdup(cmdLine);
   CommandHistory.commands[seqNo].seqNumber = seqNo;
   CommandHistory.commands[seqNo].commandLine = dup;
}

// showCommandHistory()
// - display the list of

void showCommandHistory(FILE *outf)
{
   for(int i = 0; i < MAXHIST; i++){
      fprintf(outf , "%d %s\n" , CommandHistory.commands[i].seqNumber , CommandHistory.commands[i].commandLine);
   }
}

// getCommandFromHistory()
// - get the command line for specified command
// - returns NULL if no command with this number

char *getCommandFromHistory(int cmdNo)
{
   // TODO
   char * s = "Hello";
   return s;
}

// saveCommandHistory()
// - write history to $HOME/.mymysh_history

void saveCommandHistory()
{
   char * PATH = getenv("HOME");
   char * full_path = malloc((strlen(PATH) + strlen(HISTFILE) + strlen("/")) * sizeof(char));
   strcpy(full_path , PATH);
   strcat(full_path , "/");
   strcat(full_path , HISTFILE);

   FILE * fd = fopen(full_path , "w");

   if(fd == NULL){
      printf("Error opening file\n");
   }

   showCommandHistory(fd);

   //printf("%s\n" , full_path);
   free(full_path);

   fclose(fd);
}

// cleanCommandHistory
// - release all data allocated to command history

void cleanCommandHistory()
{

}
