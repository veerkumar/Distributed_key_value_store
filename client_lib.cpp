#include "commons.h"
#include "client.h"
#include "key_store_client.h"
/*
struct Server_info{
    char ip[16]; // IP Sample: "192.168.1.2"
    uint16_t port;
};


struct Client{
    uint32_t id;
    char protocol[4]; // ABD or CM
    struct Server_info* servers; // An array containing the infromation to access each server
    uint32_t number_of_servers; // Number of elements in the array servers
};
*/
server_connections::server_connections(struct Server_info* servers, uint32_t number_of_servers){
	string key;  
	/* Create connection with all servers */
	for (int i = 0; i < number_of_servers; i++) {
		key = string(servers[i].ip) + ":" + to_string(servers[i].port);
#ifdef DEBUG_FLAG
                 cout<<"\n\n"<<__func__ <<": Creating new connection with file server :"<< key;
#endif
		server_connections[key] = new key_store_client(grpc::CreateChannel(ip_port, grpc::InsecureChannelCredentials()));
	}
}
server_connections::~server_connections(){
	/* Delete all the connection */
	map<string, key_store_client*>::iterator it = server_connections.begin();

	while(it!=server_connections.end()){
		delete it->second;
		it++;
	}
}
struct Client* client_instance(const uint32_t id, const char* protocol, const struct Server_info* servers, uint32_t number_of_servers)
{
	struct Client* cl  = new Client;
	cl->id = id;
	memcpy(cl->protocol, protocol, sizeof(protocol));
	cl->number_of_servers = number_of_servers;
	cl->servers = new struct Server_info[number_of_servers];
	cout<<sizeof(struct Server_info)*number_of_servers;
	memcpy(cl->servers, servers, sizeof(struct Server_info)*number_of_servers);

	client_wrapper cl_w = new client_wrapper;
	cl_w->cl = cl ;
	cl_w->conn = new server_connections(cl->servers, number_of_servers);

	/*Append the client information to client list */
	client_list_mutex.lock();
	client_list[id] = cl_w;
	client_list_mutex.unlock();
	return cl;
}
RequestType type = 1;
  ProtocolType protocol = 2;
  uint32 integer = 3;
  uint32 cliendId = 4;
  bytes key = 5;
  uint32 key_sz = 6;
  bytes  value = 7;         //This will be filled while writing value to the server
  uint32 value_sz = 8;

  message_type type;
		  protocol_type protocol;
		  tag_t tag;
		  char *key;
		  uint32_t key_sz;
		  char *value;
		  uint32_t *value_sz;
		} request_t;
void 
make_req_payload (KeyStoreRequest *payload, 
		request_t *req) {
	
	/*Fill protocol*/
	if (req->protocol == CM) {
		payload->set_protocol(KeyStoreRequest::CM)
	}
	if (req->protocol == ABD) {
		payload->set_protocol(KeyStoreRequest::ABD)
	}

	if (req->type == WRITE_QUERY) {
		payload->set_type(KeyStoreRequest::WRITE_QUERY);
		/* NO tag*/
		/* No clientId */
		payload->set_key(req->key,req->key_sz);
		payload->set_keysz(req->key_sz);
		/* NO value */
		payload->set_valuesz(0);
	}

	if (req->type ==  WRITE) {
		payload->set_type(KeyStoreRequest::WRITE);

	}
	if (req->type ==  READ_QUERY) {
		payload->set_type(KeyStoreRequest::READ_QUERY);
		payload->set_valuesz(0);
	}
	if (req->type ==  READ) {
		payload->set_type(KeyStoreRequest::READ);

		
	}
}

/* Agnostic to type of message, return the response */
vector<response_t*> send_message_to_all_server(client_wrapper cw, request_t *req) {
	KeyStoreRequest



}

int put(const struct Client* c, const char* key, uint32_t key_size, 
	const char* value, uint32_t value_size){
	vector<response_t*> vec_c_resp = NULL;

	/* Based on the client ID, fetch the server connection and call the function */
	if (string(c->protocol) == "CM") {
		#ifdef DEBUG_FLAG
		cout<< "CM protocol: Put request"<<endl;
	#endif

	} else {
		/*
		enum protocol_type { CM = 0, ABD = 1};
		typedef struct tag {
				uint32_t integer; uint32_t client_id;} tag_t;

		typedef struct request_ {
		  message_type type;
		  protocol_type protocol;
		  tag_t tag;
		  char *key;
		  uint32_t key_sz;
		  char *value;
		  uint32_t *value_sz;
		} request_t;
		*/
		/* ABD algorithm case */
	#ifdef DEBUG_FLAG
		cout<< "ABD protocol: Put request"<<endl;
	#endif
		/* Send the write_query */
		request_t *c_req = new request_t;
		c_req->type = WRITE_QUERY;
		c_req->tag->integer = 0; // Since query will fetch the integer
		c_req->tag->client_id = c->id;
		memcpy(c_req->key, key, key_size);
		c_req->key_sz = key_size;

		vec_c_resp = send_message_to_all_server(client_list[c->id], c_req);
		int max_integer = 0 ;
		for(auto it = vec_c_resp.begin(); it!= vec_c_resp.end();) {
			if ((*it)->tag.integer > max_integer) {
				max_integer = (*it)->tag.integer;
			}
			vec_c_resp.erase(it);
		}

		/*Send the write request*/
		c_req->type = WRITE;
		c_req->tag->integer = max_integer + 1;	
		memcpy(c_req->value, value, value_size);
		c_req->value_sz = value_size;
		vec_c_resp = send_message_to_all_server(client_list[c->id], c_req);


	}

	return 0;
}

int get(const struct Client* c, const char* key, uint32_t key_size, 
	char** value, uint32_t *value_size){
	return 0;
}

int client_delete(struct Client* c)
{
	/*Locking the client list before deleting it, may not be required but locking anyway for thread_safe*/
	client_list_mutex.lock();
	auto it  = client_list.find(c->id);
	if(it!=client_list.end()) {
		int id = c->id;
	#ifdef DEBUG_FLAG
		cout<<"Number of clients before deletion" << client_list.size();
		cout<<"Deleting server connecitons"<<endl;
	#endif
		delete  (client_list[id])->conn;

	#ifdef DEBUG_FLAG
		cout<<"Deleting server list"<<endl;
	#endif
		delete [] (client_list[id])->cl->servers;


	#ifdef DEBUG_FLAG
		cout<<"Deleting Client structure"<<endl;
	#endif
		delete (client_list[id])->cl;

		client_list.erase(id);
	#ifdef DEBUG_FLAG
		cout<<"Number of clients after deletion" << client_list.size();
	#endif
		client_list_mutex.unlock();
	}
	return 0;
}