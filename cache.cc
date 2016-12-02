#include "cache.h"
#include "def.h"
#include <memory.h>

void Cache::HandleRequest(uint64_t addr, int bytes, int read,
                          char *content, int &hit, int &time) {
  //hit = 0;
  //time = 0;
  //printf("debug1 %d\n", dirty[0][0]);
  // Bypass?
  if (!BypassDecision()) {
    //printf("in\n");
    bool ok = PartitionAlgorithm(addr, bytes, read, content);
    // Miss?
    if (ReplaceDecision(ok)) {
      // Choose victim
      ReplaceAlgorithm(addr, bytes, read, content, hit, time);
    } else {
      // return hit & time
      hit = 1;
      time += latency_.bus_latency + latency_.hit_latency;
      stats_.access_time += latency_.bus_latency + latency_.hit_latency;
      return;
    }
  }
  // Prefetch?
  if (PrefetchDecision()) {
    PrefetchAlgorithm();
  } else {
    // Fetch from lower layer
    int lower_hit, lower_time;
    lower_->HandleRequest(addr, bytes, read, content,
                          lower_hit, lower_time);
    hit = 0;
    time += latency_.bus_latency + lower_time;
    stats_.access_time += latency_.bus_latency;
  }
}

int Cache::BypassDecision() {
  return FALSE;
}

bool Cache::PartitionAlgorithm(uint64_t addr, int bytes, int read, char *content) {
  lint tag = (addr >> (indexBits + offsetBits));
  lint index = ((addr >> offsetBits) & ((1 << indexBits) - 1));
  lint offset = (addr & ((1 << offsetBits) - 1));
  for(int i = 0; i < way; i++)
  {
    if(tags[index][i] == tag && valid[index][i])
    {
      if(read)
      {
        memcpy(content, blocks[index][i]+offset, bytes);
        nowTime ++;
        lastTimes[index][i] = nowTime;
        return true;
      }
      else
      {
        if(policy == 1)
        {
          memcpy(blocks[index][i]+offset, content, bytes);
          nowTime ++;
          lastTimes[index][i] = nowTime;
          dirty[index][i] = TRUE;
          return true;
        }
        else
        {
          memcpy(blocks[index][i]+offset, content, bytes);
          nowTime ++;
          lastTimes[index][i] = nowTime;
          dirty[index][i] = TRUE;
          int h, t;
          lower_->HandleRequest(addr, bytes, read, content, h, t);
          return true;
        }
      }
    }
  }
  return false;
}

int Cache::ReplaceDecision(bool ok) {
  return !ok;
  //return FALSE;
}

void Cache::ReplaceAlgorithm(uint64_t addr, int bytes, int read,
                     char *content, int &hit, int &time){
  //printf("replace\n", addr);
  //printf("debug %d\n", dirty[0][0]);
  lint tag = (addr >> (indexBits + offsetBits));
  lint index = ((addr >> offsetBits) & ((1 << indexBits) - 1));
  lint offset = (addr & ((1 << offsetBits) - 1));
  //printf("replace %lu %lu %lu\n", tag, index, offset);
  if(read)
  {
    //printf("read\n");
    int w = 0;
    lint lt = 10000000000;
    for(int i = 0; i < way; i++)
    {
      if(!valid[index][i])
      {
        w = i;
        break;
      }
      if(lastTimes[index][i] < lt)
      {
        lt = lastTimes[index][i];
        w = i;
      }
    }
    //printf("read-in index: %d w: %d addr: %llx\n", index, w, addr);
    //printf("re1 %d %d %d\n", dirty[index][w], index, w);
    if(dirty[index][w])
    {
      uint64_t raddr = 0;
      raddr += (index << offsetBits);
      raddr += (tags[index][w] << (offsetBits + indexBits));
      //printf("raddr: %llx index: %lu tag: %llx\n", raddr, index, tags[index][w]);
      int h, t;
      lower_->HandleRequest(raddr, block_size, 0, blocks[index][w], h, time);
    }
    //printf("%llx %lu\n", addr, offset);
    //printf("before\n");
    int h;
    lower_->HandleRequest(addr-offset, block_size, 1, blocks[index][w], hit, time);
    //printf("re2\n");
    nowTime ++;
    lastTimes[index][w] = nowTime;
    tags[index][w] = tag;
    valid[index][w] = TRUE;
    dirty[index][w] = FALSE;
    memcpy(content, blocks[index][w]+offset, bytes);
  }
  else
  {
    //printf("write\n");
    if(policy == 1)
    {
      int w = 0;
      lint lt = 10000000000;
      for(int i = 0; i < way; i++)
      {
        if(!valid[index][i])
        {
          w = i;
          break;
        }
        if(lastTimes[index][i] < lt)
        {
          lt = lastTimes[index][i];
          w = i;
        }
      }
      if(dirty[index][w])
      {
        uint64_t raddr = 0;
        raddr += (index << offsetBits);
        raddr += (tags[index][w] << (offsetBits + indexBits));
        int h, t;
        lower_->HandleRequest(raddr, block_size, 0, blocks[index][w], h, time);
      }
      //printf("before\n");
      int h;
      lower_->HandleRequest(addr-offset, block_size, 1, blocks[index][w], hit, time);
      nowTime ++;
      lastTimes[index][w] = nowTime;
      tags[index][w] = tag;
      valid[index][w] = TRUE;
      dirty[index][w] = TRUE;
      memcpy(blocks[index][w]+offset, content, bytes);
    }
    else
    {
      int h;
      lower_->HandleRequest(addr, bytes, 0, content, h, time);
    }
  }
  
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

