#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hash.h>
#include <list.h>
#include <util.h>
#include <sched_util.h>
#include <sched_kw.h>
#include <sched_file.h>


struct sched_file_struct {
  hash_type  *month_hash;
  hash_type  *one_line_kw;
  hash_type  *kw_types;
  list_type  *kw_list;
  int         next_date_nr;
};




sched_file_type * sched_file_alloc() {
  sched_file_type * sched_file = malloc(sizeof *sched_file);
  {
    hash_type *month_hash = hash_alloc(24);
    hash_insert_int(month_hash , "JAN" , 0);
    hash_insert_int(month_hash , "FEB" , 1);
    hash_insert_int(month_hash , "MAR" , 2);
    hash_insert_int(month_hash , "APR" , 3);
    hash_insert_int(month_hash , "MAY" , 4);
    hash_insert_int(month_hash , "JUN" , 5);
    hash_insert_int(month_hash , "JUL" , 6);
    hash_insert_int(month_hash , "AUG" , 7);
    hash_insert_int(month_hash , "SEP" , 8);
    hash_insert_int(month_hash , "OCT" , 9);
    hash_insert_int(month_hash , "NOV" ,10);
    hash_insert_int(month_hash , "DEC" ,11);
    sched_file->month_hash = month_hash;
  }
  {
    hash_type * one_line_kw = hash_alloc(10);
    hash_insert_int(one_line_kw , "INCLUDE" , 1);
    sched_file->one_line_kw = one_line_kw;
  }
  {
    hash_type * kw_types = hash_alloc(10);
    hash_insert_int(kw_types , "DATES"    , DATES);
    hash_insert_int(kw_types , "WCONHIST" , WCONHIST);
    sched_file->kw_types = kw_types;
  }

  sched_file->next_date_nr = 0;
  sched_file->kw_list      = list_alloc();
  
  return sched_file;
}


sched_kw_type * sched_file_add_kw(sched_file_type *sched_file , const char *kw_name) {
  sched_type_enum type;
  bool            one_line_kw = false;
  sched_kw_type   *kw;

  if (hash_has_key(sched_file->kw_types , kw_name)) 
    type = hash_get_int(sched_file->kw_types , kw_name);
  else {
    type = UNTYPED;
    if (hash_has_key(sched_file->one_line_kw , kw_name))
      one_line_kw = true;
    else
      one_line_kw = false;
  }
  kw = sched_kw_alloc(kw_name , type , one_line_kw , &sched_file->next_date_nr);
  list_append_list_owned_ref(sched_file->kw_list , kw , sched_kw_free__); 
  return kw;
}
  




void sched_file_free(sched_file_type *sched_file) {
  list_free(sched_file->kw_list);
  hash_free(sched_file->month_hash);
  hash_free(sched_file->one_line_kw);
  free(sched_file);
}






void sched_file_parse(sched_file_type * sched_file , const char * filename) {
  int             lines , linenr;
  char          **line_list;
  sched_kw_type  *active_kw;
  bool            one_line_kw;
  bool            cont;
  
  sched_util_parse_file(filename , &lines , &line_list);
  linenr      = 0;
  one_line_kw = false;
  active_kw   = NULL;
  cont        = true;
  do {
    const char *line = line_list[linenr];
    if (strncmp(line , "END" , 3) == 0) {
      fprintf(stderr,"%s: Warning: found END statement in file:%s before actual file \n",__func__ , filename);
      fprintf(stderr,"end reached, a risk for premature ending in the parsing of the schedule file.\n");
      cont = false;
    } else {
      if (active_kw == NULL) {
	const char * kw_name = line;
	if (hash_has_key(sched_file->one_line_kw , kw_name))
	  one_line_kw = true;
	else
	  one_line_kw = false;
	active_kw = sched_file_add_kw(sched_file , kw_name);
      } else {
	if (line[0] == '/') 
	  active_kw = NULL;
	else 
	  sched_kw_add_line(active_kw , line , sched_file->month_hash);
	
	if (one_line_kw)
	  active_kw = NULL;
      }
      
      linenr++;
      if (linenr == (lines - 1)) {
	if (strncmp(line_list[linenr] , "END" , 3) == 0)
	  cont = false;
	else {
	  fprintf(stderr,"%s: something is rotten when parsing: %s not END on last line - aborting \n" , __func__ , filename);
	  abort();
	}
      }
    }
  } while (cont);
  util_free_string_list(line_list , lines);
}


void sched_file_fprintf(const sched_file_type * sched_file , const char * file) {
  FILE *stream = fopen(file , "w");
  list_node_type *list_node = list_get_head(sched_file->kw_list);
  
  while (list_node != NULL) {
    const sched_kw_type * sched_kw = list_node_value_ptr(list_node);
    sched_kw_fprintf(sched_kw , stream);
    list_node = list_node_get_next(list_node);
  }
  fprintf(stream , "END\n");

  fclose(stream);
}



int main (void) {
  sched_file_type *s = sched_file_alloc();
  
  sched_file_parse(s , "NEWS");
  
  sched_file_fprintf(s , "NEWS2");

  sched_file_free(s);
  return 0;
}
