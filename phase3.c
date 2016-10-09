#include <stdlib.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <sems.h>
#include <libuser.h>
#include <usyscall.h>


#define MINPRIORITY 5
#define MAXPRIORITY 1

/*---------- Global Variables ----------*/
int debugflag3 = 0;
procStruct  ProcTable[MAXPROC];
semaphore   SemTable[MAX_SEMS];

/*---------- Function Prototypes ----------*/
extern int start3(char*);

void check_kernel_mode(char*);
void setUserMode();
void nullsys3();

void initSysCallVec();
void initProcTable();
void initSemTable();

void spawn(systemArgs*);
int spawnReal(char*, int(*func)(char*), char*, unsigned int, int);
void spawnLaunch();

void wait(systemArgs*);
int waitReal(int*);

void terminate(systemArgs*);
void terminateReal(int);

void semCreate(systemArgs*);
int semCreateReal(int);

int start2(char *arg)
{
    int pid;
    int status;
    
    /*
     * Check kernel mode here.
     */
    check_kernel_mode("start2");

    /*
     * Data structure initialization as needed...
     */
    initSysCallVec();
    initProcTable();
    initSemTable();

    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscallHandler; spawnReal is the function that
     * contains the implementation and is called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes USLOSS_Syscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawnReal().
     *
     * Here, we only call spawnReal(), since we are already in kernel mode.
     *
     * spawnReal() will create the process by using a call to fork1 to
     * create a process executing the code in spawnLaunch().  spawnReal()
     * and spawnLaunch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawnReal() will
     * return to the original caller of Spawn, while spawnLaunch() will
     * begin executing the function passed to Spawn. spawnLaunch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawnReal() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */
    pid = spawnReal("start3", start3, NULL, USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
    pid = waitReal(&status);

    
    return pid; // should not get here
} /* start2 */


/* ------------------------------------------------------------------------
    Name - spawn
    Purpose - Extract the initial process info and call spawnReal.
    Parameters - A systemArgs that contains information sent from user mode process.
    Returns - None.
    Side Effects - Many.
 ----------------------------------------------------------------------- */
void spawn(systemArgs *sysArg)
{
    if(debugflag3)
        USLOSS_Console("spawn(): entered\n");
    
    // extract sysArg
    int (*func)(char *) = sysArg->arg1;
    char *arg = sysArg->arg2;
    int stackSize = (long) sysArg->arg3;
    int priority = (long) sysArg->arg4;
    char *name = sysArg->arg5;
    
    // check value
    if (func == NULL ||
        stackSize < USLOSS_MIN_STACK ||
        priority > MINPRIORITY ||
        priority < MAXPRIORITY)
    {
        sysArg->arg1 = (void*) ((long)-1);
        return;
    }
    
    // call spawnReal
    int kidpid = spawnReal(name, func, arg, stackSize, priority);
    
    // encode sysArg
    sysArg->arg1 = (void*) ((long)kidpid);
    
    // if zapped terminate itself
    
    // set to user mode
    setUserMode();
    return;
} /* spawn */


/* ------------------------------------------------------------------------
    Name - spawnReal
    Purpose - Create a new child for a user process.
    Parameters - Same set of parameters that will be used in fork1.
    Returns - Process ID of the new spawned child.
    Side Effects - Processs table gets updated.
 ----------------------------------------------------------------------- */
int spawnReal(char* name, int (*func)(char*), char *arg, unsigned int stackSize, int priority)
{
    if (debugflag3)
        USLOSS_Console("spawnReal(): entered\n");
    
    // fork1 new process
    int kidpid = fork1(name, (void *)spawnLaunch, arg, stackSize, priority);
    
    // update process table
    ProcTable[kidpid % MAXPROC].pid         = kidpid;
    ProcTable[kidpid % MAXPROC].startFunc   = func;
    ProcTable[kidpid % MAXPROC].priority    = priority;
    if (arg != NULL)
    {
        memcpy(ProcTable[kidpid%MAXPROC].startArg, arg, strlen(arg));
    }
    /* end of updating process table */
    
    // synchronize with child
    MboxSend(ProcTable[kidpid % MAXPROC].privateMboxID, NULL, 0);
    
    return kidpid;
} /* spawnReal */


/* ------------------------------------------------------------------------
    Name - spawnLaunch
    Purpose - Launch the newly spawned process.
    Parameters - None.
    Returns - None.
    Side Effects - New process gets to run.
 ----------------------------------------------------------------------- */
void spawnLaunch()
{
    if (debugflag3)
        USLOSS_Console("spawnLaunch(): entered\n");
    
    int result;
    int curpid = getpid();
    
    // synchronize with parent
    MboxReceive(ProcTable[curpid % MAXPROC].privateMboxID, NULL, 0);
    
    // launch current process
    setUserMode();
    result = ProcTable[curpid % MAXPROC].startFunc(ProcTable[curpid % MAXPROC].startArg);
    
    // calling upper case Terminate because we are in user mode
    Terminate(result);
    
} /* spawnLaunch */


/* ------------------------------------------------------------------------
    Name - wait
    Purpose - Call waitReal and then update sysArgs
    Parameter - sysArg
    Return - None.
    Side effects - sysArg gets updated.
 ----------------------------------------------------------------------- */
void wait(systemArgs *sysArg)
{
    if (debugflag3)
        USLOSS_Console("wait(): entered\n");
    
    // call waitReal
    int quitStatus;
    int quitpid = waitReal(&quitStatus);
    
    // encode sysArgs
    sysArg->arg1 = (void*) ((long)quitpid);
    sysArg->arg2 = (void*) ((long)quitStatus);
    
    // set to user mode
    setUserMode();
    
} /* wait */


/* ------------------------------------------------------------------------
    Name - waitReal
    Purpose - Equivalent to join in phase1.
    Parameters - A status that indicate the quit status of quit child process.
    Returns - Returns the the process id of the quitted child.
    Side Effects - None.
 ----------------------------------------------------------------------- */
int waitReal(int *status)
{
    if (debugflag3)
        USLOSS_Console("waitReal(): entered\n");
    
    int quitpid = join(status);
    return quitpid;
} /* waitReal */


/* ------------------------------------------------------------------------
    Name - terminate
    Purpose - Pass status's address to terminate real and call it.
    Parameters - A sysArg.
    Returns - None.
    Side Effects - sysArg gets updated.
 ----------------------------------------------------------------------- */
void terminate(systemArgs *sysArg)
{
    if (debugflag3)
        USLOSS_Console("terminate(): entered\n");
    
    // call terminateReal
    terminateReal((long)sysArg->arg1);
    setUserMode();
} /* terminate */


/* ------------------------------------------------------------------------
    Name - terminateReal
    Purpose - Equivalent to quit in phase1.
    Parameters - Adress to status.
    Returns - None.
    Side Effects - sysArg gets updated.
 ----------------------------------------------------------------------- */
void terminateReal(int status)
{
    if (debugflag3)
        USLOSS_Console("terminateReal(): entered\n");
    
    quit(status);
} /* terminateReal */


/* ------------------------------------------------------------------------
    Name - semCreate
    Purpose - Check value and call semCreateReal.
    Parameters - A sysArg.
    Returns - None.
    Side Effects - New semaphore gets created.
 ----------------------------------------------------------------------- */
void semCreate(systemArgs *sysArg)
{
    if (debugflag3)
        USLOSS_Console("semCreate(): entered\n");
    
    // extract value from sysArg
    int initValue = (long)sysArg->arg1;
    
    // check value
    if (initValue < 0)
    {
        sysArg->arg4 = (void*) (long) -2;
        return;
    }
    
    // call semCreateReal
    int semID = semCreateReal(initValue);
    
    // check full sem table
    if (semID == MAX_SEMS)
    {
        sysArg->arg4 = (void*) (long) -1;
    }
    // encode sysArg
    else
    {
        sysArg->arg1 = (void*)(long) semID;
        sysArg->arg4 = (void*)(long) 0;
    }
    
    // set to user mode
    setUserMode();
} /* semCreate */



/* ------------------------------------------------------------------------
    Name - semCreateReal
    Purpose - Really create a semaphore.
    Parameters - Initvalue.
    Returns - ID of new created semaphore.
    Side Effects - New semaphore gets created.
 ----------------------------------------------------------------------- */
int semCreateReal(int initValue)
{
    if (debugflag3)
        USLOSS_Console("semCreateReal(): entered\n");
    
    // find emplty slot in sem table
    int semID = 0;
    while(semID < MAX_SEMS && SemTable[semID].sid != -1 )
    {
        semID++;
    }

    // check full sem table
    if (semID == MAX_SEMS)
    {
        return semID;
    }
    
    // assign mutex mailbox
    int mutexID = MboxCreate(1, 0);
    
    // update sem table
    SemTable[semID].sid     = semID;
    SemTable[semID].count   = initValue;
    SemTable[semID].mutexID = mutexID;
    
    return SemTable[semID].sid;
    
} /* semCreateReal */


/*---------- check_kernel_mode ----------*/
void check_kernel_mode(char *arg)
{
    if (!(USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()))
        USLOSS_Console("%s(): called while in user mode. Halting...\n", arg);
} /* check_kernel_mode */


/*---------- setUserMode ----------*/
void setUserMode(){
    if(debugflag3)
        USLOSS_Console("setUserMode(): entered\n");
    
    // USLOSS_PsrGet() 'AND' binary number '1111110' so that last bit is set to be 0
    USLOSS_PsrSet( USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE );
} /* setUserMode */


/*---------- nullsys3 ----------*/
void nullsys3()
{
    USLOSS_Console("nullsys3(): called. Terminating current process..\n");
    terminateReal(-1);
    //USLOSS_Halt(1);
    
} /* nullsys3 */


/*---------- initSysCallVec ----------*/
void initSysCallVec()
{
    if (debugflag3)
        USLOSS_Console("initSysCallVec(): entered");
    
    // assign nullsys3 to all other vectors
    int i;
    for (i = 0; i < USLOSS_MAX_SYSCALLS; i++)
        systemCallVec[i] = nullsys3;
    
    // known vectors
    systemCallVec[SYS_SPAWN] = (void *)spawn;
    systemCallVec[SYS_WAIT] = (void *)wait;
    systemCallVec[SYS_TERMINATE] = (void *)terminate;
    systemCallVec[SYS_SEMCREATE] = (void *)semCreate;
    
    
} /* initSysCallVec */


/*---------- initProcTable ----------*/
void initProcTable()
{
    if (debugflag3)
        USLOSS_Console("initProcTable(): entered");
    
    int i;
    for (i = 0; i < MAXPROC; i++)
    {
        ProcTable[i] = (procStruct) {
            .pid            = -1,
            .priority       = -1,
            // name and startArg is initialized later
            .startFunc      = NULL,
            .stackSize      = -1,
            .nextProcPtr    = NULL,
            .childProcPtr   = NULL,
            .nextSiblingPtr = NULL,
            .privateMboxID  = MboxCreate(0,0),
            .parentPID      = -1
        };
        // name and startArg is initialized here
        memset(ProcTable[i].name, 0, sizeof(char)*MAXNAME);
        memset(ProcTable[i].startArg, 0, sizeof(char)*MAXARG);
    }
} /* initProcTable */


/*---------- initSemTable ----------*/
void initSemTable()
{
    if (debugflag3)
        USLOSS_Console("initSemTable(): entered");
    
    int i;
    for (i = 0; i < MAX_SEMS; i++)
    {
        SemTable[i] = (semaphore) {
            .sid            = -1,
            .count          = -1,
            .blockedList    = NULL,
            .mutexID        = -1
        };
    }
    
} /* initSemTable */

