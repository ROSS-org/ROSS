#include<stdio.h>

#define TSC_ALIGN(x) __attribute__ (( aligned(x) ))

typedef unsigned long int 
tsc_uint32_t
TSC_ALIGN(sizeof(unsigned long int));

typedef union
{
  signed long long int whole;
  unsigned long long int bits64;
  
  struct
  {
    tsc_uint32_t low;
    tsc_uint32_t high;
  }
  bits32;
}
tsc_uint64_t
TSC_ALIGN(sizeof(unsigned long long int));

/*
 * Global simulation time, starts at 0
 */
signed long long int g_sim_time;

/*
 * Hardware clock offset for computing simulation time
 */
signed long long int g_sim_time_offset;

/* 
 * read the Time-Stamp Counter 
 */
void read_tsc(tsc_uint64_t *buf);

int main(int argc, char **argv)
{
  int i = 0;
  int iter = 0;
  char input;
  tsc_uint64_t buffer1;
  tsc_uint64_t buffer2;

 again:

  printf("\nPlease enter the number of loop iterations: ");
  scanf("%d", &iter);
  printf("#iters = %d\n", iter);
  g_sim_time = 0;
  g_sim_time_offset = 0;

  i = 1;
  while (i <= iter)
    {
      read_tsc(&buffer2);
      
      printf("%-6s %4d \t\t %-8s %12Ld \t\t %-8s %7Ld \n", 
	     "ITER #", i, 
	     "Hardware time = ", buffer2.bits64, 
	     "tw_sim_time = ", g_sim_time);
      i++;
    }

  printf("\nAgain? (y/n) ");
  scanf("%s", &input);
  if(input == 'y')
    goto again;

  return 0;
}

void read_tsc(tsc_uint64_t *buf)
{
  __asm__ __volatile__ ("rdtsc" : "=a" (buf->bits32.low), 
			"=d" (buf->bits32.high) : : "eax", "edx");
  
  if (g_sim_time_offset == 0)
    {
      g_sim_time = 0;
      g_sim_time_offset = buf->whole;
    }  
  else
    g_sim_time = buf->whole - g_sim_time_offset;
}


