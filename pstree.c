/***************************************************
 * 
 * m     m   mm   mmmmm  mm   m mmmmm  mm   m   mmm 
 * #  #  #   ##   #   "# #"m  #   #    #"m  # m"   "
 * " #"# #  #  #  #mmmm" # #m #   #    # #m # #   mm
 *  ## ##"  #mm#  #   "m #  # #   #    #  # # #    #
 *  #   #  #    # #    " #   ## mm#mm  #   ##  "mmm"
 * 
 * GIT ISSUE! THIS PROJECT IS NOT BUILT FROM THE 
 * PROVIDED OSLAB PROJECT DUE TO THE EARLY SET-OUT.
 * YOU MAY FIND GIT LOG AT THE ORIGINAL PROJECT
 * NAMED NSPSTREE AT
 *     https://github.com/NSKernel/nspstree
 * 
 ***************************************************/


/*==================================================
  NS PSTREE Command
  Copyright (C) 2018 NSKernel. All rights reserved.
  =================================================
  Abstract:
 
  A mini clone of the Linux command pstree.
  =================================================
  Author:
 
  NSKernel
 ==================================================*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>



//=================== Helper ===================

int ReadLine(FILE *fp, char s[], int lim){
    int c, i;
    i = 0;
    while((c = fgetc(fp)) != EOF && c != '\n' && i < lim - 1)
        s[i++] = c;
    s[i] = '\0';
    return i;
}

//================= End Helper =================


#define MAX_THREAD_COUNT 2048

typedef struct TProcessInfo {
    unsigned int TGID;
    unsigned int PID;
    unsigned int PPID;
    char ProcessName[200];
    struct TProcessInfo *ChildProcess;
    struct TProcessInfo *BroProcess;
    char IsFirstChild;
    char IsThreadGroup;
    char IsVisited;
} ProcessInfo;

char PrintPIDFlag;
char NumericSortFlag;
char PrintKernelThreadsFlag;
char DoNotDoJobFlag;

struct dirent **NameList;
struct dirent **TaskNameList;
int ProcessCount;

char TreePrefix[1000];

char FilterNeq[100];

ProcessInfo ProcessInfoPool[MAX_THREAD_COUNT];
ProcessInfo Root;

/// <summary>
/// Use with scandir to filter out process directory.
/// </summary>
/// <param name="Directory"> The dirent object pointer </param>
int Filter(const struct dirent *Directory) {
    int i;
    if (!strcmp(Directory->d_name, FilterNeq))
        return 0;
    for (i = 0; i < strlen(Directory->d_name); i++) {
        if(Directory->d_name[i] < '0' || Directory->d_name[i] > '9')
            return 0;
    }
    return 1;
}

void PrintNode(ProcessInfo *CurrentNode) {
    int LastTreePrefixLength = strlen(TreePrefix);
    char RBQBuffer[100];
    int i;
    if (PrintPIDFlag) {
        if (CurrentNode->IsThreadGroup)
            sprintf(RBQBuffer, "{%s}(%d)", CurrentNode->ProcessName, CurrentNode->PID);
        else
            sprintf(RBQBuffer, "%s(%d)", CurrentNode->ProcessName, CurrentNode->PID);
    }
    else {
        if (CurrentNode->IsThreadGroup)
            sprintf(RBQBuffer, "{%s}", CurrentNode->ProcessName);
        else
            sprintf(RBQBuffer, "%s", CurrentNode->ProcessName);
    }
    if (CurrentNode->IsFirstChild) {
        if (CurrentNode->BroProcess == NULL) {
            printf("──%s", RBQBuffer);
            if (CurrentNode->ChildProcess != NULL) {
                printf("─");
                for (i = LastTreePrefixLength; i < LastTreePrefixLength + strlen(RBQBuffer) + 3; i++) {
                    TreePrefix[i] = ' ';
                }
                TreePrefix[i] = '\0';
                PrintNode(CurrentNode->ChildProcess);
                TreePrefix[LastTreePrefixLength] = '\0';
            }
            else
                printf("\n");
        }
        else {
            printf("┬─%s", RBQBuffer);
            if (CurrentNode->ChildProcess != NULL) {
                printf("─");
                strcat(TreePrefix, "│");
                for (i = LastTreePrefixLength + 3; i < LastTreePrefixLength + strlen(RBQBuffer) + 5; i++) {
                    TreePrefix[i] = ' ';
                }
                TreePrefix[i] = '\0';
                PrintNode(CurrentNode->ChildProcess);
                TreePrefix[LastTreePrefixLength] = '\0';
            }
            else
                printf("\n");
            
            
            PrintNode(CurrentNode->BroProcess);
        }
    }
    else {
        printf("%s", TreePrefix);
        if (CurrentNode->BroProcess == NULL) {
            printf("└─%s", RBQBuffer);
            if (CurrentNode->ChildProcess != NULL) {
                printf("─");
                for (i = LastTreePrefixLength; i < LastTreePrefixLength + strlen(RBQBuffer) + 3; i++) {
                    TreePrefix[i] = ' ';
                }
                TreePrefix[i] = '\0';
                PrintNode(CurrentNode->ChildProcess);
                TreePrefix[LastTreePrefixLength] = '\0';
            }
            else
                printf("\n");
        }
        else {
            printf("├─%s", RBQBuffer);
            if (CurrentNode->ChildProcess != NULL) {
                printf("─");
                strcat(TreePrefix, "│");
                for (i = LastTreePrefixLength + 3; i < LastTreePrefixLength + strlen(RBQBuffer) + 5; i++) {
                    TreePrefix[i] = ' ';
                }
                TreePrefix[i] = '\0';
                PrintNode(CurrentNode->ChildProcess);
                TreePrefix[LastTreePrefixLength] = '\0';
            }
            else
                printf("\n");
            PrintNode(CurrentNode->BroProcess);
        }
    }
}

void PrintTree() {
    TreePrefix[0] = '\0'; // TreePrefix = ""
    if (Root.ChildProcess != NULL) {
        PrintNode(Root.ChildProcess);
    }
}

void BuildNode(ProcessInfo *ParentNode) {
    int ProcessIterator;
    ProcessInfo *ChildProcessIterator;
    for (ProcessIterator = 0; ProcessIterator < ProcessCount; ProcessIterator++) {
        if (!PrintKernelThreadsFlag && ProcessInfoPool[ProcessIterator].PID == 2)
            continue;
        if (ProcessInfoPool[ProcessIterator].IsVisited)
            continue;
        if(ProcessInfoPool[ProcessIterator].PPID == ParentNode->PID) {
            if (NumericSortFlag) {
                ChildProcessIterator = ParentNode->ChildProcess;
                if (ChildProcessIterator == NULL) {
                    ProcessInfoPool[ProcessIterator].IsFirstChild = 1;
                    ProcessInfoPool[ProcessIterator].BroProcess = NULL;
                    ParentNode->ChildProcess = &(ProcessInfoPool[ProcessIterator]);
                }
                while(ChildProcessIterator != NULL) {
                    if (ProcessInfoPool[ProcessIterator].PID > ChildProcessIterator->PID) {
                        if(ChildProcessIterator->BroProcess != NULL) {
                            if (ProcessInfoPool[ProcessIterator].PID <= ChildProcessIterator->BroProcess->PID) {
                                ProcessInfoPool[ProcessIterator].BroProcess = ChildProcessIterator->BroProcess;
                                ChildProcessIterator->BroProcess = &(ProcessInfoPool[ProcessIterator]);
                                break;
                            }
                            else {
                                ChildProcessIterator = ChildProcessIterator->BroProcess;
                                continue;
                            }
                        }
                        else {
                            ChildProcessIterator->BroProcess = &(ProcessInfoPool[ProcessIterator]);
                            break;
                        }
                    }
                    else {
                        ProcessInfoPool[ProcessIterator].IsFirstChild = 1;
                        ProcessInfoPool[ProcessIterator].BroProcess = ChildProcessIterator;
                        ChildProcessIterator->IsFirstChild = 0;
                        ParentNode->ChildProcess = &(ProcessInfoPool[ProcessIterator]);
                        break;
                    }
                }
                BuildNode(&(ProcessInfoPool[ProcessIterator]));
            }
            else {
                ProcessInfoPool[ProcessIterator].BroProcess = ParentNode->ChildProcess;
                if (ProcessInfoPool[ProcessIterator].BroProcess != NULL)
                    ProcessInfoPool[ProcessIterator].BroProcess->IsFirstChild = 0;
                ProcessInfoPool[ProcessIterator].IsFirstChild = 1;
                ParentNode->ChildProcess = &(ProcessInfoPool[ProcessIterator]);
                BuildNode(ParentNode->ChildProcess);
            }
            ProcessInfoPool[ProcessIterator].IsVisited = 1;
        } 
    }
}

void BuildTree() {
    //int ProcessIterator;
    
    //for(ProcessIterator = 0; ProcessIterator < ProcessCount; ProcessIterator++)
    //    printf("Tgid: %d, Pid: %d, PPid: %d, Name: %s\n", ProcessInfoPool[ProcessIterator].TGID,
    //    ProcessInfoPool[ProcessIterator].PID,ProcessInfoPool[ProcessIterator].PPID, ProcessInfoPool[ProcessIterator].ProcessName);
    
    Root.TGID = 0;
    Root.PID = 0;
    Root.PPID = 0;
    Root.IsThreadGroup = 0;
    Root.ChildProcess = NULL;
    Root.BroProcess = NULL;
    BuildNode(&Root);
}

void ScanTask(int TaskNameListCount, int *ProcessCountIterator) {
    int Start, End, Length, TaskNameListIterator;
    char RBQPath[100];
    char RBQBuffer[200];
    char RBQIDBuffer[200];
    FILE *FangPi; // FNMDP
    for (TaskNameListIterator = 0; TaskNameListIterator < TaskNameListCount; (*ProcessCountIterator)++, TaskNameListIterator++) {
        strcpy(RBQPath, "/proc/");
        strcat(RBQPath, (TaskNameList[TaskNameListIterator])->d_name);
        strcat(RBQPath, "/status");

        FangPi = fopen(RBQPath, "r");
        ProcessInfoPool[*ProcessCountIterator].ChildProcess = NULL;
        ProcessInfoPool[*ProcessCountIterator].BroProcess = NULL;
        ProcessInfoPool[*ProcessCountIterator].IsFirstChild = 0;
        ProcessInfoPool[*ProcessCountIterator].IsThreadGroup = 0;
        ProcessInfoPool[*ProcessCountIterator].IsVisited = 0;
        while (!feof(FangPi)) {
            ReadLine(FangPi, RBQBuffer, 200);
            if (strncmp(RBQBuffer, "Pid", 3) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("Pid:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                ProcessInfoPool[*ProcessCountIterator].PID = atoi(RBQIDBuffer);
            }
            else if (strncmp(RBQBuffer, "PPid", 4) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("PPid:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                ProcessInfoPool[*ProcessCountIterator].PPID = atoi(RBQIDBuffer);
            }
            else if (strncmp(RBQBuffer, "Name", 4) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("Name:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                strcpy(ProcessInfoPool[*ProcessCountIterator].ProcessName, RBQIDBuffer);
            }
            else if (strncmp(RBQBuffer, "Tgid", 3) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("Tgid:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                ProcessInfoPool[*ProcessCountIterator].TGID = atoi(RBQIDBuffer);
            }
        }
        if(ProcessInfoPool[*ProcessCountIterator].TGID != ProcessInfoPool[*ProcessCountIterator].PID) {
            ProcessInfoPool[*ProcessCountIterator].IsThreadGroup = 1;
            ProcessInfoPool[*ProcessCountIterator].PPID = ProcessInfoPool[*ProcessCountIterator].TGID;
        }
    }
}

void ScanProcess() {
    int ProcessCountIterator, Start, End, Length, NameListIterator, TaskNameListCount;
    char RBQPath[100];
    char RBQTaskPath[100];
    char RBQBuffer[200];
    char RBQIDBuffer[200];
    
    FILE *FangPi; // FNMDP
    for (ProcessCountIterator = 0, NameListIterator = 0; NameListIterator < ProcessCount; ProcessCountIterator++, NameListIterator++) {
        strcpy(RBQPath, "/proc/");
        strcat(RBQPath, NameList[NameListIterator]->d_name);
        strcat(RBQPath, "/status");
        
        strcpy(FilterNeq, NameList[NameListIterator]->d_name);
        strcpy(RBQTaskPath, "/proc/");
        strcat(RBQTaskPath, NameList[NameListIterator]->d_name);
        strcat(RBQTaskPath, "/task");
        TaskNameListCount = scandir(RBQTaskPath, &TaskNameList, Filter, alphasort);
        if(TaskNameListCount != 0) {
            ScanTask(TaskNameListCount, &ProcessCountIterator);
            free(TaskNameList);
        }

        FangPi = fopen(RBQPath, "r");
        ProcessInfoPool[ProcessCountIterator].ChildProcess = NULL;
        ProcessInfoPool[ProcessCountIterator].BroProcess = NULL;
        ProcessInfoPool[ProcessCountIterator].IsFirstChild = 0;
        ProcessInfoPool[ProcessCountIterator].IsThreadGroup = 0;
        ProcessInfoPool[ProcessCountIterator].IsVisited = 0;
        while (!feof(FangPi)) {
            ReadLine(FangPi, RBQBuffer, 200);
            if (strncmp(RBQBuffer, "Pid", 3) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("Pid:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                ProcessInfoPool[ProcessCountIterator].PID = atoi(RBQIDBuffer);
            }
            else if (strncmp(RBQBuffer, "PPid", 4) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("PPid:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                ProcessInfoPool[ProcessCountIterator].PPID = atoi(RBQIDBuffer);
            }
            else if (strncmp(RBQBuffer, "Name", 4) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("Name:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                strcpy(ProcessInfoPool[ProcessCountIterator].ProcessName, RBQIDBuffer);
            }
            else if (strncmp(RBQBuffer, "Tgid", 3) == 0) {
                Length = strlen(RBQBuffer) + 1; // Include '\0'
                Start = strlen("Tgid:\t");
                for (End = 0; End < Length - Start; End++) {
                    RBQIDBuffer[End] = RBQBuffer[Start + End];
                }
                ProcessInfoPool[ProcessCountIterator].TGID = atoi(RBQIDBuffer);
            }
        }
    }
    ProcessCount = ProcessCountIterator;
}

int main(int argc, char **argv) {
    int i;
    PrintPIDFlag = 0;
    NumericSortFlag = 0;
    PrintKernelThreadsFlag = 0;
    DoNotDoJobFlag = 0;
    FilterNeq[0] = '\0';
    for (i = 1; i < argc; i++){
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--show-pids") == 0) {
            PrintPIDFlag = 1;
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--numeric-sort") == 0) {
            NumericSortFlag = 1;
        }
        else if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--kernel-thread") == 0) {
            PrintKernelThreadsFlag = 1;
        }
        else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("NS PSTREE Command [Version 0.1]\nCopyright (C) 2018 NSKernel. All rights reserved.\n");
            DoNotDoJobFlag = 1;
        }
        else {
            printf("Unknown argument: %s\n", argv[i]);
            printf("Usage:\n  -p, --show-pids:\tShow PIDs.\n  -n, --numeric-sort:\tNumeric sort.\n  -k, --kernel-thread:\tShow kernel threads.\n  -V, --version:\tVersion.\n");
            DoNotDoJobFlag = 1;
        }
    }
    if (!DoNotDoJobFlag) {
        ProcessCount = scandir("/proc", &NameList, Filter, alphasort);

        if (ProcessCount < 0)
            perror("FATAL: Procedure scandir failed.\n");
        else {
            ScanProcess();
            BuildTree();
            PrintTree();
            free(NameList);
        }
    }
    return 0;
}

//printf("%lu, %lu\n", sizeof("│"), sizeof("1"));
//ProcessInfoPool[2].PID = 1;
//ProcessInfoPool[2].PPID = 0;
//ProcessInfoPool[2].TGID = 1;
//strcpy(ProcessInfoPool[2].ProcessName, "systemd");
//ProcessInfoPool[2].IsThreadGroup = 0;
//ProcessInfoPool[2].IsFirstChild = 0;
//ProcessInfoPool[2].ChildProcess = NULL;
//ProcessInfoPool[2].BroProcess = NULL;
//
//ProcessInfoPool[3].PID = 2;
//ProcessInfoPool[3].PPID = 1;
//ProcessInfoPool[3].TGID = 2;
//strcpy(ProcessInfoPool[3].ProcessName, "hello");
//ProcessInfoPool[3].IsThreadGroup = 0;
//ProcessInfoPool[3].IsFirstChild = 0;
//ProcessInfoPool[3].ChildProcess = NULL;
//ProcessInfoPool[3].BroProcess = NULL;
//
//ProcessInfoPool[4].PID = 3;
//ProcessInfoPool[4].PPID = 2;
//ProcessInfoPool[4].TGID = 3;
//strcpy(ProcessInfoPool[4].ProcessName, "shit");
//ProcessInfoPool[4].IsThreadGroup = 0;
//ProcessInfoPool[4].IsFirstChild = 0;
//ProcessInfoPool[4].ChildProcess = NULL;
//ProcessInfoPool[4].BroProcess = NULL;
//
//ProcessInfoPool[1].PID = 4;
//ProcessInfoPool[1].PPID = 2;
//ProcessInfoPool[1].TGID = 4;
//strcpy(ProcessInfoPool[1].ProcessName, "fuck");
//ProcessInfoPool[1].IsThreadGroup = 0;
//ProcessInfoPool[1].IsFirstChild = 0;
//ProcessInfoPool[1].ChildProcess = NULL;
//ProcessInfoPool[1].BroProcess = NULL;
//
//ProcessInfoPool[5].PID = 6;
//ProcessInfoPool[5].PPID = 2;
//ProcessInfoPool[5].TGID = 6;
//strcpy(ProcessInfoPool[5].ProcessName, "toe");
//ProcessInfoPool[5].IsThreadGroup = 0;
//ProcessInfoPool[5].IsFirstChild = 0;
//ProcessInfoPool[5].ChildProcess = NULL;
//ProcessInfoPool[5].BroProcess = NULL;
//
//ProcessInfoPool[0].PID = 5;
//ProcessInfoPool[0].PPID = 1;
//ProcessInfoPool[0].TGID = 5;
//strcpy(ProcessInfoPool[0].ProcessName, "tooo");
//ProcessInfoPool[0].IsThreadGroup = 0;
//ProcessInfoPool[0].IsFirstChild = 0;
//ProcessInfoPool[0].ChildProcess = NULL;
//ProcessInfoPool[0].BroProcess = NULL;
//
//ProcessCount = 6;

