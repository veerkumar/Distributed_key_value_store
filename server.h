#include "key_store_services.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using KeyStore::KeyStoreRequest;
using KeyStore::KeyStoreResponse;
using KeyStore::KeyStoreService;

typedef struct value_ {
	tag_t tag;
	char* key;
	uint32_t key_sz;
	char* value;
	uint32_t value_sz;
} value_t;

mutex abd_ks_map_mutex;
map<string,value_t*> abd_ks_map;

mutex cm_ks_map_mutex;
map<string,value_t*> cm_ks_map;
