#include "bwtsearch.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#define ASCII_MAX 128
#define K_ARRAY_SIZE 1024
#define BASE_RECORD_SIZE 20000


int fin;
int fout;
int npatterns;
char* patterns[3];

off_t size;
int C[ASCII_MAX];
int freq[ASCII_MAX];

int main(int argc, char **argv){
    //open bwt & index_file
    init_bwt(argv[1], argv[2]);

    //get the patterns
    patterns[0] = argv[3];
    npatterns = 1;

    if(argc > 4) {
        patterns[1] = argv[4];
        npatterns = 2;
    } 

    if(argc > 5) {
        patterns[2] = argv[5];
        npatterns = 3;
    }

    int first, last,num_matches, ret;
    first = last = ret = num_matches = 0;
    
    GHashTable **tables = (GHashTable **) malloc(npatterns * sizeof(GHashTable*));
    int i,j;
    for(i = 0; i < npatterns; i++) {
        tables[i] = g_hash_table_new(g_int_hash, g_int_equal);

        int first, last, num_matches;
        first = last = num_matches = 0;

        num_matches = backwards_search(patterns[i], strlen(patterns[i]), &first, &last);
        int *keys = (int*) malloc(num_matches * sizeof(int));
        for(j = 0; j < num_matches; j++) {
            keys[j] = first;
            g_hash_table_insert(tables[i], &keys[j], 0);
            first++;
        }
    }

    sort_hashmaps(&tables);

    int k = 0;
    while(check_matches_left(tables)) {
        k++;
        GHashTableIter iter;
        gpointer gkey;
        g_hash_table_iter_init(&iter, tables[0]);
        g_hash_table_iter_next(&iter, &gkey, NULL);
        int key = *((int*) gkey);

        GArray *arr = g_array_new(0, 0, sizeof(int));

        char *bstr;  
        ret = decode_backwards(key, &bstr, arr);
        if(!ret) {
            remove_matches_in_array(tables, arr);
            free_array(arr);
            continue;
        }

        char *fstr;  
        ret = decode_forwards(key, &fstr, arr);
        if(!ret) {
            remove_matches_in_array(tables, arr);
            free_array(arr);
            free(bstr);
            continue;
        }
        
        if(!check_array_match(arr, tables)) {
            remove_matches_in_array(tables,arr);
            free_array(arr);
            free(bstr);
            free(fstr);
            continue;
        }

        get_str(bstr, fstr);
        printf("%s\n", bstr);
        free(bstr);
        remove_matches_in_array(tables,arr);
        free_array(arr);
    }
    close(fin);
    close(fout);
}

void remove_matches_in_array(GHashTable **tables, GArray *arr){
    int data;
    for(int i = 0; i < arr->len; i++) {
        data = g_array_index(arr, gint, i);
        for(int j = 0; j< npatterns; j++) {
            g_hash_table_remove(tables[j], &data);
        }
    }
}

void sort_hashmaps(GHashTable ***tables){ 
    GHashTable **tabs = *tables;
    GHashTable *tmp;
    if(npatterns == 1) {
        return;
    }

    int swap = 1;
    while(swap) {
        swap = 0;
        for(int i = 0; i < npatterns - 1; i++) {
            if(g_hash_table_size(tabs[i]) > g_hash_table_size(tabs[i+1])){
                tmp = tabs[i];
                tabs[i] = tabs[i+1];
                tabs[i+1] = tmp;
                swap =1;
            }
        }
    }
}

//check to see that all hash tables still have mathces
int check_matches_left(GHashTable **tables){
    for(int i = 0; i < npatterns; i++){
        if(!g_hash_table_size(tables[i])) {
                return 0;
        }
    }
    return 1;
}

//check if all hash tables except the first one have matches
int check_array_match(GArray *arr, GHashTable **tables){
    int* matches = (int*) calloc(npatterns - 1 , sizeof(int));
    int data;
    for(int i = 0; i < arr->len; i++) {
        data = g_array_index(arr, gint, i);
        for(int j = 1; j< npatterns; j++) {
            if(g_hash_table_contains(tables[j], &data)) {
                matches[j-1] = 1;
            }
        }
    }

    for(int j = 0; j < (npatterns - 1); j++) {
        if(!matches[j]) {
            free(matches);
            return 0;
        }
    }
    free(matches);
    return 1;
}

//opens bwt file for reading and idx file for r/w
void init_bwt(char *bwt_file, char* idx_file){
    fin = open(bwt_file, O_RDONLY);

    //get BWT Size
    size = lseek(fin, 0, SEEK_END)+1;
    struct stat status;
    fstat(fin, &status);
    lseek(fin, 0, SEEK_SET);
    size = status.st_size;


    //read index file, if it exists
    if(!access(idx_file, O_RDONLY)){
        fout = open(idx_file, O_RDONLY);
        load_oarray_from_idx();
        load_farray_from_idx();
    } else {
        if(size > (K_ARRAY_SIZE << 4)) {
            fout = open(idx_file,O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        }
        if(fout == -1) {
            perror("ERROR creating index file: ");
        }
        read_bwt_file();
    }
}

void load_oarray_from_idx(){
    int ret;
    lseek(fout, -(ASCII_MAX * sizeof(int)), SEEK_END);
    ret = read(fout, C, (ASCII_MAX * sizeof(int))); 
}

void load_farray_from_idx(){
    int ret;
    lseek(fout, -(2 * ASCII_MAX * sizeof(int)), SEEK_END);
    ret = read(fout, freq, (ASCII_MAX * sizeof(int))); 
}

void save_karray_in_idx(){
    int ret;
    ret = write(fout, freq, ASCII_MAX * sizeof(int));
}

void save_oarray_in_idx(){
    int ret;
    ret = write(fout, C, (ASCII_MAX * sizeof(int)));
}

void save_farray_in_idx(){
    int ret;
    ret = write(fout, freq, (ASCII_MAX * sizeof(int)));
}

void read_bwt_file() {
    char c[K_ARRAY_SIZE];

    int i = 0;
    int ret;
    while((ret = read(fin,c,K_ARRAY_SIZE))){
        for(int j = 0; j < ret; j++) {
            freq[c[j]] += 1;
        } 

        i += ret;
        if(i == K_ARRAY_SIZE){
            i = 0;
            if((K_ARRAY_SIZE << 4)) {
                save_karray_in_idx();
            }
        }
    }

    //fill C array
    int j = 0;
    for(int i = 0; i < ASCII_MAX;i++){
        if(freq[i]){
            C[i] = j;
            j += freq[i];
        }
    }

    if(size > (K_ARRAY_SIZE << 4)) {
        save_farray_in_idx();
        save_oarray_in_idx();
    }
}

char read_l_char(int index){
    lseek(fin, index, SEEK_SET);
    char c;
    int ret = 0;
    while(ret != 1){
        ret = read(fin,&c, 1);
    }
    return c;
}

char* get_str(char* bstr, char* fstr){
    size_t blen = strlen(bstr);
    int i,j;
    i = 0;
    j = blen - 1;
    for(; i<j; i++, j--){
        char tmp = bstr[i];
        bstr[i] = bstr[j];
        bstr[j] = tmp;
    }
    size_t flen = strlen(fstr);
    j = 0;
    for(i = blen; j < flen; i++, j++) {
        bstr[i] = fstr[j];
    }
    bstr[i] = '\0';
    free(fstr);
}


char get_f_from_index(int index) {
    int j = -1;
    char c = -1;
    for(int i = 0; i < ASCII_MAX; i++){
        if((C[i] <= index) &&
            freq[i] &&
        (C[i] > j || j == -1)){
            j = C[i];
            c = i;
        }
    }
    return c;
}

int decode_forwards(int index, char** ostr, GArray* arr){
    int i = 0;
    char c;
    char *str;
    str = (char *) malloc(sizeof(char) * BASE_RECORD_SIZE );
    gint *element;

    /*printf("enter decode forwards\n");*/

    *ostr = str;
    char f_char = get_f_from_index(index);
    int delta = index - C[f_char];
    index = bwt_select(f_char, delta + 1);
    c = read_l_char(index);
    if(c == '[') {
        free(str);
        /*printf("exit decode forwards\n");*/
        return 0;
    }


    while(c != '[' ){
        if( c == ']') {
            free(str);
            /*printf("exit decode forwards\n");*/
            return 0;
        }
        str[i++] = c;
        f_char = get_f_from_index(index);
        delta = index - C[f_char];
        index = bwt_select(f_char, delta + 1);
        g_array_append_val(arr, index);
        c = read_l_char(index);
    }
    str[i] = '\0';
    /*printf("exit decode forwards\n");*/
    return i+1;
}

int decode_backwards(int index, char** ostr, GArray *arr){
    int i = 0;
    char c = 0;
    char *str;
    int rbracket_seen = 0;
    gint *element;
    /*printf("enter decode_backwards\n");*/

    str = (char *) malloc(sizeof(char) * BASE_RECORD_SIZE );
    *ostr = str;
    g_array_append_val(arr, index);
    c = read_l_char(index);
    str[i++] = c;
    if( c == ']') {
        rbracket_seen = 1;
    }

    while(c != '[' ){
        char tmp = read_l_char(index);
        index = C[tmp] + bwt_rank(tmp,index - 1);
        g_array_append_val(arr, index);
        tmp = read_l_char(index);
        c = tmp;
        str[i++] = c;
        if( c == ']') {
            rbracket_seen = 1;
        }
    }
    str[i] = '\0';

    /*printf("exit decode_backwards\n");*/
    if(!rbracket_seen) {
        free(str);
        return 0;
    }
    *ostr = str;
    return i+1;
}

void free_array(GArray *arr){
    g_array_free(arr, FALSE);
}

//returns number of pattern matches, sets first and last
int backwards_search(char *p, int plen, int *pfirst, int *plast) {
    int i = plen - 1;
    int first, last;
    char c;

    c = p[i];
    first = C[c] + 1;
    last = C[c] + freq[c];

    while((first <= last) &&
          (i >= 1)){
        c = p[--i];
        first = C[c] + bwt_rank(c, (first - 2)) + 1;
        last = C[c] + bwt_rank(c, last - 1);
    }

    if(first <= last) {
        *pfirst = first - 1;
        *plast = last - 1;
        return (last - first) + 1;
    } else {
        return 0;
    }
}

//returns the bin where c occurs #occ times
int find_kth_bin(char c, int occ, int *bin, int *matches) {
    int L, R, m, freq,prev_freq, next_freq;
    L = 0;
    R = ((size - 1) / K_ARRAY_SIZE);
    int result = -1;

    while(L < R) {
        m = L + (R - L)/2;
        if(m == 0) {
            *bin = 0;
            *matches = 0;
            return 0;
        }

        //look in m'th bin (Zero index)
        lseek(fout, ((m * ASCII_MAX) + c) * sizeof(int), SEEK_SET); 
        int ret = read(fout, &freq, sizeof(int));
        //look in m-th - 1 bin (Zero index)
        lseek(fout, (((m-1) * ASCII_MAX) + c) * sizeof(int), SEEK_SET);
        ret = read(fout, &prev_freq, sizeof(int));

        //look in m-th + 1 bin (Zero index)
        lseek(fout, (((m+1) * ASCII_MAX) + c) * sizeof(int), SEEK_SET);
        ret = read(fout, &next_freq, sizeof(int));

        /*printf("bs prev_f %d f %d next_f %d L %d m %d R %d\n",*/
                /*prev_freq, freq, next_freq, L, m, R);*/

        if(freq > occ) {
            if(prev_freq < occ) {
                *bin = m-1;
                *matches = prev_freq;
                return 1;
            }
            R = m;
        } else if(freq == occ) {
            //we can continue
            if(prev_freq == freq) {
                result = m;
                R = m;
            //we are done
            } else {
                *bin = m-1;
                *matches = prev_freq;
                return 1;
            }
        } else {
            if(next_freq >= occ) {
                *bin = m;
                *matches = freq;
                return 1;
            } else {
                L = m + 1;
            }
        }
    }
    *matches = result;
    return 1;
}

//currently n time, could be speed up by array with k bins
int bwt_select(char c, int occ){
    int times_seen = 0;
    int freqc = 0;
    int prev_freq = 0;
    int k = 0;
    int bin = 0;
    int ret = 0;

    /*printf("Select char %c, occ %d\n", c, occ);*/

    int seek = 0;
    if(size > K_ARRAY_SIZE) {
        seek = find_kth_bin(c, occ, &bin, &times_seen);
    }

    /*printf("Bin %d times_seen %d\n", bin, times_seen);*/

    if(seek) {
        lseek(fin, ((bin+1) * K_ARRAY_SIZE), SEEK_SET);
        ret = (bin+1) * K_ARRAY_SIZE;
    } else {
        lseek(fin, 0, SEEK_SET);
    }

    char bwt[K_ARRAY_SIZE];
    int r = read(fin, bwt, K_ARRAY_SIZE);
    
    for(int i = 0 ; i < r; i++) {
        if(bwt[i] == c) {
            times_seen += 1;

            if(times_seen == occ) {
                ret += i;
                return ret;
            }
        }
    }
    getchar();
    return -1;
}

//returns how many C has been seen at index
int bwt_rank(char c, int index){
    int k = index / K_ARRAY_SIZE;
    int delta = index - (k * K_ARRAY_SIZE);
    int times_seen = 0;
    int ret;
    char* rd = (char*)malloc(sizeof(char) * (delta + 1));
    lseek(fin, (k * K_ARRAY_SIZE),  SEEK_SET);
    ret = read(fin, rd, delta + 1);
    for(int i = 0 ; i < ret; i++) {
        if(rd[i] == c) {
            times_seen += 1;
        }
    }

    if(k) {
        int freqc;
        lseek(fout, (((k - 1) * ASCII_MAX) + c) * sizeof(int), SEEK_SET); 
        ret = read(fout, &freqc, sizeof(int));
        times_seen += freqc;
    }

    free(rd);
    return times_seen;
}
