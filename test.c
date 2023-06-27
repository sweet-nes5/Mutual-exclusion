#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "rl_lock_library.h"


#define READ_LOCK 1
#define WRITE_LOCK 2
#define UNLOCK 0


int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s file start len R|W\n"
                        "file  : fichier\n"
                        "start : offset du début de verrou\n"
                        "len   : longueur de segment verrouillé\n"
                        "R     : verrou read\n"
                        "W     : verrou write\n"
                        "\nExemple:\n"
                        "    %s toto.txt 2 7 W\n"
                        "    Poser le verrou write sur le fichier toto.txt sur 7 octets\n", argv[0], argv[0]);
        exit(1);
    }

    rl_init_library();
    struct flock fl = { .l_start=atoi(argv[2]),
		      .l_len = atoi(argv[3]),
		      .l_type = argv[4][0]=='R' ? F_RDLCK : F_WRLCK,
		      .l_whence = SEEK_SET
    };
    int fileOpened = 0;
    
    struct flock fl = {
        .rl_start = atoi(argv[2]),
        .len = atoi(argv[3]),
        .rl_type = argv[4][0] == 'R' ? READ_LOCK : WRITE_LOCK
    };

    rl_descriptor d;
    d = rl_open(argv[1], O_RDWR | O_CREAT, 0666);
    if (d.d == -1) {
        printf("Failed to open file.\n");
    } else {
        printf("shared memory object opened and projected successfully.\n");
        fileOpened = 1;
    }

    if (d.d == -1) {
        perror("rl_open");
        exit(1);
    }
    //ajouté pour enfin pouvoir exécuter la fonction rl_fcntl
    /*int mutex_unlock_result = pthread_mutex_unlock(&(d.f->mutex));
    bool once = true;
        while (rl_fcntl(d, F_SETLK, &fl) == -1) {
        if (!once)
            continue;
        once = false;
        if (errno != EAGAIN) {
            fprintf(stderr, "Process %d: rl_fcntl failed with errno != EAGAIN\n"
                            "Did you handle errno correctly?\n", getpid());
        }
        printf("Process %d: Waiting for lock\n", getpid());
        sleep(1);  // Add a small delay to avoid excessive looping
    }


    printf("Process %d gets the lock\n", getpid());
    sleep(3);

    fl.rl_type = UNLOCK;
    printf("Process %d releases the lock\n", getpid());
    if (rl_fcntl(d, F_SETLK, &fl) == -1) {
        fprintf(stderr, "Process %d: Error releasing lock\n", getpid());
    }*/

    if (fileOpened && rl_close(d) == 0) {
        printf("File closed successfully.\n");
        fileOpened = 0;
    } else {
        perror("Failed to close file.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
