#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "rl_lock_library.h"

int main(int argc, char *argv[]) {
    rl_init_library(); // Initialize the library
    
    mode_t permissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fileOpened = 0;
    rl_descriptor test;
    if(!fileOpened){
        test = rl_open("file.txt", O_RDWR | O_CREAT, permissions);
        if (test.d == -1) {
            printf("Failed to open file.\n");
        } else {
            printf("shared memory object opened and projected successfully.\n");
        fileOpened = 1;
        }
    }
   // rl_lock example;
    /*struct my_flock lock;
    lock.rl_type = F_WRLCK; // write lock
    lock.rl_start = 0;
    lock.len = 0;
    int result = rl_fcntl(test, F_SETLK, &lock);
    if(result== -1){
        perror("rl_fcntl");
        exit(EXIT_FAILURE);
    }else if (result == 0){
        printf("File lock successful.\n");
    }else
        printf("Uknown result %d\n",result);*/

    if (fileOpened &&  rl_close(test) == 0) {
        printf("File closed successfully.\n");
        fileOpened = 0;
    }else{
        perror("Failed to close file.\n");
        exit(EXIT_FAILURE);
    }/**/
    
    
   
    
    
    /*rl_descriptor dup = rl_dup( test );
    if (dup.d == -1) {
        printf("Failed to open file.\n");
    } else {
        printf("File duplicated successfully.\n");
        printf("duplicated File closed successfully.\n");
        rl_close(dup);
       
    }
    int newd = open("output.txt", O_WRONLY | O_CREAT, 0666);
    rl_descriptor dup2 = rl_dup2( test,newd );
    if (dup.d == -1) {
        printf("Failed to open file.\n");
    } else {
        printf("File duplicated2 successfully.\n");
        printf("duplicated2 File closed successfully.\n");
        rl_close(dup2);
       
    }

    if (rl_close(test) != 0) {
        perror("rl_close");
        exit(EXIT_FAILURE);
    }
    printf("File closed successfully.\n");*/
    //pid_t test_result = rl_fork();
    //printf("le pid du child process est : %d\n", test_result);

   

    return 0;
}
