#ifndef CM_H
#define CM_H

#include "commons.h"
#include "cm_services.grpc.pb.h"
using CMKeyStore::CMRequest;
using CMKeyStore::CMResponse;
using CMKeyStore::CMService;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class cm_service_impl: public CMService::Service {
	Status CMRequestHandler (ServerContext* context, const  CMRequest* request,
			CMResponse* reply);
};

class cm_client
{
	private:
                
    public:
    	std::unique_ptr<CMService::Stub> stub_;
    	cm_client(std::shared_ptr<Channel> channel): stub_(CMService::NewStub(channel)){};
};

class cm_server_connections {
		/* Each client will have connection with server
			we done need to create connection everytime
			*/
	public:
		map<string, cm_client*> connections;
		cm_server_connections(vector<string> server_list, int mylocation);
		~cm_server_connections();
};


cm_message_request_t*
make_cm_request_t(CMRequest* request);

void
print_vec_clk(int *vecclk, uint32_t size);

void
make_cm_request_payload(CMRequest* payload, cm_message_request_t *cm_req);

void RunCMServer(string server_address);

void send_to_cm_server_handler(cm_client* connection_stub, CMRequest *req);

void send_message_to_all_cm_server(promise<string>& prom,  cm_message_request_t *c_req);

class vectclk_comparator {
	public:
		 bool operator() (cm_message_request_t const *m1, cm_message_request_t const *m2);
};

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




#endif //CM_H