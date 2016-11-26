#include "cache.h"
#include "def.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

int SetBitNum(uint64_t num){
    int bit = 0;
    while(num != 1){
        int t = num & 1;
        if (t == 1){
            printf("input not pow(2)\n");
            exit(0);
        }
        num = num >> 1;
        bit ++;
    }
    return bit;
}

Cache::Cache(int size, int set, int way, bool write_through, bool write_allocate, Storage *storage){
    //printf("init cache\n");
    config_.size = size;
    config_.associativity = way;
    config_.set_num = set;
    config_.write_allocate = write_allocate;
    config_.write_through = write_through;
    config_.block_size = size / way / set;
    config_.block_bit = SetBitNum(config_.block_size);
    config_.index_bit = SetBitNum(config_.set_num);
    config_.tag_bit = 64 - config_.index_bit - config_.block_bit;
    //printf("tag_bit: %d index_bit: %d block_bit: %d\n", config_.tag_bit, config_.index_bit, config_.block_bit);
    //printf("size %d, associativity %d, set_num %d\n", size, way, set);
    lower_ = storage;
    block_queue = new int*[config_.set_num];
    for(int i = 0; i < config_.set_num; ++ i){
        block_queue[i] = new int[config_.associativity];
        for (int j = 0; j < config_.associativity; ++ j)
            block_queue[i][j] = j;
    }

    cache_addr = new Block*[set];
    for (int i = 0; i < set; ++ i){
        cache_addr[i] = new Block[way];
        for (int j = 0; j < way; ++ j){
            Block tmp;
            tmp.valid = FALSE;
            tmp.dirty = FALSE;
            tmp.content = new char[config_.block_size];
            cache_addr[i][j] = tmp;
        }
    }

}

Cache::~Cache(){
    for (int i = 0; i < config_.set_num; ++ i){
        for (int j = 0; j < config_.associativity; ++ j)
            delete cache_addr[i][j].content;
        delete cache_addr[i];
    }
    for (int i = 0; i < config_.set_num; ++ i)
        delete block_queue[i];
    delete block_queue;
    delete cache_addr;
}

void Cache::HandleRequest(uint64_t addr, int total_bytes, int read,
                          char *content, int &hit, int &time) {
    
    printf("cache1\n");

    hit = 0;
    time = 0;
    uint64_t offset = addr & ((1 << config_.block_bit) - 1);
    uint64_t index = (addr >> config_.block_bit) & ((1 << config_.index_bit) - 1);
    uint64_t tag = addr >> (config_.block_bit + config_.index_bit);
    printf("addr: %llx, bytes: %d\n", addr, total_bytes);
    printf("tag: %llx index: %llx offset: %llx \n", tag, index, offset);
    //if (offset + bytes > config_.block_size){
    //    printf("content not in a single block\n");
    //    exit(0);
    //}
    //int content_offset = 0;
    while(1){
        int bytes;
        if (total_bytes == 0)
            break;
        if(offset + total_bytes > config_.block_size){
            bytes = config_.block_size - offset;
        }
        else
            bytes = total_bytes;
        total_bytes -= bytes;
        printf("tag: %llx index: %llx offset: %llx bytes: %d total_bytes: %d\n", tag, index, offset, bytes, total_bytes);

        printf("cache2\n");
        // dertermine whether hit
        int position = 0;
        for (int i = 0; i < config_.associativity; ++ i){
            if (cache_addr[index][i].tag == tag && cache_addr[index][i].valid == TRUE){
                hit = 1;
                position = i;
                break;
            }
        }

        printf("cache3\n");
        int lower_hit = -1, lower_time = 0;
        if (read == 1){
            if (hit == 1)
                HitCache(index, position);
            else{
                //printf("cache3.1\n");
                printf("block_size:%d\n", config_.block_size);
                char *lower_content = new char[config_.block_size];
                //printf("cache3.2\n");
                //int lower_hit, lower_time;
                uint64_t lower_addr = addr - offset;
                lower_->HandleRequest(lower_addr, config_.block_size, 1,
                    lower_content, lower_hit, lower_time);
                //printf("cache3.3\n");
                position = ReplacePlace(index, tag, lower_content);
                //printf("cache3.4\n");
                delete lower_content;
            }
            printf("cache4\n");
            memcpy(content, cache_addr[index][position].content + offset, bytes);

            for (int i = 0; i < bytes; ++ i)
                content[i] = cache_addr[index][position].content[offset + i];
        }
        else{
            if (hit == 1){
                //printf("cache3.1\n");
                HitCache(index, position);
                //printf("cache3.2\n");
                WriteCache(index, position, offset, bytes, content);
                //printf("cache3.3\n");
                if (config_.write_through == 1){
                    //int lower_hit, lower_time;
                    lower_->HandleRequest(addr, bytes, 0, 
                        content, lower_hit, lower_time);
                }
                //printf("cache3.4\n");
            }
            else{
                //printf("cache3.5\n");
                if (config_.write_allocate == 1){
                    //printf("cache3.6\n");
                    printf("block_size:%d\n", config_.block_size);
                    char *lower_content = new char[config_.block_size];
                    //int lower_hit, lower_time;
                    uint64_t lower_addr = addr - offset;
                    printf("lower_addr: %llx\n", lower_addr);
                    lower_->HandleRequest(lower_addr, config_.block_size, 1,
                        lower_content, lower_hit, lower_time);
                    //printf("cache3.7\n");
                    position = ReplacePlace(index, tag, lower_content);
                    WriteCache(index, position, offset, bytes, content);
                    //printf("cache3.8\n");
                    if (config_.write_through == 1){
                        int lower_hit, lower_time;
                        lower_->HandleRequest(addr, bytes, 0, 
                            content, lower_hit, lower_time);
                    }
                    delete lower_content;
                    //printf("cache3.9\n");
                }
                else{
                    //int lower_hit, lower_time;
                    lower_->HandleRequest(addr, bytes, 0, 
                        content, lower_hit, lower_time);
                }
            }
        }
        printf("cache5\n");
        if (hit == 1){
            time += latency_.bus_latency + latency_.hit_latency;
            stats_.access_time += time;
        }
        else{
            time += latency_.bus_latency + lower_time;
            stats_.access_time += latency_.bus_latency;
        }

        offset = 0;
        index ++;
        if (index == config_.set_num){
            index = 0;
            tag ++;
        }
        content += bytes;
        addr += bytes;
    printf("cache6\n");
    
    }
    printf("\n");
}

int Cache::ReplacePlace(uint64_t index, uint64_t tag, char* content){
    //printf("replace0 index %x tag %x\n", index, tag);
    int position = block_queue[index][0];
    for (int i = 1; i < config_.associativity; ++ i){
        //printf("%d\n", i);
        block_queue[index][i - 1] = block_queue[index][i];
    }
    block_queue[index][config_.associativity - 1] = position;

    //printf("replace1");
    if (config_.write_through == 0 && cache_addr[index][position].valid == TRUE && cache_addr[index][position].dirty == TRUE){
        int lower_hit, lower_time;
        uint64_t lower_addr = (cache_addr[index][position].tag << (config_.block_bit + config_.index_bit))
            + (index << (config_.block_bit)); 
        lower_->HandleRequest(lower_addr, config_.block_size, 0,
                cache_addr[index][position].content, lower_hit, lower_time);
    }

    //printf("replace2");
    //memcpy(cache_addr[index][position].content, content, config_.block_size);
    for (int i = 0; i < config_.block_size; ++ i)
        cache_addr[index][position].content[i] = content[i];
    cache_addr[index][position].dirty = FALSE;
    cache_addr[index][position].valid = TRUE;
    cache_addr[index][position].tag = tag;
    return position;
}

void Cache::WriteCache(uint64_t index, uint64_t position, uint64_t offset, int bytes, char* content){
    for (int i = 0; i < config_.block_size; ++ i)
        cache_addr[index][position].content[offset + i] = content[i];
    cache_addr[index][position].dirty = TRUE;
}

void Cache::HitCache(uint64_t index, uint64_t position){
    for (int i = 0; i < config_.associativity; ++ i){
        if (block_queue[index][i] == position){
            for (int j = i + 1; j < config_.associativity; ++ j)
                block_queue[index][j - 1] = block_queue[index][j];
            block_queue[index][config_.associativity - 1] = position;
            break; 
        }
    }
}
/*
    // Bypass?
    if (!BypassDecision()) {
        PartitionAlgorithm();
        // Miss?
        if (ReplaceDecision(index, tag)) {
            // Choose victim
            ReplaceAlgorithm();
        } else {
            // return hit & time
            hit = 1;
            time += latency_.bus_latency + latency_.hit_latency;
            stats_.access_time += time;
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
    */

/*
int Cache::BypassDecision() {

    return FALSE;
}

void Cache::PartitionAlgorithm() {
}

int Cache::ReplaceDecision(uint64_t index, uint64_t tag) {
    for (int i = 0; i < set_num; ++ i)

}

void Cache::ReplaceAlgorithm(){
}

int Cache::PrefetchDecision() {
  return FALSE;
}

void Cache::PrefetchAlgorithm() {
}

*/