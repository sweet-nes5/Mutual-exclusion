/*********************************************
 * Developpeuses: Bey Nesrine, Sawsen Aouida *
 * Projet: Projet de systeme M1              *                                     
 * Description: Implementation de verrous    *                                       
 ********************************************/

#include <stdio.h> // perror()
#include <stdlib.h> // malloc(), exit(), atexit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h> // ftruncate()
#include <fcntl.h> // fcntl : file control ; Objets memoire POSIX : pour les constantes O_
#include <sys/mman.h> // mmap(), munmap() ; Objets memoire POSIX : pour shm_open()
#include <sys/stat.h> // fstat(), Pour les constantes droits d’acces
#include <pthread.h> // pthread_mutexattr_init(), pthread_mutexattr_setpshared(), pthread_mutex_init()
#include <errno.h> // Pour strerror(), la variable errno
#include <string.h> // strerror(), memset(), memmove()
#include <stdarg.h> // Pour acceder a la liste des parametres de l’appel de fonctions avec un nombre variable de parametres
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
#include "rl_lock_library.h"

rl_descriptor rl_open(const char *path, int oflag, ...){
    //allouer memoire
    rl_descriptor descriptor;
    rl_descriptor* file = (rl_descriptor*) malloc(sizeof(rl_descriptor));
    if (file == NULL)
    {
        fprintf(stderr, "L'allocation de mémoire à l'aide de la fonction malloc() à echoué \n");
        descriptor.d =-1; 
        return descriptor;// en cas d'echec on retourne l'objet dont le champ d est -1

    }


    int  new_shm = true;  /* new_shm == true si la creation de nouveau
			     * shared memory object */
  /* ajouter '/' au debut du nom de shared memory object */
  char *shm_name = prefix_slash(path);

  /* open and create */
    descriptor.d = shm_open(shm_name, O_CREAT| O_RDWR | O_EXCL,
		    S_IWUSR | S_IRUSR);
  if( descriptor.d >= 0 ){
    //creation de shared memory object reussie.
    //fixer la taille de shared memory object
    //MacOS permet de modifier la taille objet mémoire
    //uniquement après la création quand sa taille est 0
    if( ftruncate( descriptor.d, sizeof( file ) ) < 0 )
      PANIC_EXIT("ftruncate");
  }
  else if( descriptor.d < 0 && errno == EEXIST ){/* shared memory object existe déjà
				       * il suffit de l'ouvrir */
    descriptor.d = shm_open(shm_name,  O_RDWR, S_IWUSR | S_IRUSR);
    if( descriptor.d < 0 )
      PANIC_EXIT("shm_open existing object");
    new_shm = false;
    
  }else
    PANIC_EXIT("shm_open");
    
}