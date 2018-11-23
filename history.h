// COMP1521 18s2 mymysh ... command history
// Implements an interface to an abstract data object

#include <stdio.h>

// Functions on the Command History object

typedef struct _history_list * HistoryL;
//typedef struct _history_entry * HistoryE;


int initCommandHistory();
void addToCommandHistory(char *cmdLine, int seqNo);
void showCommandHistory(FILE *histFile);
char *getCommandFromHistory(int cmdNo);
void saveCommandHistory();
void cleanCommandHistory();
void showCommandList();
