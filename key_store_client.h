#include "commons.h"
#include "key_store_services.grpc.pb.h"
#include "client.h"


using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;

using KeyStore::KeyStoreRequest;
using KeyStore::KeyStoreResponse;
using KeyStore::KeyStoreService;

/* Indvidual client */
class key_store_client
{
	private:
                
    public:
    	std::unique_ptr<KeyStoreService::Stub> stub_;
    	key_store_client(std::shared_ptr<Channel> channel): stub_(KeyStoreService::NewStub(channel)){};
};

/* This funciton sends message to each server */
//void send_to_server_handler(key_store_client* connection_stub, 
//				promise<request_t*>& ketStoreResponsePromise, KeyStoreRequest *req);
/*https://stackoverflow.com/questions/10890653/why-would-i-ever-use-push-back-instead-of-emplace-back/10890716
emplace_back is used when we have to call explicit constructor, since this object dont have one, hence not using it */

class server_connections {
		/* Each client will have connection with server
			we done need to create connection everytime
			*/
	public:
		map<string, key_store_client*> connections;
		server_connections(struct Server_info* servers, uint32_t number_of_servers);
		~server_connections();
};

/*i = Internal*/
typedef struct client_wrapper_i {
	Client *cl;
	server_connections *conn;
} client_wrapper;

mutex client_list_mutex;

std::map<uint32_t, client_wrapper*> client_list;
