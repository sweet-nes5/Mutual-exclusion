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
#include "rl_lock_library.c"



int main(int argc, char *argv[]){
   
   /*rl_descriptor test = rl_open("f_4_1000564",O_RDWR| O_CREAT,0666);
        if (test.d == -1)
        exit(EXIT_FAILURE);
   int i;
   pid_t fork_result;
   for ( i = 0; i < 3; i++)
   {
    fork_result = fork();
    if ((fork_result == -1)){
        perror("Fonction Fork()");
        exit(EXIT_FAILURE);
    }
    if (fork_result == 0)
    {
         rl_close(test);
    }
    
    
    
   }*/
   
   
   








    return 0;
}