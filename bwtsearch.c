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


int fin;
int fout;
int npatterns;
char* patterns[3];
/*char* p1; */
/*char* p2; */
/*char* p3; */

off_t size; //size of text
int* C; //ascii-max big, gives frequency of all chars
int* freq; //ascii-max big, gives frequency of all chars

#define ASCII_MAX 128
#define K_ARRAY_SIZE 200000
#define BASE_RECORD_SIZE 20000



int main(int argc, char **argv){
    
    //open bwt & index_file
    init_bwt(argv[1], argv[2]);

    //get the patterns
    patterns[0] = argv[3];
    npatterns = 1;

    if(argc > 3) {
        patterns[1] = argv[4];
        npatterns = 2;
    } 

    if(argc > 4) {
        patterns[2] = argv[5];
        npatterns = 3;
    }

    /*printf("printing c and freq");*/
#ifdef _DEBUG_
    print_c_and_freq();

    /*print_bwt();*/
#endif
    /*printf("size is %d occ[127] is %d freq[127] is %d sum is %d\n", size, C[127], freq[127], C[127]+freq[127]);*/

    int first, last,num_matches, ret;
    first = last = ret = num_matches = 0;
#ifdef DEBUG
    printf("start backwards_search on %s with length %lu\n", p1, strlen(p1));
#endif
    
    GHashTable **tables = (GHashTable **) malloc(npatterns * sizeof(GHashTable**));
    for(int i = 0; i < npatterns; i++) {
        tables[i] = g_hash_table_new(g_int_hash, g_int_equal);

        int first, last, num_matches;
        first = last = num_matches = 0;

        num_matches = backwards_search(patterns[i], strlen(patterns[i]), &first, &last);
        int *keys = (int*) malloc(num_matches * sizeof(int));
        int *values = (int*) malloc(num_matches * sizeof(int));
        for(int j = 0; j < num_matches; j++) {
            keys[j] = first;
            values[j] = first;
            g_hash_table_insert(tables[i], keys[j], values[j]);
            first++;
        }
    }

    //interate first hash_map
    GList *elements = g_hash_table_get_keys(tables[0]);
    while(g_hash_table_size(tables[0])) {
        int key = *((int*)g_list_first(elements)->data);
        

    }

    while(num_matches > 0) {
#ifdef _DEBUG_
        printf("processing match %d with first %d\n", num_matches, first);
#endif
        ret = decode_backwards(first, &bstr);
        if(!ret) {
            free(bstr);
            num_matches--;
            first++;
            continue;
        }
        ret = decode_forward(first, &fstr);
        if(!ret) {
            free(bstr);
            free(fstr);
            first++;
            num_matches--;
            continue;
        }

        get_str(bstr, fstr);
        num_matches--;
        first++;
        printf("%s\n", bstr);
        free(bstr);
    }

    close(fin);
    close(fout);
}

void print_bwt(){
    char *tmp = (char*)malloc(size);
    lseek(fin, 0, SEEK_SET);
    read(fin, tmp, size);
    printf("\n BWT \n");
    for( int i = 0; i < size; i++) {
        printf("%d %c\n", i, tmp[i]);
    }
}

//opens bwt file for reading and idx file for r/w
void init_bwt(char *bwt_file, char* idx_file){
    fin = open(bwt_file, O_RDONLY);
    size = lseek(fin, 0, SEEK_END)+1;
    struct stat status;
    fstat(fin, &status);

    //reset fp to start again
    lseek(fin, 0, SEEK_SET);
    size = status.st_size;

    alloc_tables();

    //the index file exist - read metadata
    if(!access(idx_file, O_RDONLY)){
        /*printf("file exist\n");*/
        fout = open(idx_file, O_RDONLY);
        if(fout == -1) {
            perror("ERROR opening index file: ");
        }
        /*if(size < (ASCII_MAX << 2)) {*/
            /*read_bwt_file();*/
        /*} else {*/
            load_oarray_from_idx();
            load_farray_from_idx();
        /*}*/
    //create index file
    } else {
        fout = open(idx_file,O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
        if(fout == -1) {
            perror("ERROR creating index file: ");
        }
        read_bwt_file();
    }
}

void alloc_tables(){ 
    freq = (int*)calloc(ASCII_MAX, sizeof(int));
    C = (int *)calloc(ASCII_MAX, sizeof(int));
}
    

void print_idx_file(){
    int tmp[ASCII_MAX * 2];
    int ret;

    lseek(fout, 0, SEEK_SET);
    ret = read(fout, tmp, (ASCII_MAX * sizeof(int) * 2)); 
    int j = 128;
    for(int i = 0; i < (ASCII_MAX); i++,j++) {
        printf("char %c C[%c] = %d, freq[%c] = %d\n", i, i, tmp[i], i, tmp[j]);
    }
    printf("ret was %d\n", ret);
}

void load_oarray_from_idx(){
    int ret;
    lseek(fout, -(ASCII_MAX * sizeof(int)), SEEK_END);
    ret = read(fout, C, (ASCII_MAX * sizeof(int))); 
    /*printf("ret was %d\n", ret);*/
}

void load_farray_from_idx(){
    int ret;
    lseek(fout, -(2 * ASCII_MAX * sizeof(int)), SEEK_END);
    ret = read(fout, freq, (ASCII_MAX * sizeof(int))); 
    /*printf("ret was %d\n", ret);*/
}

void save_karray_in_idx(){
    int ret;
    ret = write(fout, freq, ASCII_MAX * sizeof(int));
    /*printf("ret was %d\n", ret);*/
}

void save_oarray_in_idx(){
    int ret;
    ret = write(fout, C, (ASCII_MAX * sizeof(int)));
    /*printf("ret was %d\n", ret);*/
}

void save_farray_in_idx(){
    int ret;
    ret = write(fout, freq, (ASCII_MAX * sizeof(int)));
    /*printf("ret was %d\n", ret);*/
}

void read_bwt_file() {
    char c[K_ARRAY_SIZE];

    int i = 0;
    int ret;
    while((ret = read(fin,c,K_ARRAY_SIZE))){
        for(int j = 0; j < ret; j++) {
            freq[c[j]] += 1;
        } 

        /*printf("READ ret was %d\n", ret);*/

        i += ret;
        if(i == K_ARRAY_SIZE){
            i = 0;
            save_karray_in_idx();
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
    
    /*if( size > (ASCII_MAX<<2)) {*/
    /*printf("SAVING Arrays\n");*/
    save_farray_in_idx();
    save_oarray_in_idx();
    /*}*/
}

char read_l_char(int index){
    lseek(fin, index, SEEK_SET);
    char c;
    int ret = 0;
    while(ret != 1){
        /*printf("in read l index %d\n", index);*/
        ret = read(fin,&c, 1);
        if(ret != 1)
            getchar();
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
            /*printf("considered char %d with j = %d\n", i, j);*/
        }
    }
    /*printf("in get f, c = %c for index %d\n", c, index);*/
    
    return c;
}

int decode_forward(int index, char** ostr){
    int i = 0;
    char c;
    char *str;
    str = (char *) malloc(sizeof(char) * BASE_RECORD_SIZE );
    *ostr = str;
    char f_char = get_f_from_index(index);
    int delta = index - C[f_char];
    index = bwt_select(f_char, delta + 1);
    /*printf("in forward decode, f_char %c delta %d index %d\n", f_char, delta, index);*/
    /*c = L[index];*/
    c = read_l_char(index);
    if(c == '[') {
        return 0;
    }


    while(c != '['){
        if( c == ']') {
            return 0;
        }
        str[i++] = c;
        char f_char = get_f_from_index(index);
        int delta = index - C[f_char];
        index = bwt_select(f_char, delta + 1);
        /*printf("in forward decode, C[f_char] = %d f_char %c delta %d index %d\n", C[f_char], f_char, delta, index);*/
        /*c = L[index];*/
        c = read_l_char(index);
    }
    str[i] = '\0';
    /*printf("forward decode produced: %s\n", str);*/
    return i+1;
}

int decode_backwards(int index, char** ostr){
    int i = 0;
    char c = 0;
    char *str;
    int rbracket_seen = 0;
    str = (char *) malloc(sizeof(char) * BASE_RECORD_SIZE );
    *ostr = str;
#ifdef _DEBUG_
    printf("Starting decode backwards index is %d c = %c\n", index,read_l_char(index));
#endif
    c = read_l_char(index);
    str[i++] = c;
    if( c == ']') {
        rbracket_seen = 1;
    }

    while(c != '[' ){
        char tmp = read_l_char(index);
        index = C[tmp] + bwt_rank(tmp,index - 1);
#ifdef _DEBUG_
        printf("decode backwards index is %d c = %c\n", index,c);
#endif
        tmp = read_l_char(index);
        c = tmp;
        str[i++] = c;
        if( c == ']') {
            rbracket_seen = 1;
        }
    }


    str[i] = '\0';
#ifdef _DEBUG_
    printf("backward decode produced: %s\n", str);
#endif

    if(!rbracket_seen) {
        /*printf("backward decode return 0\n");*/
        return 0;
    }
    return i+1;
}

//returns number of pattern matches, sets first and last
int backwards_search(char *p, int plen, int *pfirst, int *plast) {
    int i = plen - 1;
    int first, last;
    char c;

    c = p[i];
    first = C[c] + 1;
    last = C[c] + freq[c];
#ifdef _DEBUG_
    printf("first is %d, last is %d\n", first, last);
#endif

    while((first <= last) &&
          (i >= 1)){
        c = p[--i];
        first = C[c] + bwt_rank(c, (first - 2)) + 1;
        last = C[c] + bwt_rank(c, last - 1);
#ifdef _DEBUG_
        printf("first is %d, last is %d\n", first, last);
#endif
    }

    if(first <= last) {
        *pfirst = first - 1;
        *plast = last - 1;
        return (last - first) + 1;
    } else {
        return 0;
    }
}

void print_c_and_freq(){
    for(int i = 0; i < ASCII_MAX; i++){
        printf("\n%c, %d frequency %d C %d",(char)i, C[i], freq[i], C[i]);
    }
}

//currently n time, could be speed up by array with k bins
int bwt_select(char c, int occ){
    int times_seen = 0;
    int freqc = 0;
    int prev_freq = 0;
    int k = 0;

    if(size > K_ARRAY_SIZE) {
        int ret = 0;
        lseek(fout, 0, SEEK_SET); 
        while (times_seen < occ) {
            int freqk[ASCII_MAX];
            ret = read(fout, freqk, ASCII_MAX * sizeof(int));
            if(ret != (ASCII_MAX * sizeof(int))) {
                printf("SELECT READ ERROR, ret is %d, k is %d\n", ret, k);
                getchar();
            }

            prev_freq = times_seen;
            times_seen = freqk[c];
            k += 1;
        }
    }
    if( times_seen == occ) {
        return (k * K_ARRAY_SIZE) - 1;
    }
    /*printf("BWTSELECT times seen %d, prev_freq %d\n", times_seen, prev_freq);*/

    int delta_freq = times_seen - prev_freq;
    times_seen -= delta_freq;

    if(k) {
        k -= 1;
        lseek(fin, (k * K_ARRAY_SIZE), SEEK_SET);
    } else {
        lseek(fin, 0, SEEK_SET);
    }

    char bwt[K_ARRAY_SIZE];
    int ret;
    ret = read(fin, bwt, K_ARRAY_SIZE);
    
    for(int i = 0 ; i < ret; i++) {
        if(bwt[i] == c) {
            times_seen += 1;

            if(times_seen == occ) {
#ifdef _DEBUG_
                /*printf("return index %d\n", (i + (k * K_ARRAY_SIZE)));*/
#endif
                return (i + (k * K_ARRAY_SIZE));
            }
        }
    }
    printf("ERROR:select return -1 for %d seen %d times\n", c, occ);
    printf("ERROR:times_seen %d, delta_freq %d\n", times_seen, delta_freq);
    getchar();
    return -1;
}

//returns how many C has been seen at index
//cuurently n time, could be speed up by array with k bins
int bwt_rank(char c, int index){
#ifdef _DEBUG_
    printf("bwt rank c is %c index %d\n", c, index);
#endif

    int k = index / K_ARRAY_SIZE;
    int delta = index - (k * K_ARRAY_SIZE);


#ifdef _DEBUG_
    printf("delta is %d, k is %d\n", delta, k);
#endif

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
#ifdef _DEBUG_
    printf("times seen %c in delta is %d\n", c, times_seen);
#endif

    if(k) {
        int freqc;
        lseek(fout, (((k - 1) * ASCII_MAX) + c) * sizeof(int), SEEK_SET); 
        read(fout, &freqc, sizeof(int));
        times_seen += freqc;
#ifdef _DEBUG_
        printf("times seen in k is %d\n", freqc);
#endif
    }

    free(rd);
    return times_seen;
}
