#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

char RBQCodeBuffer[100000];
char RBQExecBuffer[100000];
char RBQSymbol[100];

int main() {
	void *handle;
	char *error;
	int (*ExecFunction)();
	FILE *FangPi;
	unsigned int ExecId = 0;

    FangPi = fopen("/tmp/crepltmp.c", "w"); // create and empty the file
	fclose(FangPi);

	
	while (1) {
        if (fgets(RBQCodeBuffer, sizeof(RBQCodeBuffer), stdin) != NULL) {
		    if (RBQCodeBuffer[0] == 'i' && RBQCodeBuffer[1] == 'n' && RBQCodeBuffer[2] == 't') {
				// define function
				FangPi = fopen("/tmp/crepltmptest.c", "w");
				if (FangPi == NULL) {
				    printf("ERROR: Cannot open temporary file.\n");
					return 1;
				}

				fprintf(FangPi, "%s", RBQCodeBuffer);
			    fclose(FangPi);

				if (system("gcc -shared -fPIC /tmp/crepltmptest.c -o /tmp/crepltmptest.so -ldl") != 0) {
					printf("ERROR: Function definition contains error(s).\n");
                    continue;
				}

				FangPi = fopen("/tmp/crepltmp.c", "a+");
				if (FangPi == NULL) {
				    printf("ERROR: Cannot open temporary file.\n");
					return 1;
				}

				fprintf(FangPi, "%s", RBQCodeBuffer);
			    fclose(FangPi);
			}
			else {
				// Expression
				if (RBQCodeBuffer[strlen(RBQCodeBuffer) - 1] == '\n')
					RBQCodeBuffer[strlen(RBQCodeBuffer) - 1] = '\0';

				RBQExecBuffer[0] = '\0';
				RBQSymbol[0] = '\0';
				sprintf(RBQSymbol, "__execwrapper_%d", ExecId);
				sprintf(RBQExecBuffer, "int %s(){return %s;}", RBQSymbol, RBQCodeBuffer);
				ExecId += 1;

				if (access("/tmp/crepltmp.c", F_OK) != -1) {
				    if (system("cp /tmp/crepltmp.c /tmp/crepltmpexec.c") != 0) {
						printf("ERROR: Copy failed.\n");
						return 1;
					}
				}

				FangPi = fopen("/tmp/crepltmpexec.c", "a+");
				if (FangPi == NULL) {
					printf("ERROR: Cannot open temporary file.\n");
					continue;
				}

				fprintf(FangPi, "%s", RBQExecBuffer);
				fclose(FangPi);

				if (system("gcc -shared -fPIC /tmp/crepltmpexec.c -o /tmp/crepltmpexec.so -ldl") != 0) {
					printf("ERROR: Compilation failure.\n");
					return 1;
				}
                
				handle = dlopen("/tmp/crepltmpexec.so", RTLD_LAZY);
 				if (!handle) {
					printf("ERROR: Failed to load temporary module.\n");
					return 1;
				}

				ExecFunction = dlsym(handle, RBQSymbol);
				if ((error = dlerror()) != NULL) {
					printf("ERROR: Failed to find symbol.\n");
					printf("Detail: %s\n", error);
					return 1;
				}

				printf(">> %s = %d\n", RBQCodeBuffer, (*ExecFunction)());

                dlclose(handle);
			}
		}
		else {
			return 0;
		}
	}
    return 0;
}
