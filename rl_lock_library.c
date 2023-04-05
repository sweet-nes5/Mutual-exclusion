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
#include <stdarg.h>
#include "rl_lock_library.h"



int initialiser_mutex(pthread_mutex_t *pmutex){
    pthread_mutexattr_t mutexattr;
  int code;
  if( ( code = pthread_mutexattr_init(&mutexattr) ) != 0){
    fprintf(stderr, "Fonction pthread_mutexattr_init(): ");
    return code;
  }
  if( ( code = pthread_mutexattr_setpshared(&mutexattr, PTHREAD_PROCESS_SHARED) ) != 0){
    fprintf(stderr, "Fonction pthread_mutexattr_setpshared(): ");	    
    return code;
  code = pthread_mutex_init(pmutex, &mutexattr) ;
  return code;
  }
}

int initialiser_cond(pthread_cond_t *pcond){
  pthread_condattr_t condattr;
  int code;
  if( ( code = pthread_condattr_init(&condattr) ) != 0 )
    return code;
  if( ( code = pthread_condattr_setpshared(&condattr,PTHREAD_PROCESS_SHARED) ) != 0 )
    return code;
  code = pthread_cond_init(pcond, &condattr) ; 
  return code;
}	



rl_descriptor rl_open(const char *path, int oflag, ...){
    /*allouer memoire
    rl_descriptor descriptor = (rl_descriptor*) malloc(sizeof(rl_descriptor));
    if (descriptor.d == -1)
    {
        fprintf(stderr, "L'allocation de mémoire à l'aide de la fonction malloc() à echoué \n");
        descriptor.d =-1; 
        exit(EXIT_FAILURE);// en ca.d'echec on retourne l'objet dont le champ.d est -1

    }*/
    rl_descriptor descriptor;
    void* ptr = NULL;
    //que passer dans l'argument de mmap
    int mmap_protect;
    if((O_RDWR & oflag)== O_RDWR)
      mmap_protect= PROT_READ | PROT_WRITE;

    int  new_shm = true;  /* new_shm == true si la creation de nouveau
			     * shared memory object */
  /* ajouter '/' au debut du nom de shared memory object */
    char *shm_name = prefix_slash(path);
    mode_t mode = 0;
  /* open and create */
  if((O_CREAT & oflag)== O_CREAT){// veut dire que l'objet memoire n'existe pas encore
        va_list liste_parametres;
        va_start(liste_parametres,oflag);
        mode_t mode = va_arg(liste_parametres, mode_t);
        va_end(liste_parametres);
   
    int taille_memoire= sizeof(rl_open_file);
    descriptor.d = shm_open(shm_name, oflag, mode);
  if( descriptor.d >= 0 ){//creation de shared memory object reussie.
    
    if( ftruncate( descriptor.d, sizeof(rl_open_file) ) < 0 )
      PANIC_EXIT("ftruncate");
    ptr= mmap((void*)0,taille_memoire,mmap_protect,MAP_SHARED,descriptor.d,0 );
    if(ptr == MAP_FAILED){
      PANIC_EXIT("Fonction mmap()");} 

    descriptor.f= ptr;
  }
 }  
  //erreur avec l'ouverture de shm
  else if( descriptor.d < 0 && errno == EEXIST ){/* shared memory object existe déjà et il suffit de l'ouvrir */
				       
    descriptor.d = shm_open(shm_name,  O_RDWR, S_IWUSR | S_IRUSR);
    if( descriptor.d < 0 )
      PANIC_EXIT("shm_open existing object");
    new_shm = false;
    
  }else
    PANIC_EXIT("shm_open");

  rl_all_files.nb_files = 0;
 *rl_all_files.tab_open_files =descriptor.f;
   
  //initialiser la memoire seulement si nouveau shared memory object est créé
  if (new_shm)
  {
    initialiser_mutex(&(descriptor.mutex));
    //initialiser rl_all_files
      

















  }
    

    /*oflag is a bit mask created by ORing together exactly one of
       O_RDONLY or O_RDWR and any of the other flags listed here:

       O_RDONLY
              Open the object for read access.  A shared memory object
              opened in this way can be mmap(2)ed only for read
              (PROT_READ) access.

       O_RDWR Open the object for read-write access.

       O_CREAT
              Create the shared memory object if it does not exist

              A new shared memory object initially has zero length—the
              size of the object can be set using ftruncate(2).  The
              newly allocated bytes of a shared memory object are
              automatically initialized to 0.

       O_EXCL If O_CREAT was also specified, and a shared memory object
              with the given name already exists, return an error.  The
              check for the existence of the object, and its creation if
              it does not exist, are performed atomically.

       O_TRUNC
              If the shared memory object already exists, truncate it to
              zero bytes.*/




return descriptor;









    
}