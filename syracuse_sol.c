#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t recu = 0;

void handler(int signal){
  recu = 1;
}

int  syracuse( long n){
  long k = n;
  do{

    
    if(( k % 2 ) == 1 ){
      k = 3*k+1;
    }else{
      k /= 2;
    }

    if(recu){
      recu = 0;
      printf( "n=%ld k=%ld\n", n, k);
    }

  }while( k > n );
  return ( k == n )? 0 : 1;
}

int main(int argc, char **argv){
  if( argc != 2){
    fprintf(stderr, "usage : %s valeur_initiale\n", argv[0]);
    exit(1);
  }
  long fin =  atol( argv[1] );


  struct sigaction sigact;
  sigact.sa_handler = handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags = 0;
  
  if(sigaction(SIGUSR1,&sigact,NULL) < 0){
    perror("sigaction");
    exit(1);
  }
  
  printf("pid = %d\n", getpid());

  for(long d = 2 ; d < fin; d++){
    int v = syracuse(d);
    if( v == 0 )
      printf("conjecture fausse pour %ld\n", d);
  }
  printf("conjecture satisfaites jusqu'à %ld\n", fin);
}
      
  
