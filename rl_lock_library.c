 /********************************************************************
 * Developpeuses: Bey Nesrine                                        *
 * Projet: Projet de systeme M1                                      *                                     
 * Description: Implementation de verrous sur segments de fichiers   *                                       
 ********************************************************************/

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
    return code; }
  code = pthread_mutex_init(pmutex, &mutexattr) ;
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
int segment_unlocked (rl_descriptor lfd, off_t start, off_t len){
 rl_open_file *file = rl_all_files.tab_open_files[lfd.d];
  //parcours des verrous associés au fichier
  for (int i = 0; i < NB_LOCKS; i++)
  {
    rl_lock *lock = &file->lock_table[i];
    //verification du chevauchement de verrou avec le segment
    if (lock->type != F_UNLCK && lock->starting_offset < start+len && lock->starting_offset + lock->len > start)
    {
      return 0; // chevauchement de verrou
      
    }
    
  }
  return 1;// aucun verrou ne chevauche le segment
  
}


rl_descriptor rl_open(const char *path, int oflag, ...){

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
    
    int result_init_mutex = initialiser_mutex(&(descriptor.f->mutex));
    if(result_init_mutex != 0){
      char* erreur_init_mutex = strerror(result_init_mutex);
      fprintf(stderr, "erreur dans l'initialisation du mutex : %s \n",erreur_init_mutex);}
      
    int result_init_cond = initialiser_cond(&(descriptor.f->verrou_libre));
    if(result_init_cond != 0){
      char* erreur_init_cond = strerror(result_init_cond);
      fprintf(stderr, "erreur dans l'initialisation du mutex : %s \n",erreur_init_cond);}

    *rl_all_files.tab_open_files = (descriptor.f);
    (*rl_all_files.tab_open_files) ++;
    rl_all_files.nb_files++;
    
      

  }


return descriptor;
    
}
int rl_close(rl_descriptor lfd) {
    int result = close(lfd.d);
    if (result == -1) {
        const char *msg = strerror(errno);
        fprintf(stderr, "Error closing the file: %s\n", msg);
        exit(EXIT_FAILURE);
    }
    int nb_owners_delete = sizeof(NB_OWNERS);
    //int nb_locks_delete = size(NB_LOCKS);

    owner lfd_owner = {.proc = getpid(), .des = lfd.d};
    for (int i = 0; i < nb_owners_delete ; i++) {
        if ((lfd.f->lock_table[i].nb_owners > 0) &&
            (lfd.f->lock_table[i].lock_owners->des == lfd_owner.des) &&
            (lfd.f->lock_table[i].lock_owners->proc == lfd_owner.proc)) {
            // Remove the lock from the lock_table array
            // treat it a table nd delete the element free(lfd.f->lock_table[i].lock_owners);
            lfd.f->lock_table[i-1].next_lock++;
            lfd.f->lock_table[i].nb_owners--;
            nb_owners_delete--;
          
        }
        if ((lfd.f->lock_table[i].nb_owners == 1) || (lfd.f->lock_table[i].lock_owners->des == lfd_owner.des) ){
            // Delete the lock from the lock_table array
            lfd.f->lock_table[i-1].next_lock++;
            nb_owners_delete--;
            int j =0;
            int nb_locks_delete = sizeof(NB_LOCKS);
            while (j<nb_locks_delete -1)
            {
              lfd.f->lock_table[j] = lfd.f->lock_table[j+1];
              j++;
            }
            
        }

    }

    return 0;
}
int rl_fcntl(rl_descriptor lfd, int cmd, struct my_flock *lck){
   if (lfd.d <0)
   {
    errno = EBADF;
    return -1;
   }
   int peut_continuer = 0;
   rl_open_file *ptr_f = lfd.f;
   int lock_size = sizeof(ptr_f->lock_table);

   switch (cmd)
   {
   case F_SETLK:{
    int j;
    //verifier si verrou existe deja
    for (int i = 0; i < lock_size; i++)
    {
      if (ptr_f->lock_table[i].nb_owners>0)
      {
        for (j = 0; j < ptr_f->lock_table->nb_owners; j++)
        {
          owner current = ptr_f->lock_table[i].lock_owners[j];
          //verifier verrou incompatible
          if ((current.proc != lfd.f->lock_table[i].lock_owners[j].proc) && (lck->rl_type == F_WRLCK))
          {
            errno = EAGAIN;// ressource non available
            return -1;
          }
          
        }
        
      }
      
    }
    //parcourir lock_table pour trouver le verrou
    int lock_pos = -1;
    for (int i = 0; i < lock_size; i++)
    {
      if ((ptr_f->lock_table[i].starting_offset == lck->rl_start) && (ptr_f->lock_table[i].len == lck->len))
      {
        lock_pos = i;
        break;
      }
      
    }
    //on cree un nouveau verrou seulement si il n'existe pas
    if (lock_pos == -1)
    {
      for (int i = 0; i < lock_size; i++)
      {
        if (ptr_f->lock_table[i].nb_owners == 0)
        {
          lock_pos = i;
          ptr_f->lock_table[i].starting_offset = lck->rl_start;
          ptr_f->lock_table[i].len = lck->len;
          break;
        }
        
      }
      
    }
    
    //gestion d'erreur ou cas ou le verrou reste introuvable
    if (lock_pos == -1)
    {
      errno = ENOMEM;
      return -1;
    }
    // mettre a jour
    int index_last_owner_pos = ptr_f->lock_table[lock_pos].nb_owners;
    ptr_f->lock_table[lock_pos].lock_owners[index_last_owner_pos].proc = lfd.f->lock_table[lock_pos].lock_owners[j].proc;
    ptr_f->lock_table[lock_pos].type = lck->rl_type;
    ptr_f->lock_table[lock_pos].nb_owners++;

    return 0;
   }
   case F_SETLKW:{
    int result_lock = pthread_mutex_lock(&(ptr_f->mutex));
    if (result_lock != 0)
    {
      perror("pthread lock");
      exit(EXIT_FAILURE);
    }
    
      if (!segment_unlocked(lfd,lck->rl_start,lck->len))
      {
        peut_continuer = 0;
      }
      else 
        peut_continuer = 1;
      
      //verifier si un verrou n'a pas de propriétaire et si un segments de fichier n'a pas de proprietaire en ecriture
      while (!peut_continuer)
      {// un verrou chevauche le segment alors on fait un wait
      int result_wait = pthread_cond_wait(&(ptr_f->verrou_libre),&(ptr_f->mutex));
      if (result_wait !=0)
      {
        perror("pthread wait");
        exit(EXIT_FAILURE);
      }
      if (segment_unlocked(lfd,lck->rl_start,lck->len))
        peut_continuer = 1; 
      }
      int result_unlock = pthread_mutex_unlock(&(ptr_f->mutex));
      if (result_unlock != 0)
      {
        perror("pthread unlock");
        exit(EXIT_FAILURE);
      }
      
      

      return 0;
   } 
   
   default:
        errno = EINVAL;
        return -1;
    break;
   }
   

  return cmd;
  
}
rl_descriptor rl_dup( rl_descriptor lfd ){
  int newd = dup( lfd.d );
  if ( newd == -1 ) {
    
			perror("Function dup()"); //  void perror (const char *message)
			exit(EXIT_FAILURE);
		
  }
  int nb = sizeof(NB_OWNERS);
  int pos;
  owner lfd_owner = {.proc = getpid(), .des = lfd.d};
  for (int i = 0; i < nb ; i++) {
        if ((lfd.f->lock_table[i].nb_owners > 0) &&
            (lfd.f->lock_table[i].lock_owners->des == lfd_owner.des) &&
            (lfd.f->lock_table[i].lock_owners->proc == lfd_owner.proc)) {
                owner new_owner = {.proc = getpid(), .des = newd } ;
                for(int j=0; j<lfd.f->lock_table[i].nb_owners+1; j++ ){
                    pos = j;
                }
                lfd.f->lock_table[i].lock_owners[pos] = new_owner;
                //lfd.f->lock_table[i].lock_owners[pos-1] = new_owner;
                lfd.f->lock_table->nb_owners++;
                nb++;
            }
  }
  rl_descriptor new_rl_descriptor = { .d = newd, .f = lfd.f };
  return new_rl_descriptor;
}
rl_descriptor rl_dup2( rl_descriptor lfd, int newd ){
  int dup2_result = dup2( lfd.d , newd );
  if ( dup2_result == -1 ) {
    
			perror("Function dup2()"); //  void perror (const char *message)
			exit(EXIT_FAILURE);
		
  }
  int nb = sizeof(NB_OWNERS);
  int pos;
  owner lfd_owner = {.proc = getpid(), .des = lfd.d};
  for (int i = 0; i < nb ; i++) {
        if ((lfd.f->lock_table[i].nb_owners > 0) &&
            (lfd.f->lock_table[i].lock_owners->des == lfd_owner.des) &&
            (lfd.f->lock_table[i].lock_owners->proc == lfd_owner.proc)) {
                owner new_owner = {.proc = getpid(), .des = newd } ;

                for(int j=0; j<lfd.f->lock_table[i].nb_owners+1; j++ ){
                    pos = j;
                }
                lfd.f->lock_table[i].lock_owners[pos] = new_owner;
                //lfd.f->lock_table[i].lock_owners[pos-1] = new_owner;
                lfd.f->lock_table->nb_owners++;
                nb++;
            }
  }
  rl_descriptor new_rl_descriptor = { .d = newd, .f = lfd.f };
  return new_rl_descriptor;

}
pid_t rl_fork(){
  pid_t fork_result = fork();
  if (fork_result == 0) // child process
  {
    pid_t parent = getppid();
    
    for (int i = 0; i < NB_LOCKS; i++)
    {
      //verifier si le parent a des verrous
      if(rl_all_files.tab_open_files[parent]!= NULL && rl_all_files.tab_open_files[parent]->lock_table[i].nb_owners>0){
        //on parcour le tableau lock_owners
        for (int j = 0; j < NB_OWNERS; j++)
        {
          owner parent_owner = rl_all_files.tab_open_files[parent]->lock_table[i].lock_owners[j];
          if (parent_owner.proc == parent)
          {
            owner new_owner = { .proc = getpid(), .des = i };
            //copier le verrou du pere
            rl_all_files.tab_open_files[getpid()]->lock_table[i].lock_owners[j]= new_owner;
            //incrementer le nb_owners apres avoir ajouté le child process
            rl_all_files.tab_open_files[getpid()]->lock_table[i].nb_owners++;
          }
        }
      }   
    }
  }
  else if (fork_result == -1){
    perror("Function fork()");
    exit(EXIT_FAILURE);
  }
    //parent process
    int status;
    //waiting for the child to finish
    if(waitpid(fork_result,&status,0)== -1){
      perror("waitpid()");
      exit(EXIT_FAILURE);
    }
    return fork_result;
  
}
int rl_init_library(){
  // nb_files=0 alors
  rl_all_files.nb_files=0;
  //le tableau de pointeurs est vide donc on met tout à NULL
  for (int i = 0; i < NB_FILES; i++)
  {
    rl_all_files.tab_open_files[i]=NULL;
  }

  return 0;

}