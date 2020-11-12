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
                std::unique_ptr<KeyStoreService::Stub> stub_;
    public:
                key_store_client(std::shared_ptr<Channel> channel): stub_(KeyStoreService::NewStub(channel)){};
                /* converting to string
                string(servers[0].ip) + ":" + to_string(servers[0].port)
                */
};


class server_connections {
		/* Each client will have connection with server
			we done need to create connection everytime
			*/
	public:
		map<string, key_store_client*> server_connections;
		server_connections(struct Server_info* servers, uint32_t number_of_servers);
		~server_connections();
};

/*i = Internal*/
typedef struct client_wrapper_i {
	Client *cl;
	server_connections conn;
} client_wrapper;

mutex client_list_mutex;

std::map<uint32_t, client_wrapper> client_list;
