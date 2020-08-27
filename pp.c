#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pp.h"

#ifdef COLOR_SUPPORT
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"
#endif

struct path_list{
    int n_entries, cap;
    char** lst;
};

struct path_list chop_commas(char* str){
    struct path_list ret;
    ret.lst = malloc(sizeof(char*)*500);
    char* beg = str, * com;
    ret.n_entries = 0;
    while((com = strchr(beg, ','))){
        *com = 0;
        ret.lst[ret.n_entries++] = strdup(beg);
        beg = com+1;
    }
    return ret;
}

void print_tree(char* tstr){
    struct path_list* paths = malloc(sizeof(struct path_list)*500);
    char* beg = tstr, * brk;
    int ind = 0;
    while((brk = strchr(beg, '|'))){
        *brk = 0;
        /*paths[ind++] = strdup(beg);*/
        paths[ind++] = chop_commas(beg);
        beg = brk+1;
    }
    /*paths[ind++] = strdup(beg);*/

    for(int i = 0; i < ind; ++i){
        /*printf("path %i:\n", i);*/
        for(int j = 0; j < paths[i].n_entries-1; ++j){
            /*printf("  %i: %s\n", j, paths[i].lst[j]);*/
            #ifdef COLOR_SUPPORT
            printf("%s%s<-%s", paths[i].lst[j], ANSI_RED, ANSI_RESET);
            #else
            printf("%s<-", paths[i].lst[j]);
            #endif
        }
        printf("%s\n", paths[i].lst[paths[i].n_entries-1]);
    }
}

#ifdef PP_MAIN
int main(int a, char** b){
    /*char*** paths = malloc(sizeof(char**)*500);*/
    print_tree(b[1]);

    return 0;
    #if 0

    printf("found %i |s\n", ind);
    for(int i = 0; i < ind; ++i){
        printf("path %i:\n", i);
        for(int j = 0; j < paths[i].n_entries; ++j){
            printf("  %i: %s\n", j, paths[i].lst[j]);
        }
    }

    return 0;
    int max_entrylen = 5;

    char shared;
    for(int i = 0; i < ind-1; ++i){
        for(int j = 0; j < max_entrylen; ++j){
            /*if(paths[i].n_entries <= j)continue;//hopefully to next i*/
            if(!strcmp(paths[i].lst[j], paths[i+1].lst[j])){
                printf("^");
            }
            else if(1)
                printf("%s", paths[i].lst[j]);
            printf("-\n");
            /*if(paths[i].lst[j])*/
        }
    }
    #endif
}
/*
 * 
 * struct node{
 *     struct node* parent, ** children;
 * };
*/
#endif
