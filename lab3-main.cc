#include "stdio.h"
#include "cache.h"
#include "memory.h"
#include "param.h"
#include <fstream>

using namespace std;

ifstream fin;
ofstream fout;

int main(int argc, char **argv) {
  int size, set, way, write_through, write_allocate;
  if(argc < 6)
  {
    printf("God! Not good enough!\n");
    return 1;
  }
  size = atoi(argv[1]);
  set = atoi(argv[2]);
  way = atoi(argv[3]);
  write_through = atoi(argv[4]);
  write_allocate = atoi(argv[5]);

  Memory m(NULL);
  Cache l1(size, set, way, write_through, write_allocate, &m);
  l1.SetLower(&m);

  StorageStats s;
  s.access_time = 0;
  m.SetStats(s);
  l1.SetStats(s);

  StorageLatency ml;
  ml.bus_latency = 6;
  ml.hit_latency = 100;
  m.SetLatency(ml);

  StorageLatency ll;
  ll.bus_latency = 3;
  ll.hit_latency = 10;
  l1.SetLatency(ll);

  int hit, time;
  char content[64];

  fin.open("./trace/1.trace", ios::in);
  char q;
  lint address;
  int requestNum = 0;
  while(fin >> q)
  {
    fin >> address;

    if(q == 'r')
    {
      l1.HandleRequest(address, 4, 1, content, hit, time);
    }
    else if(q == 'w')
    {
      l1.HandleRequest(address, 4, 0, content, hit, time);
    }
    else
    {
      printf("God! Undefined request!\n");
      return 2;
    }
    requestNum ++;
  }
  fin.close();

  printf("%d\t%d\t%d\n", hit, requestNum, time);

  //l1.HandleRequest(0, 0, 1, content, hit, time);
  //printf("Request access time: %dns\n", time);
  //l1.HandleRequest(1024, 0, 1, content, hit, time);
  //printf("Request access time: %dns\n", time);

  //l1.GetStats(s);
  //printf("Total L1 access time: %dns\n", s.access_time);
  //m.GetStats(s);
  //printf("Total Memory access time: %dns\n", s.access_time);
  return 0;
}
