#include "commons.h"
#include "key_store_services.grpc.pb.h"
using KeyStore::KeyStoreRequest;
using KeyStore::KeyStoreResponse;
using KeyStore::KeyStoreService;

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