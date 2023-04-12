#ifndef RL_LOCK_LIBRARY_H_
#include <stdlib.h> // srand() atoi() rand() exit() EXIT_SUCCESS EXIT_FAILURE
#include <unistd.h> // fork(), getpid(), sleep()
#include <stdio.h> // fprintf(), perror()
#include <sys/mman.h> // mmap()
#include <pthread.h> // pthread_mutexattr_init(), pthread_mutexattr_setpshared(), pthread_mutex_init()
#include <sys/wait.h> // wait()
#include <errno.h>
#include <string.h>

#define NB_OWNERS 5
#define NB_LOCKS 
#define NB_FILES 256
#define PANIC_EXIT( msg )  do{			\
   fprintf(stderr,\
     "\n %d %s : error \"%s\" in file %s in line %d\n",\
	   (int) getpid(), msg, strerror(errno), __FILE__, __LINE__);	\
   exit(1);\
  } while(0)		
#define PANIC( msg )  do{			\
   fprintf(stderr,\
     "\n %d %s : error \"%s\" in file %s in line %d\n",\
	   (int) getpid(),msg, strerror(errno), __FILE__, __LINE__);	 \
  } while(0)	
char *prefix_slash(const char *name);

typedef struct{
    pid_t proc; /* pid du processus */
    int des; /* descripteur de fichier */
} owner;

typedef struct{
    int next_lock;
    off_t starting_offset;
    off_t len;
    short type; /* F_RDLCK F_WRLCK */
    size_t nb_owners;
    owner lock_owners[NB_OWNERS];
} rl_lock;

typedef struct{
    int first;
    rl_lock lock_table[NB_LOCKS];
} rl_open_file;

typedef struct{
    int d;
    rl_open_file *f;
} rl_descriptor;

static struct {
    int nb_files;
    rl_open_file *tab_open_files[NB_FILES];
    pthread_mutex_t mutex;
} rl_all_files;

// les fonctions

rl_descriptor rl_open(const char *path, int oflag, ...);
int initialiser_mutex(pthread_mutex_t *pmutex);
int initialiser_cond(pthread_cond_t *pcond);





#endif