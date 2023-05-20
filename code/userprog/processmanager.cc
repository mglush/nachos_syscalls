/*
 * ProcessManager implementation
 *
 * This class keeps track of the current PCBs and manages their creation and
 * deletion.
*/

#include "processmanager.h"
#include "utility.h" // definition of NULL
#include "system.h" // definition of processManagerLock

//-----------------------------------------------------------------------------
// ProcessManager::ProcessManager
//     Constructor that initializes the PCB list, condition list, lock list, 
//     and address space list to MAX_PROCESSES size.
//-----------------------------------------------------------------------------
ProcessManager::ProcessManager() : processesBitMap(MAX_PROCESSES) {

    pcbList = new PCB*[MAX_PROCESSES];
    conditionList = new Condition*[MAX_PROCESSES];
    lockList = new Lock*[MAX_PROCESSES];
    addrSpaceList = new AddrSpace*[MAX_PROCESSES];

    numAvailPIDs = MAX_PROCESSES;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        lockList[i] = NULL;
        conditionList[i] = NULL;
        pcbList[i] = NULL;
        addrSpaceList[i] = NULL;
    }
}

//-----------------------------------------------------------------------------
// ProcessManager::~ProcessManager
//     Destructor that frees up the memory allocated by the constructor.
//-----------------------------------------------------------------------------

ProcessManager::~ProcessManager() {
    delete [] pcbList;
    delete [] conditionList;
    delete [] lockList;
    delete [] addrSpaceList;
}

//-----------------------------------------------------------------------------
// ProcessManager::getPID
//     Returns the first free PID found.
//-----------------------------------------------------------------------------

int ProcessManager::getPID() {

    int newPID = processesBitMap.Find();
    processesWaitingOnPID[newPID] = 0;
    processesWaitingOnPID[newPID]++;
    return newPID;
}

//-----------------------------------------------------------------------------
// ProcessManager::clearPID
//     Decrements the number of processes waiting on this guy before its PID
//     can be reused, and frees the respective PID for re-use if possible.
//-----------------------------------------------------------------------------

void ProcessManager::clearPID(int pid) {

    processesWaitingOnPID[pid]--;
    if (processesWaitingOnPID[pid] == 0) {
        processesBitMap.Clear(pid);
    }
}

//-----------------------------------------------------------------------------
// ProcessManager::addProcess
//     Add a new process to the list.
//-----------------------------------------------------------------------------

void ProcessManager::addProcess(PCB* pcb, int pid) {
    pcbList[pid] = pcb;
}

//-----------------------------------------------------------------------------
// ProcessManager::join
//     Allows process A to wait on another process B in order to perform a join
//     system call.
//-----------------------------------------------------------------------------

void ProcessManager::join(int pid) {

    Lock* lockForOtherProcess = lockList[pid];
    if (lockForOtherProcess == NULL) {
        lockForOtherProcess = new Lock("");
        lockList[pid] = lockForOtherProcess;
    }

    Condition* conditionForOtherProcess = conditionList[pid];
    if (conditionForOtherProcess == NULL) {
        conditionForOtherProcess = new Condition("");
        conditionList[pid] = conditionForOtherProcess;
    }

    lockForOtherProcess->Acquire();
    //BEGIN HINTS
    
    //Increment  processesWaitingOnPID[pid].
    processesWaitingOnPID[pid]++;
    
    //Conditional waiting on conditionForOtherProcess
    fprintf(stderr, "PROCESS %d ENTERING WAIT LOOP WITH STATUS %d\n", pid, getStatus(pid));
    currentThread->space->getPCB()->status = P_BLOCKED;
    if (pid != 0) {
        while (getStatus(pid) != -1) {
            fprintf(stderr, "PROCESS %d INSIDE OF LOOP WITH STATUS %d\n", pid, getStatus(pid));
            conditionForOtherProcess->Wait(lockForOtherProcess);
        }
    }
    currentThread->space->getPCB()->status = P_RUNNING;
    fprintf(stderr, "PROCESS %d EXITING WAIT LOOP WITH STATUS %d\n", pid, getStatus(pid));

    //Decrement   processesWaitingOnPID[pid].
    processesWaitingOnPID[pid]--;

    //END HINTS
    
    if (processesWaitingOnPID[pid] == 0) {
        processesBitMap.Clear(pid);
    }
    lockForOtherProcess->Release();
}

//-----------------------------------------------------------------------------
// ProcessManager::broadcast
//     Lets everyone know that the process has changed status so that other 
//     processes can act accordingly if they are waiting.
//-----------------------------------------------------------------------------

void ProcessManager::broadcast(int pid) {

    Lock* lock = lockList[pid]; //This line is needed when using a lock specific for pid
    Condition* condition = conditionList[pid];
    pcbStatuses[pid] = pcbList[pid]->status;
    fprintf(stderr, "PROCESS %d CALLED BROADCAST WITH STATUS %d\n", pid, getStatus(pid));
    if (condition != NULL) { // somebody is waiting on this process
        // BEGIN HINTS

        // Wake up others
        fprintf(stderr, "WE ACTUALLY BROADCAST, there are %d processes waiting on process %d\n", processesWaitingOnPID[pid], pid);
        condition->Broadcast(lock);
        fprintf(stderr, "WE ACTUALLY BROADCAST (after)\n");

        // END HINTS
    }
}

//-----------------------------------------------------------------------------
// ProcessManager::getStatus
//     Returns the status of a given process
//-----------------------------------------------------------------------------

int ProcessManager::getStatus(int pid) {
    if (processesBitMap.Test(pid) == 0) {
        return -1; // process finished
    }
    return pcbStatuses[pid];
}
