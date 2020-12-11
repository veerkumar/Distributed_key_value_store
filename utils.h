#include "commons.h"
#include "key_store_services.grpc.pb.h"
using KeyStore::KeyStoreRequest;
using KeyStore::KeyStoreResponse;
using KeyStore::KeyStoreService;

string
get_ipaddr();

request_type
get_c_request_type(KeyStoreRequest::RequestType type);

string
getstring_grpc_request_type(KeyStoreRequest::RequestType type);

string
getstring_c_request_type(request_type type);


return_code
get_c_return_code(KeyStoreResponse::ReturnCode type);

void
print_grpc_return_code(KeyStoreResponse::ReturnCode type);


return_code
get_c_return_code(PaxosResponse::ReturnCode type);

request_type
get_c_command_type(PaxosResponse::CommandType type);

mp_request_type
get_c_mp_req_type(PaxosRequest::RequestType type)

PaxosResponse::ReturnCode
get_grpc_return_code(return_code type);

PaxosRequest::CommandType
get_grpc_req_command_type(request_type type);

PaxosRequest::RequestType
get_grpc_req_type(mp_request_type type);

PaxosResponse::CommandType
get_grpc_resp_command_type(request_type type);

void print_mp_req_type(mp_req_type type);
void print_command_type (request_type type);
void
print_return_code(return_code type);
void 
print_command_t(command_t *req, bool request);