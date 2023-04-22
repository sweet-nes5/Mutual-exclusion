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

//gcc -Wall rl_lock_library.c main.c -o  min

int initialiser_mutex(pthread_mutex_t *pmutex){
    pthread_mutexattr_t mutexattr;
  int code = 0;
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
  return code;
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

char *prefix_slash(const char *name){
  #define L_NAME 256
  static char nom[L_NAME]; 

  if( name[0] != '/' ){
    nom[0] = '/';
    strncpy(nom+1, name, L_NAME-1);
  }else{
    strncpy(nom, name, L_NAME);
  }
  nom[L_NAME-1]='\0';
  return nom;
}



rl_descriptor rl_open(const char *path, int oflag, ...){
    /*allouer memoire*/
    rl_open_file* open_file= (rl_open_file*) malloc(sizeof(rl_open_file) + sizeof(int)* NB_LOCKS);
    if (open_file == NULL)
    {
      fprintf(stderr, "L'allocation de mémoire à l'aide de la fonction malloc() à echoué \n");
      exit(-1);
    }
    rl_descriptor descriptor = {.d = 0, .f = NULL};
    void* ptr = NULL;
    //que passer dans l'argument de mmap
    int mmap_protect;
    if((O_RDWR & oflag)== O_RDWR)
      mmap_protect= PROT_READ | PROT_WRITE;

    int  new_shm = true;  /* new_shm == true si la creation de nouveau shared memory object */
  /* ajouter '/' au debut du nom de shared memory object */
    char *shm_name = prefix_slash(path);
    //mode_t mode = 0;
  /* open and create */
  if((O_CREAT & oflag)== O_CREAT){// veut dire que l'objet memoire n'existe pas encore et on va le creer
        va_list liste_parametres;
        va_start(liste_parametres,oflag);
        mode_t mode = va_arg(liste_parametres, mode_t);
        va_end(liste_parametres);
   
    int taille_memoire= sizeof(rl_open_file);
    descriptor.d = shm_open(shm_name, oflag, mode);
  if( descriptor.d >= 0 ){//creation de shared memory object reussie.
    
    if( ftruncate( descriptor.d, sizeof(rl_open_file) ) < 0 )
      PANIC_EXIT("ftruncate");
    //projection en memoire
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
    exit(-1);
    
  //mettre à jour rl_all_files
   
   
  //initialiser la memoire seulement si nouveau shared memory object est créé
  if (new_shm)
  {
    
    int result_init_mutex = initialiser_mutex(&(open_file->mutex));
    if(result_init_mutex != 0){
      char* erreur_init_mutex = strerror(result_init_mutex);
      fprintf(stderr, "erreur dans l'initialisation du mutex : %s \n",erreur_init_mutex);}
      
    int result_init_cond = initialiser_cond(&(open_file->section_libre));
    if(result_init_cond != 0){
      char* erreur_init_cond = strerror(result_init_cond);
      fprintf(stderr, "erreur dans l'initialisation du mutex : %s \n",erreur_init_cond);}

    *rl_all_files.tab_open_files = (descriptor.f);
    (*rl_all_files.tab_open_files) ++;
    rl_all_files.nb_files++;
    
      

  }
    

  




return descriptor;
    
}
 int rl_close( rl_descriptor lfd){

  int result = close(lfd.d);
  if (result == -1){
    const char *msg;
    msg = strerror(errno);
    fprintf(stderr,"Eroor closing the file : %s\n",msg );
    exit(EXIT_FAILURE);
  }
  owner lfd_owner = {.proc = getpid(), .des = lfd.d};
  lfd.f->lock_table;




  return 0;
 }