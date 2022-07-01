#include "cachelab.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <assert.h>

#define HIT 0
#define MISS 1
#define EVICTION 2

const char * status_msg[] = {"hit", "miss", "miss eviction"};
unsigned int statistics[3]; //unsigned int num_hits unsigned int num_misses unsigned int num_evictions
enum OP {load, store};

typedef unsigned long word_t;
typedef unsigned long actime_t;

typedef struct {
  word_t tag;
  bool valid;
  bool dirty;
  actime_t times;
}CacheLine;

CacheLine ** cache;
unsigned int cache_size;
unsigned int group_size;


unsigned int s, E, b;
word_t tag_mask, gidx_mask;
char * trace_file;


FILE * output; // /dev/null or stdout

void helper_msg();
void parse_args(int, char**);
void get_gidx_tag(word_t addr, word_t * gidx, word_t * tag);
int access_cache(word_t gidx, word_t tag, enum OP op);


int main(int argc, char **argv)
{
  output = fopen("/dev/null", "w");
  // parse args
  parse_args(argc, argv);

  FILE * ft = fopen(trace_file, "r");

  if (!ft)
  {
    perror("fopen");
    exit(1);
  }

  // build cache
  cache_size = (1 << s) * E;
  group_size = E;
  cache = (CacheLine **)malloc(cache_size * sizeof(CacheLine *));
  for (int i = 0; i < cache_size; i++)
  {
    cache[i] = (CacheLine*)malloc(sizeof(CacheLine));
    cache[i]->valid = false;
    cache[i]->times = 0;
  }

  //set tag_mask and gidx_mask
  word_t tmp = 1 << b;
  for (int i = 0; i < s; i++)
  {
    gidx_mask |= tmp;
    tmp <<= 1;
  }
  int n = 8 * sizeof(word_t) - s - b;
  for (int i = 0; i < n; i++)
  {
    tag_mask |= tmp;
    tmp <<= 1;
  }
  
  char c;
  long addr;
  int width;
  while (fscanf(ft, " %c %lx,%d\n", &c, &addr, &width) != EOF)
  {
    if (c == 'I')
      continue;
	// get tag gidx
    word_t gidx, tag;
    get_gidx_tag(addr,&gidx, &tag);
    fprintf(output, "%c %lx,%d", c, addr, width);
    int res;
    switch (c)
    {
      case 'L':
        res = access_cache(gidx, tag, load);
        fprintf(output, " %s", status_msg[res]);
        statistics[res]++;
        break;
      case 'S':
        res = access_cache(gidx, tag, store);
        fprintf(output, " %s", status_msg[res]);
        statistics[res]++;
        break;
      case 'M':
        res = access_cache(gidx, tag, load) + access_cache(gidx, tag, store);
        fprintf(output, " %s %s", status_msg[res], status_msg[0]);
        statistics[res]++;
        statistics[0]++;
        break;
      default:
        break;
    }
    fprintf(output, " \n");
  }
  statistics[1] += statistics[2];
  printSummary(statistics[0], statistics[1], statistics[2]);
  fclose(ft); 

  return 0;
}


void helper_msg()
{
  printf("Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n");
  printf("Options:\n");
  printf("\t-h\tPrint this help message.\n");
  printf("\t-v\tOptional verbose flag.\n");
  printf("\t-s <num>\tNumber of set index bits.\n");
  printf("\t-E <num>\tNumber of lines per set.\n");
  printf("\t-b <num>\tNumber of block offset bits.\n");
  printf("\t-t <file>\tTrace file.\n");
  printf("\n");
  printf("Examples:\n");
  printf("\tlinux> ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n");
  printf("\tlinux> ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void parse_args(int argc, char *argv[]) {
    int o;
    const char *optstring = "hvs:E:b:t:"; // 有三个选项-abc，其中c选项后有冒号，所以后面必须有参数
    while ((o = getopt(argc, argv, optstring)) != -1) {
        switch (o) {
            case 's':
              // printf("opt is a, oprarg is: %s\n", optarg);
              s = atoi(optarg);
              break;
            case 'E':
              E = atoi(optarg);
              break;
            case 'b':
              // printf("opt is b, oprarg is: %s\n", optarg);
              b = atoi(optarg);
              break;
            case 't':
              // printf("opt is t, oprarg is: %s\n", optarg);
              trace_file = optarg;
              break;
            case 'v':
              output = fdopen(1, "w");
              break;
            case '?':
            case 'h':
              helper_msg();
              exit(0);
        }
    }
}

void get_gidx_tag(word_t addr, word_t * gidx, word_t * tag)
{
  *gidx = (addr & gidx_mask) >> b;
  *tag = (addr & tag_mask) >> (s + b);
}


int access_cache(word_t gidx, word_t tag, enum OP op)
{
  int start = gidx * group_size;
  int end = start + group_size;
  bool hasSpace = false;
  int i; 
  int res; // hit miss or eviction
  int idx = -1; // index of line which contains data

  // weather hit
  for (i = start; i < end; i++)
  { 
    hasSpace |= !cache[i]->valid;
    if (cache[i]->valid && cache[i]->tag == tag)
    {
      idx = i;
      res = HIT;
      break;
    }
  }

  // no hit
  if (idx == -1)
  {
    // choose a victim
    if (!hasSpace)
    { 
      idx = start;
      for (i = start + 1; i < end; i++)
      {
        if (cache[i]->times < cache[idx]->times)
          idx = i;
      }
      res = EVICTION;
    }

    // has space
    else
    {
  	  for (i = start; i < end; i++)
      {
        if (!cache[i]->valid)
        {
          cache[i]->valid = true;
          idx = i;
          res = MISS;
          break;
        }
      }
    }
    cache[idx]->times = 0;
    cache[idx]->tag = tag;
  }

  if (op == store)
		cache[idx]->dirty = true;
  
  for (int i = 0; i < cache_size; i++)
    cache[i]->times >>= 1;
  actime_t x = 1;
  cache[idx]->times |=  x<< (8 * sizeof(actime_t) - 1);

  return res;
}