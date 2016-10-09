/*
 * These are the definitions for phase 3 of the project
 */

#ifndef _PHASE3_H
#define _PHASE3_H

#define MAXSEMS         200

#endif /* _PHASE3_H */

typedef struct procStruct procStruct;
typedef struct procStruct *procPtr;

struct procStruct{
    int         pid;
    int         priority;
    char        name[MAXNAME];
    char        startArg[MAXARG];
    int         (* startFunc) (char *);
    unsigned int stackSize;
    procPtr     nextProcPtr; // to keep track of next blocked process waiting for a semaphore
    procPtr     childProcPtr; // used in terminate
    procPtr     nextSiblingPtr; // used in terminate
    int         privateMboxID; // used in self blocked
    int         parentPID; // used in terminate
};
