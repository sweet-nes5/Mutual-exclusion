#include <stdio.h> // perror()
#include <stdlib.h> // malloc(), exit(), atexit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h> // ftruncate()
#include <fcntl.h> // fcntl : file control ; Objets memoire POSIX : pour les constantes O_
#include <sys/mman.h> // mmap(), munmap() ; Objets memoire POSIX : pour shm_open()
#include <sys/stat.h> // fstat(), Pour les constantes droits d’acces // pthread_mutexattr_init(), pthread_mutexattr_setpshared(), pthread_mutex_init()
#include <errno.h> // Pour strerror(), la variable errno
#include <string.h> // strerror(), memset(), memmove()
#include <stdarg.h> // Pour acceder a la liste des parametres de l’appel de fonctions avec un nombre variable de parametres
#include <signal.h>
#include <sys/types.h>
#include<sys/wait.h>
#include "rl_lock_library.h"



int main(int argc, char *argv[]){

    rl_init_library();
    rl_descriptor test = rl_open("test_file.txt", O_RDWR | O_CREAT, 0666);
    if (test.d == -1) {
        printf("Failed to open file.\n");
    } else {
        printf("File opened successfully.\n");
        rl_close(test);
    }
    if (rl_close(test) != 0) {
    perror("rl_close");
    exit(EXIT_FAILURE);}
    return 0;
}