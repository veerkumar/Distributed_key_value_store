#ifndef MP_H
#define MP_H

#include "commons.h"
#include "paxos_services.grpc.pb.h"
using Paxos::PaxosRequest;
using Paxos::PaxosResponse;
using Paxos::PaxosService;

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

typedef struct proposal {
	int32_t proposal_client_id;
	int32_t proposal_num;
} proposal_t;

enum current_status
{
	EMPTY = 0,
	IN_PROGRESS = 1,
	DONE = 2
	
};
enum mp_request_type {
  PREPARE = 0,
  ACCEPT = 1,
};


typedef struct command {
  mp_request_type mp_req_type;
  request_type command_type; // READ or WRITE
  return_code code ;
  uint32_t command_id;
  uint32_t index;
  char *key;
  uint32_t key_sz;
  char *value;
  uint32_t value_sz;
  proposal_t *min_proposal_num;
  proposal_t *accepted_proposal;
} command_t;


typedef struct commands 
{
	set<int> next_available_slot;
	std::vector<command_t*> cmd_vec;
	int last_touched_index;
} commands_list_t;

class mp_service_impl: public PaxosService::Service {
	Status PaxosRequestHandler (ServerContext* context, const  PaxosRequest* request,
			PaxosResponse* reply);
};

class mp_client
{
	private:
                
    public:
    	std::unique_ptr<PaxosService::Stub> stub_;
    	mp_client(std::shared_ptr<Channel> channel): stub_(PaxosService::NewStub(channel)){};
};

class mp_server_connections {
		/* Each client will have connection with server
			we done need to create connection everytime
			*/
	public:
		map<string, mp_client*> connections;
		mp_server_connections(vector<string> server_list, int mylocation);
		~mp_server_connections();
};


void print_command_t(command_t *req, bool request);

return_code get_c_return_code(PaxosResponse::ReturnCode type);

void
make_mp_request_payload(PaxosRequest* payload, command_t *mp_req);

void RunMPServer(string server_address);

void send_to_mp_server_handler(mp_client* connection_stub, 
				promise<command_t*>&& paxosResponsePromise, PaxosRequest *req);

void send_message_to_all_mp_server(promise<vector<command_t*>>& prom,  command_t *c_req);

return_code get_c_return_code(PaxosResponse::ReturnCode type);

request_type get_c_command_type(PaxosResponse::CommandType type);
request_type get_c_command_type(PaxosRequest::CommandType type);

mp_request_type get_c_mp_req_type(PaxosRequest::RequestType type);


void print_mp_req_type(mp_request_type type);
void print_command_type (request_type type);
void
print_return_code(return_code type);

void 
print_command_t(command_t *req, bool request);


void print_current_log_db_state();

extern mutex abd_ks_map_mutex;
extern map<string,value_t*> abd_ks_map;

extern mutex cm_ks_map_mutex;
extern map<string,value_t*> cm_ks_map;


extern mutex cm_t_mutex;
extern int *t;
extern int serverlist_size;

extern int mynodenumber;



extern mutex mp_ks_map_mutex;
extern map<string,value_t*> mp_ks_map;

extern mp_server_connections *mp_connection_obj;

extern mutex log_map_mutex;
extern map<string, commands_list_t*> log_map;
extern mutex mp_mutex;
extern mutex printing_mutex;




#endif //MP_H