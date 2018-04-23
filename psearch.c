/**
 * psearch.c
 *
 * Performs a parallel directory search over the file system.
 *
 * Author: Tao Guo
 */
#define _GNU_SOURCE
#include <string.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

sem_t thread_semaphore;
char *dir = ".";
bool is_exact = false;

void print_usage(char *argv[]);
void *search_thread(void *opts);
void search(char *name, char *options);
void printer(bool is_exact, char *abs_path, char *options, char *compare);

// The helper function for print directory/file path.
void printer(bool is_exact, char *abs_path, char *options, char *compare) 
{
    if (is_exact == true) {
        if (strcasecmp(compare, options) == 0) {
            printf("%s\n", abs_path);
        }
    } else {
        if (strcasestr(compare, options) != NULL) {
            printf("%s\n", abs_path);
        }  
    }

}

// Search function.
// Contain search contains and search exact.
// Using recursive.
void search(char *name, char *options) 
{
    DIR *dir;
    struct dirent *entry;
    struct stat buffer;

    if(!(dir = opendir(name))){  
        printf("Cannot open directory: %s\n", name);  
        return;  
    }

    // Recursivly go through all the directories and subdirectories.
    while ((entry = readdir(dir)) != NULL) {
        char path[1024];
        char buf[PATH_MAX + 1];
        snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
        lstat(path, &buffer);
        char *abs_path = realpath(path, buf);
        if (entry->d_type == DT_DIR) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            printer(is_exact, abs_path, options, entry->d_name);

            // if (is_exact == true) {
            //     if (strcasecmp(entry->d_name, options) == 0) {
            //         printf("%s\n", abs_path);
            //     }
            // } else {
            //     if (strcasestr(entry->d_name, options) != NULL) {
            //         printf("%s\n", abs_path);
            //     }  
            // }

            search(path, options);
        } else {
            // if (is_exact == true) {
            //     if (strcasecmp(entry->d_name, options) == 0) {
            //         printf("%s\n", abs_path);
            //     }
            // } else {
            //     if (strcasestr(entry->d_name, options) != NULL) {
            //         printf("%s\n", abs_path);
            //     }  
            // }
            printer(is_exact, abs_path, options, entry->d_name);
        }
    }
    closedir(dir);

}

// Implement thread logic. 
// Call the search function.
void *search_thread(void *opts)
{
    search(dir, opts);

    sem_post(&thread_semaphore);
    
    return 0;
}

// Print usage messages.
void print_usage(char *argv[])
{
    printf("Usage: %s [-eh] [-d directory] [-t threads] "
            "search_term1 search_term2 ... search_termN\n" , argv[0]);
    printf("\n");
    printf("Options:\n"
           "    * -d directory    specify start directory (default: CWD)\n"
           "    * -e              print exact name matches only\n"
           "    * -h              show usage information\n"
           "    * -t threads      set maximum threads (default: # procs)\n");
    printf("\n");
}

int main(int argc, char *argv[])
{

    // Check the arguments. Avoid no agrs.
    if(argc < 2) {

        print_usage(argv);
        return EXIT_FAILURE;
    }

    int c;
    opterr = 0;

    pthread_t *thread;

    //Determine the number of cores online
    int num_threads = get_nprocs();
    int temp_num_threads = 0;

    /* Handle command line options */
    while ((c = getopt(argc, argv, "d:eht:")) != -1) {
        switch (c) {
            // Handle flags

            case 'd':

                dir = argv[optind - 1];
                break;

            case 'e':

                is_exact = true;
                break;

            case 't':

                temp_num_threads = atoi(argv[optind - 1]);
                if(temp_num_threads > 0) {
                    num_threads = temp_num_threads;
                } else {
                    printf("Invalid Thread Count\n");
                    return EXIT_FAILURE;
                }
                break;

            case 'h':

                print_usage(argv);
                return EXIT_FAILURE;

            case '?':
                 if (optopt == 'd') {
                    fprintf(stderr,
                            "Option -%c requires an argument.\n", optopt);
                } else if (optopt == 't') {
                    fprintf(stderr,
                            "Option -%c requires an argument.\n", optopt);
                } else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                } else {
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n", optopt);
                }
                return 1;
            default:
                abort();
        }
    }

    sem_init(&thread_semaphore, 0, num_threads);
    thread = malloc(sizeof(pthread_t) * (argc - optind + 1));

    int i;
    for (i = optind; i < argc; i++) {
        // Look for each search term. Once this is working correctly, you
        // should launch a new thread here to perform the search.
        sem_wait(&thread_semaphore);
        pthread_create(&thread[i - optind], NULL, search_thread, argv[i]);
        
    }

    int j;
    for (j= optind; j < argc; j++) {

        pthread_join(thread[j - optind], NULL);
    }

    free(thread);

    return 0;
}