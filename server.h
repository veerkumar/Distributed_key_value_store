#include "key_store_services.grpc.pb.h"
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using KeyStore::KeyStoreRequest;
using KeyStore::KeyStoreResponse;
using KeyStore::KeyStoreService;




void
delete_cm_request_t(cm_message_request_t* cm_req);





extern mutex abd_ks_map_mutex;
extern map<string,value_t*> abd_ks_map;

extern mutex cm_ks_map_mutex;
extern map<string,value_t*> cm_ks_map;

extern mutex cm_pq_mutex;
extern priority_queue <cm_message_request_t*, vector<cm_message_request_t*>, vectclk_comparator > cm_message_pq;

extern mutex cm_outque_mutex;
extern std::queue<cm_message_request_t* > cm_message_outqueue;

extern mutex cm_t_mutex;
extern int *t;
extern int serverlist_size;

extern int mynodenumber;

extern cm_server_connections *cm_connection_obj;