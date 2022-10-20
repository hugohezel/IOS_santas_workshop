#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <time.h>

#define OUTPUT_FILE "proj2.out"

/*
* @ file proj2.case
* @ brief Program pre projekt na prácu s procesmi a semaformi
* @author Hugo Hežel | xhezel00
* @date 27.4.2021
*/

// STRUCTURE FOR SHARED MEMORY
typedef struct {
    int number_elves;
    int number_reindeers;
    int time_elves_work;
    int time_reindeers_chill;
    int activity_counter;
    int waiting_reindeers;
    int waiting_elves;
    int workshop_closed;
    sem_t mutex;
    sem_t santa_sleeps;
    sem_t reindeers_hitch;
    sem_t elves_getting_help;
    sem_t elves_help_queue;
    sem_t reindeers_all_hitched;
    sem_t santa_helping_elves;
} env;

// DECLARE VARIABLE FOR OUTPUT FILE
FILE *file;

/*
* @brief Function that prints program's usage after typing parameter -h or --help
*/
void usage(){
    printf("\n");
    printf("./proj2.case NE NR TE TR\n");
    printf("\n");
    printf("NE - number of elfs\n");
    printf("   - 0 < NE < 1000\n\n");
    printf("NR - number of reindeers\n");
    printf("   - 0 < NR < 20\n\n");
    printf("TE - the maximum time in milliseconds that elf works alone\n");
    printf("   - 0 <= TE <= 1000\n\n");
    printf("TR - the maximum time in milliseconds that reindeer returns home from vacation\n");
    printf("   - 0 <= RE <= 1000\n");
    printf("\n");
}

/*
* @brief Function for checking entry parameters
* @param argc Int representing number of entry parameters
* @param argv Array of entry parameters
* @return 1 if all parameters are fine, 0 if not
*/
int check_entries(int argc, char const **argv){

    if( argc != 5 ){
        return 0;
    }

    char *ptr;

    int argv1 = strtol(argv[1], &ptr, 10);

    if( strlen(ptr) != 0 ){
        return 0;
    }

    int argv2 = strtol(argv[2], &ptr, 10);

    if( strlen(ptr) != 0 ){
        return 0;
    }

    int argv3 = strtol(argv[3], &ptr, 10);

    if( strlen(ptr) != 0 ){
        return 0;
    }

    int argv4 = strtol(argv[4], &ptr, 10);

    if( strlen(ptr) != 0 ){
        return 0;
    }


    if( argv1 <= 0 || argv1 >= 1000 ){
        return 0;
    }

    if( argv2 <= 0 || argv2 >= 20 ){
        return 0;
    }

    if( argv3 < 0 || argv3 > 1000 ){
        return 0;
    }

    if( argv4 < 0 || argv4 > 1000 ){
        return 0;
    }

    return 1;

}

/*
* @brief Function prints out the message called by process.
* @param env Environment structure attached to shared memory.
* @param item_id Function counts with printing item_id if it's non-zero. If item_id is equal 0, function ignores this parameter.
* @param mutex Indicates wheter function needs to lock mutex or it is already using it. If mutex equals 0, function doesn't need to lock mutex. Else it is required.
*/
void print_message(env *env, char *message, int item_id, int mutex){

    if( mutex == 0 ){

        if( item_id == 0 ){
            fprintf(file, message, env->activity_counter);
            fflush( file );
        }else{
            fprintf(file, message, env->activity_counter, item_id);
            fflush( file );
        }

        env->activity_counter++;

    }else{

        sem_wait( (&env->mutex) );

        if( item_id == 0 ){
            fprintf(file, message, env->activity_counter);
            fflush( file );
        }else{
            fprintf(file, message, env->activity_counter, item_id);
            fflush( file );
        }

        env->activity_counter++;

        sem_post( (&env->mutex) );

    }

}

/*
* @brief Function runs santa's process
*
*/
void santa_process(env *north_pole){

    while(1){

        // GO TO SLEEP
        print_message(north_pole, "%d: Santa: going to sleep\n", 0, 1);

        // WAIT/SLEEP UNTIL SOMEONE WAKES UP SANTA AND FOR SHARED MEMORY ACCESS
        sem_wait( (&north_pole->santa_sleeps) );
        sem_wait( (&north_pole->mutex) );

        if( north_pole->waiting_reindeers >= north_pole->number_reindeers ){

            // IF THERE ARE ALL REINDEERS WAITING TO GET HITCHED, CLOSE WORKSHOP
            north_pole->workshop_closed = 1;
            print_message(north_pole, "%d: Santa: closing workshop\n", 0, 0);

            // SIGNAL EVERY REINDEER THAT HE CAN GET HITCHED
            for( int y = 0; y < north_pole->number_reindeers; y++ ){
                sem_post( (&north_pole->reindeers_hitch) );
            }

            // REALEASE SHARED MEMORY ACCESS SEMAPHORE
            sem_post( (&north_pole->mutex) );

            // WAIT UNTIL ALL REINDEERS GET HITCHED AND FOR SHARED MEMORY ACCESS
            sem_wait( (&north_pole->reindeers_all_hitched) );
            sem_wait( (&north_pole->mutex) );

            // START CHRISTMAS
            print_message(north_pole, "%d: Santa: Christmas started\n", 0, 0);

            // SIGNAL TO EVERY ELF THAT IS WAITING OUTSIDE THE WORKSHOP
            for( int k = 0; k < north_pole->waiting_elves; k++ ){
                sem_post( (&north_pole->elves_help_queue) );
            }

            // RELEASE SHARED MEMORY ACCESS
            sem_post( (&north_pole->mutex) );

            // END PROCESS
            exit(0);

        }else if( north_pole->waiting_elves == 3 ){

            // IF THERE ARE 3 ELVES WAITING OUTSIDE WORKSHOP TO GET HELP
            print_message(north_pole, "%d: Santa: helping elves\n", 0, 0);

            // HELP EVERY ELF
            for( int z = 0; z < 3; z++ ){
                sem_post( (&north_pole->elves_help_queue) );
            }

        }

        // RELEASE SHARED MEMORY ACCESS AND WAIT UNTIL ALL ELVES GOT HELPED
        sem_post( (&north_pole->mutex) );
        sem_wait( (&north_pole->santa_helping_elves) );

        // THEN RUN WHILE LOOP AGAIN

    }

}

/*
* @brief Function runs elf's process
*
*/
void elf_process(env *north_pole, int e){

    // INITIALIZE MESSAGE
    print_message(north_pole, "%d: Elf %d: started\n", e, 1);

    while(1){

        // SIMULATE ELF'S WORK
        usleep( 1000 * (rand() % ( north_pole->time_elves_work + 1) ) );

        // WAIT TO JOIN QUEUE FOT GETTING HELP AND TO GET SHARED MEMORY ACCESS
        sem_wait( (&north_pole->elves_getting_help) );
        sem_wait( (&north_pole->mutex) );

        print_message( north_pole, "%d: Elf %d: need help\n", e, 0);

        // IF WORKSHOP IS CLOSED
        if( north_pole->workshop_closed ){
            print_message(north_pole, "%d: Elf %d: taking holidays\n", e, 0);
            sem_post( (&north_pole->elves_getting_help) );
            sem_post( (&north_pole->mutex) );
            exit(0);
        }

        north_pole->waiting_elves = north_pole->waiting_elves + 1;

        // IF THERE ARE 3 ELVES IN QUEUE
        if( north_pole->waiting_elves == 3 ){
            // WAKE UP SANTA
            sem_post( (&north_pole->santa_sleeps) );
        }else{
            // LET ANOTHER ELF TO JOIN QUEUE
            sem_post( (&north_pole->elves_getting_help) );
        }

        // RELEASE SHARED MEMORY ACCESS SEMAPHORE
        sem_post( (&north_pole->mutex) );

        // ELVES ARE GETTING HELP HERE
        sem_wait( (&north_pole->elves_help_queue) );
        sem_wait( (&north_pole->mutex) );

            if( north_pole->workshop_closed ){
                print_message(north_pole, "%d: Elf %d: taking holidays\n", e, 0);
                sem_post( (&north_pole->elves_getting_help) );
                sem_post( (&north_pole->mutex) );
                exit(0);
            }else{
                print_message(north_pole, "%d: Elf %d: get help\n", e, 0);
            }

            north_pole->waiting_elves--;

            // LAST ELF SIGNALS SANTA THAT HE CAN GO TO SLEEP
            if( north_pole->waiting_elves == 0 ){
                sem_post( (&north_pole->elves_getting_help) );
                sem_post( (&north_pole->santa_helping_elves) );
            }

        // RELEASE SHARED MEMORY ACCESS SEMAPHORE
        sem_post( (&north_pole->mutex) );

    }

}

/*
* @brief Function runs reindeer's process
*
*/
void reindeer_process(env *north_pole, int r){

    // INITIALIZE MESSAGE
    print_message(north_pole, "%d: RD %d: rstarted\n", r, 1);

    // SIMULATE BEING ON VACATION
    usleep( 1000 * (rand() % ( north_pole->time_reindeers_chill + 1) ) );

    // RETURN FROM VACATION MESSAGE
    print_message(north_pole, "%d: RD %d: return home\n", r, 1);

    // WAIT FOR ACCESS TO SHARED MEMORY
    sem_wait( (&north_pole->mutex) );

    north_pole->waiting_reindeers++;

    // IF ALL REINDEERS ARE BACK FROM VACATION
    if( north_pole->waiting_reindeers == north_pole->number_reindeers ){

        // WAKE UP SANTA
        sem_post( (&north_pole->santa_sleeps) );

    }

    // RELEASE SEMAPHORE FOR ACCESSING TO SHARED MEMORY
    sem_post( (&north_pole->mutex) );

    // WAIT UNTIL CURRENT REINDEER IS HITCHED AND FOR ACCESS TO SHARED MEMORY
    sem_wait( (&north_pole->reindeers_hitch) );
    sem_wait( (&north_pole->mutex) );

    north_pole->waiting_reindeers--;
    print_message(north_pole, "%d: RD %d: get hitched\n", r, 0);

    // SIGNAL THAT ALL REINDEERS ARE HITCHED
    if( north_pole->waiting_reindeers == 0 ){
        sem_post( (&north_pole->reindeers_all_hitched) );
    }

    // RELEASE SEMAPHORE FOR ACCESSING TO SHARED MEMORY
    sem_post( (&north_pole->mutex) );

    // END PROCESS
    exit(0);

}

/*
* @brief Function sets deffault values in shared memory
*
*/
void setup_env(env *north_pole, int ne, int nr, int te, int tr){

    // NASTAVENIE SEVERNEHO POLU PRE ZACIATOK
    north_pole->number_elves = ne;
    north_pole->number_reindeers = nr;
    north_pole->time_elves_work = te;
    north_pole->time_reindeers_chill = tr;
    north_pole->activity_counter = 1;
    north_pole->waiting_reindeers = 0;
    north_pole->waiting_elves = 0;
    north_pole->workshop_closed = 0;

    // INICIALIZOVANIE SEMAFOROV
    sem_init( (&north_pole->mutex), 80, 1 );
    sem_init( (&north_pole->santa_sleeps), 80, 0 );
    sem_init( (&north_pole->reindeers_hitch), 80, 0 );
    sem_init( (&north_pole->elves_getting_help), 80, 1 );
    sem_init( (&north_pole->elves_help_queue), 80, 0 );
    sem_init( (&north_pole->reindeers_all_hitched), 80, 0 );
    sem_init( (&north_pole->santa_helping_elves), 80, 0 );

}

/*
* @brief Function closes all opened sources
*
*/
void close_sources( env *north_pole, int shmid ){

    // ZATVORENIE SUBORU
    fclose( file );

    // VYMAZANIE SEMAFOROV
    sem_destroy( (&north_pole->mutex) );
    sem_destroy( (&north_pole->santa_sleeps) );
    sem_destroy( (&north_pole->reindeers_hitch) );
    sem_destroy( (&north_pole->elves_getting_help) );
    sem_destroy( (&north_pole->elves_help_queue) );
    sem_destroy( (&north_pole->reindeers_all_hitched) );
    sem_destroy( (&north_pole->santa_helping_elves) );

    // VYMAZANIE PREPOJENIA NA SHARED MEMORY
    shmdt(north_pole);

    // VYMAZANIE ALLOKOVANEJ SHARED MEMORY
    shmctl(shmid, IPC_RMID, NULL);

}



/*
* @brief Main function
*
*/
int main(int argc, char const *argv[]) {

    // CHECK PARAMETERS
    // IF HELP IS REQUESTED
    if( ( argc == 2 ) && ( strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0 ) ){

        // PRINT HELP
        usage();
        return 0;

    }else if( ! check_entries( argc, argv ) ){

        // WRONG PARAMETERS GIVEN
        fprintf(stderr, "Wrong input of parameters. Use parameters -h or --help for help.\n");
        exit(1);

    }

    // SAVE GIVEN PARAMETERS INTO VARIABLES
    int ne = atoi( argv[1] );
    int nr = atoi( argv[2] );
    int te = atoi( argv[3] );
    int tr = atoi( argv[4] );

    // GENERATE SEED FOR RAND() FUNCTION
    srand( time(0) );

    // THIS VARIABLE WILL POINT TO SHARED MEMORY
    env *north_pole;

    // ALLOCATE SHARED MEMORY
    int shmid = shmget(6969, sizeof(env), IPC_CREAT | 0660);

    // CHECK IF ALLOCATION ENDED CORRECTLY
    if( shmid == -1 ){

        fprintf(stderr, "Error while allocating shared memory.");
        exit(1);

    }

    // ATTACH ALLOCATED SHARED MEMORY
    north_pole = shmat(shmid, NULL, 0);

    // CHECK IF ATTACHING ENDED CORRECTLY
    if( north_pole == (env *)-1 ){

        fprintf(stderr, "Error while attaching shared memory.");
        shmctl(shmid, IPC_RMID, NULL);
        exit(1);

    }

    // SET DEFFAULT VALUES IN SHARED MEMORY
    setup_env(north_pole, ne, nr, te, tr);

    // OPEN FILE WHERE OUTPUT SHOULD BE WRITTEN
    file = fopen(OUTPUT_FILE, "w");

    // CHECK IF FILE HAS BEEN OPENED CORRECTLY
    if( file == NULL ){
        fprintf(stderr, "Error while opening file.");
        close_sources( north_pole, shmid );
        exit(1);
    }

    // CREATE SANTA'S PROCESS
    int santa_proc_id = fork();

    if( santa_proc_id == 0 ){

        // RUN CODE FOR SANTA'S PROCESS
        santa_process(north_pole);

    }else if( santa_proc_id < 0 ){
        fprintf(stderr, "Error while making santa's process.");
        exit(1);
    }

    // CREATE ELVES' PROCESSES
    for( int e = 1; e <= north_pole->number_elves; e++ ){

        int elf_process_id = fork();

        if( elf_process_id == 0 ){

            // RUN ELF'S PROCESS
            elf_process(north_pole, e);

        }else if( elf_process_id < 0 ){
            fprintf(stderr, "Error while making elf's process.");
            exit(1);
        }

    }

    // CREATE REINDEERS' PROCESSES
    for( int r = 1; r <= north_pole->number_reindeers; r++ ){

        int reindeer_process_id = fork();

        if( reindeer_process_id == 0 ){

            // RUN REINDEER'S PROCESS
            reindeer_process(north_pole, r);

        }else if( reindeer_process_id < 0 ){
            fprintf(stderr, "Error while making reindeer's process.");
            exit(1);
        }

    }

    // WAIT UNTIL ALL PROCESSES ARE DONE WITH WORK
    for( int x = 0; x < (north_pole->number_elves + north_pole->number_reindeers + 1); x++ ){
        waitpid(-1, NULL, 0);
    }

    // CLOSE ALL SOURCES
    close_sources( north_pole, shmid );

    // END PROGRAM
    return 0;

}

/*** END OF FILE proj2.c ***/
