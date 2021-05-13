#pragma once
extern "C" {
#include <stdio>
#include <stdint>

int BigoMLSHMCopy(const char* input_data, const uint input_size, char** shm_ptr, const int key_id, int* ret_id);

int BigoMLSHMDetach(const uint8_t* shm_addr);

int BigoMLSHMDel(int shm_id);
  
}