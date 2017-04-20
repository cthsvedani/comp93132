#ifndef _BWTSEARCH_H_
#define _BWTSEARCH_H_
#include <glib.h>



void read_bwt();
int bwt_select(char c, int occ);
int bwt_rank(char c, int index);
void print_bwt();
void read_c();
void print_c_and_freq();
void read_rank();
void print_rank();
int backwards_search(char *p, int plen, int *pfirst, int *plast);
int decode_backwards(int index, char** ostr, GList **olist);
int decode_forwards(int index, char** ostr, GList **olist);
char get_f_from_index(int index);
char* get_str(char* bstr, char* fstr);
void init_bwt(char *bwt_file, char* idx_file);
void load_oarray_from_idx();
void load_farray_from_idx();
void save_idx(int fout);
void read_bwt_file();
void save_oarray_in_idx();
void save_karray_in_idx();
void save_farray_in_idx();
char read_l_char(int index);
void print_idx_file();
void alloc_tables();
int int_sort (gconstpointer a, gconstpointer b);
int check_matches_left(GHashTable **tables);
void free_list(GList *list);
void remove_matches_in_list(GHashTable **tables, GList *list);
int check_key_matches(int key, GHashTable **tables);
void sort_hashmaps(GHashTable ***tables);
int check_list_match(GList *list, GHashTable **tables);
void destroy_key(gpointer data);
int find_kth_bin(char c, int index);

#endif
