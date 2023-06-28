/********************************************************************
 * Developpeuses: Bey Nesrine                                        *
 * Projet: Projet de systeme M1                                      *                                     
 * Description: Implementation de verrous sur segments de fichiers   *                                       
 ********************************************************************/

#include <stdio.h> // perror()
#include <stdlib.h> // malloc(), exit(), atexit(), EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h> // ftruncate()
#include <sys/mman.h> // mmap(), munmap() ; Objets memoire POSIX : pour shm_open()
#include <sys/stat.h> // fstat(), Pour les constantes droits d’acces
#include <pthread.h> // pthread_mutexattr_init(), pthread_mutexattr_setpshared(), pthread_mutex_init()
#include <errno.h> // Pour strerror(), la variable errno
#include <string.h> // strerror(), memset(), memmove()
#include <stdarg.h> // Pour acceder a la liste des parametres de l’appel de fonctions avec un nombre variable de parametres
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <stdarg.h>
#include "rl_lock_library.h"

#define MAX_OWNERS 5
#define SHARED_MEMORY_PREFIX "/f_"

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
char* getSHM(const struct stat* fileStats) {
  char* objectName = malloc(sizeof(char) * 256);
  sprintf(objectName, "%s%lu_%lu", SHARED_MEMORY_PREFIX, fileStats->st_dev, fileStats->st_ino);
  return objectName;
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

/*  WZ
 *   variable globale ?
 *   static ?
*/
//variable globale pour le nom du fichier
char *currentFileName = NULL;

rl_descriptor rl_open(const char *path, int oflag, ...){
    
  currentFileName = strdup(path);
  rl_descriptor descriptor = {.d = -1, .f = NULL};
  void* ptr = NULL;
  struct stat fileStats;
  int taille_memoire= sizeof(rl_open_file);
  //que passer dans l'argument de mmap
  int mmap_protect= PROT_READ | PROT_WRITE;
  bool new_shm = false;  /* new_shm == true si la creation de nouveau shared memory object */
  char *shm_name=NULL;
  /* open and create */

  if((O_CREAT & oflag)== O_CREAT){// veut dire que le fichier n'existe pas encore et on va le creer
    va_list liste_parametres;
    va_start(liste_parametres,oflag);
    mode_t mode = va_arg(liste_parametres, mode_t);
    va_end(liste_parametres);
    // ouvrir le fichier pour descriptor
    descriptor.d = open( path, oflag, mode ); 
    if(descriptor.d == -1){
      fprintf(stderr, "[%d] open file failed : %s\n", (int)getpid(), strerror(errno));
      return descriptor;
    }else 
      printf("[%d] file opened correctly.\n", (int)getpid() );
    //pour le nom de l'shm
      if (fstat(descriptor.d,&fileStats)== -1){
        fprintf(stderr,"Failed to get file stats.\n");
        close(descriptor.d);
        return (rl_descriptor){ .d = -1, .f = NULL }; 
      }
    
       
  }
  else{
    descriptor.d = open( path, oflag );
    if( descriptor.d == -1 ){
      fprintf(stderr, "[%d] open file failed : %s\n", (int)getpid(), strerror(errno));
      return descriptor;}
  }

  /* nom de shared memory object */
  shm_name = getSHM(&fileStats);
  printf("[%d] the shared memory object name is : %s\n", (int) getpid(), shm_name);
  int shm_fd = shm_open( shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);

  if(shm_fd >= 0){  //objet shm est créé, donc faire ftruncate et //ensuite la projection en mémoire et  initialiser la structure rl_open_file
      //set the size of the shared memory object
    printf("[%d] Shared memory object does not exist. Creating it...\n", (int) getpid());
    if( ftruncate( shm_fd, sizeof(rl_open_file) ) < 0 ){
      const char *msg = strerror(errno);
      fprintf(stderr, "[%d] Error in ftruncate: %s\n", (int) getpid(), msg);
      exit(EXIT_FAILURE);
    }
    printf("[%d] Shared memory object created.\n", (int) getpid());  
    //projection en memoire
    ptr= mmap((void*)0, taille_memoire,  mmap_protect, MAP_SHARED,shm_fd,0 );
    if(ptr == MAP_FAILED){
      fprintf( stderr, "[%d] mmap failed %s\n", (int)getpid(), strerror(errno));
      close(shm_fd);
      fprintf(stderr, "[%d] exist\n", (int)getpid());
      exit(EXIT_FAILURE);
    }  
    descriptor.f= ptr;
    
    //initialiser les mutex et mettre à jour la variable rl_all_files seulement si nouveau shared memory object est créé    
    int result_init_mutex = initialiser_mutex(&(descriptor.f->mutex));
    if(result_init_mutex != 0){
      char* erreur_init_mutex = strerror(result_init_mutex);
      fprintf(stderr, "[%d] erreur dans l'initialisation du mutex : %s \n",(int) getpid(), erreur_init_mutex);
      /* WZ
	 si on n'a pas reussi d'initialiser le mutex à quoi bon continuer ?
      */
    }
    // Initialize lock_table
    for (int i = 0; i < NB_LOCKS; i++) {
        descriptor.f->lock_table[i].type = F_UNLCK;
        descriptor.f->lock_table[i].starting_offset = 0;
        descriptor.f->lock_table[i].len = 0;
        descriptor.f->lock_table[i].nb_owners = 0;
        for (int j = 0; j < NB_OWNERS; j++) {
            descriptor.f->lock_table[i].lock_owners[j].proc = -1;
            descriptor.f->lock_table[i].lock_owners[j].des = -1;
        }
    }
    
    int result_init_cond = initialiser_cond(&(descriptor.f->read_cond));
    if(result_init_cond != 0){
      char* erreur_init_cond = strerror(result_init_cond);
      fprintf(stderr, "erreur dans l'initialisation du mutex : %s \n",erreur_init_cond);
      /*WZ
       * si on n'a pas réussi d'initiliser la condition à quoi bon contiunuer ?
       */
    }
    int result_init_cond1 = initialiser_cond(&(descriptor.f->write_cond));
    if(result_init_cond1 != 0){
      char* erreur_init_cond1 = strerror(result_init_cond1);
      fprintf(stderr, "erreur dans l'initialisation du mutex : %s \n",erreur_init_cond1);
      /*WZ
       * si on n'a pas réussi d'initiliser la condition à quoi bon contiunuer ?
       */
    }
    descriptor.f->num_readers = 0;
    descriptor.f->num_writers = 0;
    descriptor.f->num_writers_waiting = 0;
    descriptor.f->num_readers_waiting = 0;
    rl_all_files.tab_open_files[rl_all_files.nb_files] = (descriptor.f);// va recevoir le resultat de mmap
    rl_all_files.nb_files++;
    rl_all_files.ref_count++;
    /* WZ
     * à quoi sert le champs ref_count ???    pour la suppression du shared memory object
     */
    return descriptor;
  }
  else if (shm_fd == -1 && errno == EEXIST) {
    printf("[%d] Shared memory object exists. Projecting it...\n", (int) getpid() );
    // Refaire shm_open
    shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
      const char *msg = strerror(errno);
      fprintf(stderr, "[%d] Error opening the Shared object memory: %s\n", (int) getpid(), msg);
      exit(EXIT_FAILURE);
    }
    
    // Projection en mémoire
    ptr = mmap((void *)0, taille_memoire, mmap_protect, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
      close(shm_fd);
      perror("Fonction mmap()");
      exit(EXIT_FAILURE);
    }
    sleep(1);
    descriptor.f = ptr;
    return descriptor;
  }

  /*WZ
   * si errno != EEXIST
   */
  close(descriptor.d);
  return (rl_descriptor) {.d =-1, .f = NULL};
}


int rl_close(rl_descriptor lfd) {
  int result = close(lfd.d);
  if (result == -1) {
    const char *msg = strerror(errno);
    fprintf(stderr, "Error closing the file descriptor: %s\n", msg);
    exit(EXIT_FAILURE);
  }
  //deconnecter le processus de la memoire partagé mais celle ci n'est pas détruite
  if(munmap( (void *) lfd.f, sizeof(rl_open_file)) == -1){
    fprintf(stderr,"unmap");
    exit(EXIT_FAILURE);
  }
  //printf("%s\n",currentFileName);    
  rl_all_files.nb_files--;
  //printf("%d\n", rl_all_files.nb_files);
  //seulement si dernier processus veut fermer
  if( rl_all_files.nb_files==0 ){

    if (access(currentFileName, F_OK) != -1) {
      printf("[%d] File has been closed succesfully.\n", (int) getpid());
      if (unlink(currentFileName) == -1) {
	perror("unlink");
	exit(EXIT_FAILURE);}
    }
    remove(currentFileName);
    
  }


  //printf("%d\n", rl_all_files.nb_files);
    
  /*int nb_owners_delete = sizeof(NB_OWNERS);
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

  }*/

  return 0;
}


 int rl_fcntl(rl_descriptor lfd, int cmd, struct flock *lck) {
    if (cmd == F_SETLK) {
        int mutex_lock_result = pthread_mutex_lock(&(lfd.f->mutex));
        if (mutex_lock_result != 0) {
            fprintf(stderr, "Function pthread_mutex_lock() failed: %s\n", strerror(mutex_lock_result));
            exit(EXIT_FAILURE);
        }

        owner lfd_owner = {.proc = getpid(), .des = lfd.d};
        //increment num readers and writers 
        if(lck->l_type == F_RDLCK){
          lfd.f->num_readers ++;
          printf("Process %d incremented num_readers: %d\n", getpid(), lfd.f->num_readers);
        }else if(lck->l_type == F_WRLCK){
          lfd.f->num_writers ++;
          printf("Process %d incremented num_writers: %d\n", getpid(), lfd.f->num_writers);
        }

        // Check for lock conflicts
        printf("Checking for lock conflicts...\n");
        printf("lck->l_type: %d\n", lck->l_type);
        printf("lfd.f->num_readers: %d\n", lfd.f->num_readers);
        printf("lfd.f->num_writers: %d\n", lfd.f->num_writers);

        // Wait for the lock to become available
        while ((lck->l_type == F_WRLCK && (lfd.f->num_readers > 0 || lfd.f->num_writers > 0)) ||
       (lck->l_type == F_RDLCK && lfd.f->num_writers > 1)) {
        printf("Lock conflict: Process %d waiting...\n", getpid());
        if (lck->l_type == F_WRLCK) {
            lfd.f->num_writers_waiting++;
            pthread_cond_wait(&(lfd.f->write_cond), &(lfd.f->mutex));
            lfd.f->num_writers_waiting--;
        } else if (lck->l_type == F_RDLCK) {
            lfd.f->num_readers_waiting++;
            pthread_cond_wait(&(lfd.f->read_cond), &(lfd.f->mutex));
            lfd.f->num_readers_waiting--;
        }
    }


        // Release the lock
        if (lck->l_type == F_WRLCK) {
            lfd.f->num_writers--;
        } else if (lck->l_type == F_RDLCK) {
            lfd.f->num_readers--;
        }

        

        // Find an available index in the lock table
        int lock_index = -1;
        for (int i = 0; i < NB_LOCKS; i++) {
            if (lfd.f->lock_table[i].type == F_UNLCK) {
                lock_index = i;
                break;
            }
        }

        if (lock_index == -1) {
            fprintf(stderr, "Lock table is full\n");
            errno = EAGAIN;
            int mutex_unlock_result = pthread_mutex_unlock(&(lfd.f->mutex));
            if (mutex_unlock_result != 0) {
                fprintf(stderr, "Function pthread_mutex_unlock() failed: %s\n", strerror(mutex_unlock_result));
                exit(EXIT_FAILURE);
            }
            return -1;
        }

        // Set lock information in the lock table
        lfd.f->lock_table[lock_index].type = lck->l_type;
        lfd.f->lock_table[lock_index].starting_offset = lck->l_start;
        lfd.f->lock_table[lock_index].len = lck->l_len;
        lfd.f->lock_table[lock_index].nb_owners++;
        lfd.f->lock_table[lock_index].lock_owners[lfd.f->lock_table[lock_index].nb_owners-1] = lfd_owner;


        // Check for lock conflicts
        for (int i = 0; i < NB_LOCKS; i++) {
            if (i != lock_index && lfd.f->lock_table[i].type != F_UNLCK &&
                lfd.f->lock_table[i].starting_offset < lck->l_start + lck->l_len &&
                lck->l_start < lfd.f->lock_table[i].starting_offset + lfd.f->lock_table[i].len) {
                // Check owners for a conflict
                for (int j = 0; j < lfd.f->lock_table[i].nb_owners; j++) {
                    if (lfd.f->lock_table[i].lock_owners[j].proc == lfd_owner.proc &&
                        lfd.f->lock_table[i].lock_owners[j].des == lfd_owner.des) {
                        // Process found itself in the owners array
                        continue;  // Skip current owner
                    }

                    // Lock conflict error
                    fprintf(stderr, "Lock conflict error: Process %d tried to acquire lock on the same segment\n", getpid());
                    errno = EAGAIN;

                    // Rollback the lock addition
                    lfd.f->lock_table[lock_index].type = F_UNLCK;
                    lfd.f->lock_table[lock_index].starting_offset = 0;
                    lfd.f->lock_table[lock_index].len = 0;
                    lfd.f->lock_table[lock_index].nb_owners = 0;
                    for (int k = 0; k < MAX_OWNERS; k++) {
                        lfd.f->lock_table[lock_index].lock_owners[k].proc = -1;
                        lfd.f->lock_table[lock_index].lock_owners[k].des = -1;
                    }


                    int mutex_unlock_result = pthread_mutex_unlock(&(lfd.f->mutex));
                    if (mutex_unlock_result != 0) {
                        fprintf(stderr, "Function pthread_mutex_unlock() failed: %s\n", strerror(mutex_unlock_result));
                        exit(EXIT_FAILURE);
                    }

                    return -1;
                }
            }
        }
      
        int mutex_unlock_result = pthread_mutex_unlock(&(lfd.f->mutex));
        if (mutex_unlock_result != 0) {
            fprintf(stderr, "Function pthread_mutex_unlock() failed: %s\n", strerror(mutex_unlock_result));
            exit(EXIT_FAILURE);
        }
        // Signal waiting writers or readers
        if (lfd.f->num_writers_waiting > 0) {
            pthread_cond_signal(&(lfd.f->write_cond));
        } else if (lfd.f->num_readers_waiting > 0) {
            pthread_cond_signal(&(lfd.f->read_cond));
        }

        // Lock addition successful
        return 0;
    } else {
        printf("Unsupported command\n");
        return -1;
    }
}


//gcc -o test test1.c rl_lock_library.c -lpthread

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
  //compteur d'objets
  rl_all_files.ref_count = 0;
  return 0;

}
