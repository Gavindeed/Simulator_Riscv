#ifndef CACHE_CACHE_H_
#define CACHE_CACHE_H_

#include <stdint.h>
#include <memory.h>
#include "storage.h"
#include "param.h"

typedef struct CacheConfig_ {
  int size;
  int associativity;
  int set_num; // Number of cache sets
  int write_through; // 0|1 for back|through
  int write_allocate; // 0|1 for no-alc|alc
} CacheConfig;


class Cache: public Storage {
 public:
  Cache(int size, int set, int way, bool write_through, bool write_allocate, Storage *lower) 
  {
    this->size = size;
    this->set = set;
    this->way = way;
    this->block_size = size / (set * way);
    indexBits = 0;
    int s = set;
    while(s > 1)
    {
      indexBits ++;
      s = (s >> 1);
    }
    offsetBits = 0;
    int b = block_size;
    while(b > 1)
    {
      offsetBits ++;
      b = (b >> 1);
    }
    tagBits = 64 - offsetBits - indexBits;
    blocks = new char**[set];
    valid = new bool*[set];
    tags = new lint*[set];
    lastTimes = new lint*[set];
    dirty = new bool*[set];
    for(int i = 0; i < set; i++)
    {
      blocks[i] = new char*[way];
      for(int j = 0; j < way; j++)
      {
        blocks[i][j] = new char[block_size];
      }
      valid[i] = new bool[way];
      dirty[i] = new bool[way];
      //memset(valid[i], false, way*sizeof(bool));
      //memset(dirty[i], false, way*sizeof(bool));
      for(int j = 0; j < way; j++)
      {
        valid[i][j] = false;
        dirty[i][j] = false;
      }
      tags[i] = new lint[way];
      lastTimes[i] = new lint[way];
      for(int j = 0; j < way; j++)
      {
        lastTimes[i][j] = 0;
      }
      //memset(lastTimes[i], 0, way*sizeof(lint));
    }
    policy = 0;
    if(write_through == 0 && write_allocate == 1) policy = 1;
    this->lower_ = lower;
    nowTime = 0;
    //printf("cons %d\n", dirty[0][0]);
  }
  ~Cache() 
  {
    for(int i = 0; i < set; i++)
    {
      for(int j = 0; j < way; j++)
      {
        delete [] blocks[i][j];
      }
    }
    for(int i = 0; i < set; i++)
    {
      delete [] blocks[i];
      delete [] valid[i];
      delete [] tags[i];
      delete [] lastTimes[i];
    }
    delete [] blocks;
    delete [] tags;
    delete [] valid;
    delete [] lastTimes;
  }

  // Sets & Gets
  void SetConfig(CacheConfig cc);
  void GetConfig(CacheConfig cc);
  void SetLower(Storage *ll) { lower_ = ll; }
  // Main access process
  void HandleRequest(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time);

 private:
  // Bypassing
  int BypassDecision();
  // Partitioning
  bool PartitionAlgorithm(uint64_t addr, int bytes, int read, char *content);
  // Replacement
  int ReplaceDecision(bool ok);
  void ReplaceAlgorithm(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time);
  // Prefetching
  int PrefetchDecision();
  void PrefetchAlgorithm();

  CacheConfig config_;
  int size;
  int set;
  int way;
  int block_size;
  int policy;
  char ***blocks;
  lint **tags;
  lint **lastTimes;
  bool **valid;
  bool **dirty;
  int offsetBits;
  int indexBits;
  int tagBits;
  lint nowTime;
  Storage *lower_;
  DISALLOW_COPY_AND_ASSIGN(Cache);
};

#endif //CACHE_CACHE_H_ 
