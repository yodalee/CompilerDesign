#include <stdio.h>
#include "genfunction.h"

int reg_number = 8;
int ARoffset;
FILE *fd;

void 
openfile(char *outputname) 
{
  fd = fopen(outputname, "w");
  if (fd == NULL) {
    fprintf(stderr, "open output file fail\n");
    return;
  }
}

void 
closefile() 
{
  fclose(fd);
}

void 
gen_prologue(char *name) 
{
  fprintf(fd ,"\tsw $ra, 0($sp)\n");
  fprintf(fd ,"\tsw $fp, -4($sp)\n");
  fprintf(fd ,"\tadd $fp, $sp, -4\n");
  fprintf(fd ,"\tadd $sp, $sp, -8\n");
  fprintf(fd ,"\tlw $2, _framesize_%s;\n", name);
  fprintf(fd ,"\tsub $sp, $sp, $2\n");
  fprintf(fd ,"_begin_%s:\n", name);
}

void
gen_globalvar(char *name)
{
  fprintf(fd, ".data\n");
  fprintf(fd, "_%s: .word 0\n", name);
}

void 
gen_epilogue(char *name, int framesize) 
{
  fprintf(fd ,"_end_%s\n", name);
  fprintf(fd ,"\tlw $ra, 4($fp)\n");
  fprintf(fd ,"\tadd $sp, $fp, 4\n");
  fprintf(fd ,"\tlw $fp, 0($fp)\n");
  if (strcmp(name, "main") == 0) {
    fprintf(fd ,"\tli $v0, 10\n");
    fprintf(fd ,"\tsyscall\n");
  } else {
    fprintf(fd ,"\tjr $ra\n");
  }
  fprintf(fd ,".data\n");
  fprintf(fd ,"\tframesize_%s: .word %d\n", name, framesize);
}

void 
gen_head(char *name) 
{
  fprintf(fd ,".text\n");
  fprintf(fd ,"%s:\n", name);
}

void
gen_const_str(int idx, char *content)
{
  fprintf(fd ,"m%d:.asciiz \"%s\"\n", idx, content);
}
