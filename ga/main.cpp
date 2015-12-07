/*
 Copyright Adrian Lozano 2015
 GNU License
 */
#include <pthread.h>
//MacOSX doesn't have barriers, this is fixed with pthread_barrier.h
//obtained from https://github.com/ademakov/DarwinPthreadBarrier
#include "pthread_barrier.h"
#include <time.h>
#include <iostream>
#include <string>
#include <cstdlib>

#define NUMBER_ORGANISMS 200
#define ALLELES 127
#define MUTATION_RATE 100
#define NUM_THREADS 4


//typedef struct _thread_data_t {
//    int tid;
//    double stuff;
//} thread_data_t;
//
///* thread function */
//void *thr_func(void *arg) {
//    thread_data_t *data = (thread_data_t *)arg;
//    
//    printf("hello from thr_func, thread id: %d\n", data->tid);
//    
//    pthread_exit(NULL);
//}


using namespace std;

struct Environment {
    int num_genes;
    int num_organisms;
    int total_fitness;
    char *target;
    char **organisms;
    char **next_gen_organisms;
    int *organismsfitness;
    int num_threads;
    int done;
    pthread_barrier_t begin_eval;
    pthread_barrier_t begin_new_gen;
    pthread_barrier_t end_eval;
    pthread_barrier_t end_new_gen;
};



typedef struct ThreadInfo {
    int t_start;
    int t_end;
    int t_max;
    int t_total_fit;
    struct Environment *env;
}ThreadInfo;

int ChooseOrganism(struct Environment *env) {
    int org;
    if (env->total_fitness == 0) {
        return rand() % env -> num_organisms;
    }
    while (1) {
        org = rand() % env -> num_organisms;
        if ( (rand() % (env -> total_fitness)) <= (env -> organismsfitness[org]) ) {
            return org;
        }
    }
    return 0;
    
//    int organism;
//    int runningTotal;
//    int randomSelectPoint;
//        
//    runningTotal = 0;
//    randomSelectPoint = rand() % (env->total_fitness + 1);
//    
//        
//    for(organism=0; organism<NUMBER_ORGANISMS; ++organism){
//        runningTotal += env->organismsfitness[organism];
//        if(runningTotal >= randomSelectPoint) return organism;
//    }
//    return 0;
}



void *CreateNewGeneration(void *args) {
    struct ThreadInfo *t_inf = (struct ThreadInfo*) args;
    int start = t_inf -> t_start;
    int end = t_inf -> t_end;
    int num_genes = t_inf -> env -> num_genes;
    int parent_one;
    int parent_two;
    
    pthread_barrier_wait(&t_inf->env->begin_new_gen);
    while ( !(t_inf->env->done) ) {
        for (int org = start; org < end; ++org) {
            // parents might be the same
            parent_one = ChooseOrganism(t_inf -> env);
            parent_two = ChooseOrganism(t_inf -> env);
//            int crossoverpoint = rand() % num_genes;
            int crossoverpoint = num_genes/2;
            for (int gene = 0; gene < num_genes; ++gene) {
//                if ( gene < crossoverpoint ) {
                if ( rand() % 2 == 0 ) {
                   // cout << "p1";
                     t_inf -> env -> next_gen_organisms[org][gene] = t_inf -> env -> organisms[parent_one][gene];
                } else {
                    //cout<<"p2";
                     t_inf -> env -> next_gen_organisms[org][gene] = t_inf -> env -> organisms[parent_two][gene];
                }
                
                if ( rand() % MUTATION_RATE == 0) {
                     t_inf -> env -> next_gen_organisms[org][gene] = rand() % ALLELES + 1;
                }
            }
        }
        
        for (int org = start; org < end; ++org) {
            for (int gene = 0; gene < num_genes; ++gene) {
                t_inf -> env -> organisms[org][gene] =  t_inf -> env -> next_gen_organisms[org][gene];
                //cout<<t_inf -> env -> organisms[org] << endl;
            }
        }
        int delete_this_var = 0;
        pthread_barrier_wait(&t_inf->env->end_new_gen);
        pthread_barrier_wait(&t_inf->env->begin_new_gen);
    }
    
    pthread_exit(NULL);
}

void *EvaluatePopulation(void *args) {
    struct ThreadInfo *t_inf = (struct ThreadInfo*) args;
    int start = t_inf -> t_start;
    int end = t_inf -> t_end;
    int num_genes = t_inf -> env -> num_genes;
    char **organisms = t_inf -> env -> organisms;
    char *target = t_inf -> env -> target;
    
    int org_fitness;
    int t_total_fitness;
    int thread_max;
    
    pthread_barrier_wait(&t_inf->env->begin_eval);
   // cout<<"before while " << !(t_inf->env->done) << endl;
    while ( !(t_inf->env->done) ) {
        //cout<<"waiting..." << endl;
        
        thread_max = 0;
        t_total_fitness = 0;
        org_fitness = 0;
        for (int org = start; org < end; ++org) {
            org_fitness = 0;
            for (int gene = 0; gene < num_genes; ++gene) {
                if (organisms[org][gene] == target[gene]) {
                    org_fitness++;
                }
            }
            
            t_total_fitness += org_fitness;
            t_inf -> env -> organismsfitness[org] = org_fitness;
            thread_max = (thread_max > org_fitness) ? thread_max : org_fitness;
        }
        
        t_inf -> t_total_fit = t_total_fitness;
        t_inf -> t_max = thread_max;
        pthread_barrier_wait(&t_inf->env->end_eval);
        pthread_barrier_wait(&t_inf->env->begin_eval);
    }
    
    
    pthread_exit(NULL);
}

void *InitializeOrganisms(void *args) {
    ThreadInfo *t_inf = (ThreadInfo*) args;
    int start = t_inf -> t_start;
     int end = t_inf -> t_end;
     int num_genes = t_inf -> env -> num_genes;
     char **organisms = t_inf -> env -> organisms;
    
    
//     start = 1; // t_inf -> t_start;
//    int end = 0; //t_inf -> t_end;
//    int num_genes = 0; //t_inf -> env -> num_genes;
//    char **organisms = NULL; //t_inf -> env -> organisms;
    
    //cout <<"hello from init orgs" << endl;
    //printf("hello from thr_func, thread id: ");
    char *buffer;
    
    buffer = static_cast<char*> (malloc(sizeof(char) * num_genes + 1));
    
    // todo get args
    for (int i = start; i < end; ++i) {
        /* unix only
         // http://stackoverflow.com/questions/7427832/c-fastest-way-to-fill-a-buffer-with-random-bytes
         int randStream = open("/dev/urandom", O_RDONLY);
         read(randStream, buffer, num_genes);
         // */
        
        //* windows and unix
        for (int j = 0; j < num_genes; j++) {
            buffer[j] = (rand() % ALLELES) + 1;
        }
        // */
        buffer[num_genes] = 0;
        snprintf(t_inf -> env -> organisms[i], num_genes + 1, "%s", buffer);
    }
    
    free(buffer);
    
    pthread_exit(NULL);
}


void InitThreadInfo(const int &num_threads, struct Environment &env, struct ThreadInfo **thread_args2) {
    struct ThreadInfo* thread_args;
    thread_args = static_cast<struct ThreadInfo*> (malloc(sizeof(struct ThreadInfo) * num_threads));
    for (int i = 0; i < num_threads; ++i) {
        thread_args[i].t_start = (env.num_organisms * i) / num_threads;
        thread_args[i].t_end = (env.num_organisms * (i + 1)) / num_threads;
        thread_args[i].env = &env;
    }
    *thread_args2 = thread_args;
}


void InitEnvironment(struct Environment &env){
    
    string str;
    int num_org;
    int num_threads;
    
    // UI
    cout<< "Enter target string: \n";
    getline(cin, str);
    cout<< "Enter number of organisms: \n";
    cin >> num_org;
    cout<< "Number of threads to use: \n";
    cin >> num_threads;
    
    
    
    // INIT ENVIRONMENT
    srand(time(NULL));
    env.target = static_cast<char*> (malloc(sizeof(char) * (str.length() + 1)));
    snprintf(env.target, str.length() + 1, "%s", str.c_str());
    
    env.num_organisms = num_org;
    env.num_genes = str.length();
    
    env.num_threads = num_threads;
    
    // barrier info:
    // http://stackoverflow.com/questions/14485708/how-to-kill-all-child-threads
    // barriers can be reused, they reset to original init (check comments):
    // http://stackoverflow.com/questions/12907479/c-pthread-synchronize-function
    // alias: http://stackoverflow.com/a/12907534/3509398
    pthread_barrier_init(&env.end_eval, NULL, num_threads + 1);
    pthread_barrier_init(&env.end_new_gen, NULL, num_threads + 1);
    pthread_barrier_init(&env.begin_eval, NULL, num_threads + 1);
    pthread_barrier_init(&env.begin_new_gen, NULL, num_threads + 1);
    // finishing condition
    env.done = 0;
    
    env.organisms = static_cast<char**> (malloc( sizeof(char*) * num_org ));
    for (int org = 0; org < num_org; ++org) {
        env.organisms[org] = static_cast<char*> (malloc ( sizeof(char) * env.num_genes + 1)); //(env.num_genes + 1)));
    }
    
    env.next_gen_organisms = static_cast<char**> (malloc( sizeof(char*) * num_org ));
    for (int org = 0; org < num_org; ++org) {
        env.next_gen_organisms[org] = static_cast<char*> (malloc ( sizeof(char) * env.num_genes + 1)); //(env.num_genes + 1)));
    }
    
    env.organismsfitness = static_cast<int*> (malloc( sizeof(int) * num_org ));
}




int main() {
    
    int target_fitness;
    struct ThreadInfo *thread_args;
    struct Environment env;
    //pthread_attr_t attr;
    pthread_t *eval_threads;
    pthread_t *new_gen_threads;
    pthread_t *init_threads;
    
    InitEnvironment(env);
    
    // INIT THREAD INFO
    InitThreadInfo(env.num_threads, env, &thread_args);
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    eval_threads = (pthread_t *) (malloc(sizeof(pthread_t) * env.num_threads));
    init_threads = (pthread_t *) (malloc(sizeof(pthread_t) * env.num_threads));
    new_gen_threads = (pthread_t *) (malloc(sizeof(pthread_t) * env.num_threads));
    
    // INIT ORGANISMS THREADED
    cout <<"creating organisms... \n";
    for (int t = 0; t < env.num_threads; t++) {
        pthread_create(&init_threads[t], NULL, InitializeOrganisms, &thread_args[t]);
    }
    for (int t = 0; t < env.num_threads; t++) {
       // cout <<"finished organism " << t << " \n";
        pthread_join(init_threads[t], NULL);
    }
    
    
    //these threads won't join yet, controlled by barriers
    
    for (int t = 0; t < env.num_threads; t++) {
        pthread_create(&eval_threads[t], NULL, EvaluatePopulation, &thread_args[t]);
    }
    
    for (int t = 0; t < env.num_threads; t++) {
        pthread_create(&new_gen_threads[t], NULL, CreateNewGeneration, &thread_args[t]);
    }
    int print_freq;
    cout<< "Fittest organism in generation printing frecuency (print every __ generation): \n";
    cin >> print_freq;
    int counter = 0;
    char bestIndex;
    
    //start first evaluation
   // cout << "1" << endl;
    pthread_barrier_wait(&env.begin_eval);
    //cout << "2";
    int max_fitness;
    target_fitness = env.num_genes;
    int generations = 1;
    
    while (1) {
        // all evaluations should be done
        pthread_barrier_wait(&env.end_eval);
        //cout<<"unwait";
        // get maximum fitness
        max_fitness = thread_args[0].t_max;
        env.total_fitness = 0;
        for (int i = 1; i < env.num_threads; ++i) {
            env.total_fitness += thread_args[i].t_total_fit;
            max_fitness = thread_args[i].t_max > max_fitness ? thread_args[i].t_max : max_fitness;
        }
        counter++;
        if (counter == print_freq && print_freq != 0)
        {
            counter = 0;
            for (int j = 0; j < env.num_organisms; ++j)
            {
                if(env.organismsfitness[j] == max_fitness){
                    cout<< "Generation: " << generations << "\tBest organism " << env.organisms[j] << '\n';
                    break;
                }
            }
        }
        generations++;
        if ( max_fitness == target_fitness ) {
            env.done = 1;
            break;
        }
        // start new generation
        pthread_barrier_wait(&env.begin_new_gen);
        //wait for new generation creation to finish before starting evaluations
        pthread_barrier_wait(&env.end_new_gen); //waits for all CretaeNewGen() to finish
        //start evaluations
        pthread_barrier_wait (&env.begin_eval);
    }
    
    //waiting threads, should quit immedietly
    pthread_barrier_wait (&env.begin_eval);
    pthread_barrier_wait(&env.begin_new_gen);
    // join threads
    for (int t = 0; t < env.num_threads; t++) {
        pthread_join(eval_threads[t], NULL);
    }
    for (int t = 0; t < env.num_threads; t++) {
        pthread_join(new_gen_threads[t], NULL);
    }
    
    //report to user
    
    cout << "Done!\n" << "Found target string \"" << env.target << "\" in " << generations << " generations.\n";
    
    
    
    // CLEAN UP MEMORY
    
    //clean up threads
    free(init_threads);
    free(new_gen_threads);
    free(eval_threads);
    
    //clean up barriers
    pthread_barrier_destroy(&env.begin_eval);
    pthread_barrier_destroy(&env.begin_new_gen);
    pthread_barrier_destroy(&env.end_eval);
    pthread_barrier_destroy(&env.end_new_gen);
    
    free(thread_args);
    
    for (int org = 0; org < env.num_organisms; ++org) {
        free(env.organisms[org]);
        free(env.next_gen_organisms[org]);
    }
    
    free(env.organisms);
    free(env.next_gen_organisms);
    free(env.organismsfitness);
    free(env.target);
    
    return 0;
}




