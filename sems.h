#define MAX_SEMS 200

typedef struct semaphore semaphore;

struct semaphore {
    int     sid;
    int     count;
    procPtr blockedList;
    int     mutexID;
};
