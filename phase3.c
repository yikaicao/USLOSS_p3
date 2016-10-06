#include <stdlib.h>
#include <string.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <sems.h>
#include <usyscall.h>

/*---------- Global Variables ----------*/
int debugflag3 = 0;
procStruct  ProcTable[MAXPROC];
semaphore   SemTable[MAX_SEMS];

/*---------- Function Prototypes ----------*/
extern int start3(char*);

void check_kernel_mode(char*);

void nullsys3();

void initSysCallVec();
void initProcTable();
void initSemTable();

//void spawnReal(char*, int (*func)(char*), char*, unsigned int, int);
//void waitReal(int*);

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

    
    return -1; // should not get here
} /* start2 */



/*---------- check_kernel_mode ----------*/
void check_kernel_mode(char *arg)
{
    if (!(USLOSS_PSR_CURRENT_MODE & USLOSS_PsrGet()))
    {
        USLOSS_Console("%s(): called while in user mode. Halting...\n", arg);
    }
} /* check_kernel_mode */


/*---------- nullsys3 ----------*/
void nullsys3()
{
    USLOSS_Console("nullsys3(): called. Halting..\n");
    //TODO: should terminate current process instead of halting
    USLOSS_Halt(1);
    
} /* nullsys3 */


/*---------- initSysCallVec ----------*/
void initSysCallVec()
{
    if (debugflag3)
        USLOSS_Console("initSysCallVec(): entered");
    
    int i;
    for (i = 0; i < USLOSS_MAX_SYSCALLS; i++)
        systemCallVec[i] = nullsys3;
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
            .privateMBoxID  = -1,
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
            .blockedList    = NULL
        };
    }
    
} /* initSemTable */
