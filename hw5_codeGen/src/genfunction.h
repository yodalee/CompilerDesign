#ifndef CODEGEN_H_
#define CODEGEN_H_ value

typedef struct stringbuf stringbuf;
struct stringbuf {
  stringbuf *next;
  char *str;
} /* optional variable list */;

FILE *fd;
stringbuf *strbuf;

void gen_prologue(char *name);
void gen_epilogue(char *name, int framesize);
void gen_head(char *name);
void gen_reg();
void get_offset();
void gen_const_str(int idx, char *content);


#endif /* end of include guard: CODEGEN_H_*/


