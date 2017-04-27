#ifndef _BWTSEARCH_H_
#define _BWTSEARCH_H_
#include <glib.h>

void remove_matches_in_array(GHashTable **tables, GArray *arr);
void sort_hashmaps(GHashTable ***tables);
int check_matches_left(GHashTable **tables);
int check_array_match(GArray *arr, GHashTable **tables);
void init_bwt(char *bwt_file, char* idx_file);
void load_oarray_from_idx();
void load_farray_from_idx();
void save_karray_in_idx();
void save_oarray_in_idx();
void save_farray_in_idx();
void read_bwt_file() ;
char read_l_char(int index);
char* get_str(char* bstr, char* fstr);
char get_f_from_index(int index) ;
int decode_forwards(int index, char** ostr, GArray* arr);
int decode_backwards(int index, char** ostr, GArray *arr);
void free_array(GArray *arr);
int backwards_search(char *p, int plen, int *pfirst, int *plast) ;
int find_kth_bin(char c, int occ, int *bin, int *matches);
int bwt_select(char c, int occ);
int bwt_rank(char c, int index);



#endif
