#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "rl_lock_library.h"


int main(int argc , char **argv){

  if(argc != 5){
    fprintf(stderr, "usage : %s file start len R|W \n"
	    "file  : fichier\n"
	    "start : offset du début de verrou\n"
	    "le    : longueur de segment verrouilé\n"
	    "R     : verrou read\n"
	    "W     : verrou write\n"
	    "\n exemple :\n"
	    "     %s  totp.txt  2 7 W\n÷n"
	    "     poser le verrou write dur le ficheir toto.txt sur  7  octets"
	    "     à partir de 2e l'octet\n" , argv[0], argv[0] );
    exit(1); 
  }
  
  rl_init_library();
  
  
  struct flock fl = { .l_start=atoi(argv[2]),
		      .l_len = atoi(argv[3]),
		      .l_type = argv[4][0]=='R' ? F_RDLCK : F_WRLCK,
		      .l_whence = SEEK_SET
  };

  rl_descriptor d = rl_open(argv[1], O_RDWR|O_CREAT, 0666);
  
  if( d.d == -1 ){
    perror("rl_open");
    exit(1);
  }
  bool once = true;
  //  while( (rl_fcntl(d, F_SETLK, &fl) == -1) && (errno==EAGAIN) ){
  while( (rl_fcntl(d, F_SETLK, &fl) == -1)  ){
    if( !once )
      continue;
    once = false;
    if( errno != EAGAIN ){
      fprintf(stderr, "proc %d :  rl_fcntl echoue avec errno != EAGAIN\n"
	      "est\'que que vous avez bien geré errno ?\n", (int) getpid());
    }
    printf("proc %d : en attente de verrou\n", (int) getpid());
  }
  
  printf("process %d gets the lock\n", (int) getpid());
  sleep(3);

  fl.l_type = F_UNLCK;
  printf("process %d releases the lock\n", getpid());
  if( rl_fcntl(d, F_SETLK, &fl) == -1){
    fprintf(stderr, "process %d release lock error\n", (int) getpid());
  }
  rl_close(d);
}
