#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>

int FileDescriptor[2];
char **ArgumentsVector;
char Buffer[2048];

struct Entry {
    char *Name;
	unsigned int MicroSeconds;
} EntryTable[2048];

int TotalTime;
int EntryCount;

//================== Helper ====================

int CompareTime(const void *Left, const void *Right) {
	if (((struct Entry *)Left)->MicroSeconds < ((struct Entry *)Right)->MicroSeconds)
		return 1;
	else
		return -1;
}

//================ End Helper ==================

int main(int argc, char *argv[], char *envp[]) {
    int PIDAfterForked;
    int i;
	int NameEnd;
	int TimeStart;
	char AppearedBeforeFlag;
	int AppearedAt;
	int MicroSeconds;
	int RealBufferSize;
	int MaxLength;

	if (argc < 2) {
	    printf("usage: perf COMMAND ARGUMENTS\n");
		return 0;
	}

	if (pipe(FileDescriptor) != 0) {
	    // Error!
		printf("ERROR: Failed to create pipe.\n");
		return 0;
	}

	PIDAfterForked = fork();

	if (PIDAfterForked == 0) {
		// Child, execute strace command
        ArgumentsVector = (char**)malloc((argc + 2) * sizeof(char*));
		ArgumentsVector[0] = "strace";
		ArgumentsVector[1] = "-T";
		for (i = 1; i < argc + 1; i++) {
			ArgumentsVector[i + 1] = argv[i];
		}
		ArgumentsVector[argc + 1] = NULL; // Last one
        
		// Redirect strace's output to the pipe
		dup2(FileDescriptor[1], STDERR_FILENO);
  
		// Send program output to /dev/null
		int FDDevNull = open("/dev/null", O_WRONLY);
		if (FDDevNull == -1) { // Fail to open /dev/null
			printf("ERROR: Failed to send program output to /dev/null.\n");
		    return 0;
		}
		dup2(FDDevNull, STDOUT_FILENO);

		execvp("strace", ArgumentsVector);

		printf("ERROR: Failed to execute command %s.\n", ArgumentsVector[3]);
		return 0;
        // free(ArgumentsVector): Image replaced, original memory is cleared.
	}
	else {
		// Parent, read strace

		// Redirect strace to stdin
        dup2(FileDescriptor[0], STDIN_FILENO);
        
		EntryCount = 0;
		// Read a single line from strace
	    while (fgets(Buffer, 2048, stdin) != NULL) {
		    // Parse strace result
			// printf("DEBUG: Parsing\n%s\n", Buffer);
			//////////////DEBUG////////
			if (Buffer[0] == '+')
				break;
			/////////////END DEBUG////
			RealBufferSize = strlen(Buffer) - (Buffer[strlen(Buffer) - 1] == '\n' ? 1 : 0);
            if (Buffer[RealBufferSize - 1] == '>') { // CONTAINS time
			    // printf("DEBUG: Contains time\n");
				// Parse name
				NameEnd = 0;
				while (Buffer[NameEnd++] != '(');
				NameEnd -= 2;

				AppearedBeforeFlag = 0;
				AppearedAt = 0;
				for (i = 0; i < EntryCount && !AppearedBeforeFlag; i++) {
				    if (!strncmp(Buffer, EntryTable[i].Name, NameEnd + 1)) {
					    AppearedBeforeFlag = 1;
						AppearedAt = i;
					}
				}
                
				if (!AppearedBeforeFlag) {
				    EntryTable[EntryCount].Name = (char*)malloc((NameEnd + 2) * sizeof(char));
				    for (i = 0; i <= NameEnd; i++) {
					    EntryTable[EntryCount].Name[i] = Buffer[i];
				    }
					EntryTable[EntryCount].Name[NameEnd + 1] = '\0';
				    // printf("DEBUG: Name = %s\n", EntryTable[EntryCount].Name);
				}

				// Parse time
				TimeStart = strlen(Buffer) - 1;
				while (Buffer[TimeStart--] != '<');
				TimeStart += 2;

				MicroSeconds = 0;

				for (i = TimeStart; i < RealBufferSize - 1; i++) {
				    if (Buffer[i] >= '0' && Buffer[i] <= '9') {
					    MicroSeconds *= 10;
						MicroSeconds += (Buffer[i] - '0');
					}
				}

				if (AppearedBeforeFlag){
					EntryTable[AppearedAt].MicroSeconds += MicroSeconds;
				    // printf("DEBUG: Name = %s\n", EntryTable[AppearedAt].Name);
				}
				else
					EntryTable[EntryCount].MicroSeconds = MicroSeconds;

				// Increase EntryCount
				if (!AppearedBeforeFlag) {
				    // printf("DEBUG: Name = %s\n", EntryTable[EntryCount].Name);
					EntryCount += 1;
				}
			}
			// printf("DEBUG: Done.\n");
		}


        // Calculate and print time percentage
        // printf("DEBUG: Calculating and printing time percentage.\n");
		qsort(EntryTable, EntryCount, sizeof(struct Entry), CompareTime);
		TotalTime = 0;
		MaxLength = 0;
		for (i = 0; i < EntryCount; i++) {
		    TotalTime += EntryTable[i].MicroSeconds;
			MaxLength = (MaxLength >= strlen(EntryTable[i].Name) ? MaxLength : strlen(EntryTable[i].Name));
		}
		printf("Total time: %d ms\n%-*s Percent\tTime\n", TotalTime, MaxLength, "Name");
		for (i = 0; i < EntryCount; i++) {
			printf("%-*s %6.2f%%\t%d\n", MaxLength, EntryTable[i].Name, (double)EntryTable[i].MicroSeconds * 100 / TotalTime, EntryTable[i].MicroSeconds);
		}

		return 0;
	}

	return 0;
}
