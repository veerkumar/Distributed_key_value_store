#ifndef COMMONS_H
#define COMMONS_H

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <stdint.h>
#include <time.h>
#include <mutex>
#include <fstream>
#include <utility>
#include <cstddef>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bits/stdc++.h>
#include <cstdint>
#include <cstring>
#include <future>
#include <utility>



#include <grpc/grpc.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
//#include <grpcpp/security/credentials.h>

//#define DEBUG_FLAG 1
//#define DEBUG_LOG_QUEUE 1

//#define INTERFACE "lo"
#define INTERFACE "ens4"

#define SIZE_OF_VALUE  1024

#define ABD_NUMBER_OF_CLIENTS 3

#define CM_NUMBER_OF_CLIENTS 10

#define MP_NUMBER_OF_CLIENTS 3

#define INT_SIZE sizeof(int)

#define INIT_COMMAND_LENGTH 100000


using namespace std;
enum request_type {
	READ_QUERY = 0,
	WRITE_QUERY = 1,
	READ = 2,
	WRITE = 3
};

enum return_code {
            ACK = 0,
            ERROR = 1,
            OK = 2,
            NACK = 3,
            SOME_NACK = 4
};

enum protocol_type {
	CM = 0,
	ABD = 1,
  MP = 2  // Multi-Paxos
};

typedef struct tag {
	uint32_t integer;
	uint32_t client_id;
} tag_t;

typedef struct request_ {
  request_type type;
  protocol_type protocol;
  tag_t tag;
  char *key;
  uint32_t key_sz;
  char *value;
  uint32_t value_sz;
} request_t;

typedef struct  response_ {
  return_code code;
  protocol_type protocol;
  tag_t tag;
  char *key;
  uint32_t key_sz;
  char *value;
  uint32_t value_sz;
} response_t;

typedef struct value_ {
  tag_t tag;
  char* key;
  uint32_t key_sz;
  char* value;
  uint32_t value_sz;
} value_t;


typedef struct cm_message_request_ {
  uint32_t nodenum;
  char* key;
  uint32_t key_sz;
  char* value;
  uint32_t value_sz;
  int *vec_clk;
  uint32_t vecclk_sz;
} cm_message_request_t;

typedef struct cm_message_response_ {
  return_code code;
} cm_message_response_t;


extern uint32_t get_random_number ();

bool apply_last_write(string key, uint32_t index);



#endif //end of COMMONS_H
