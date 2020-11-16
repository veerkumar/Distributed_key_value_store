#include "commons.h"
#include "client.h"
#include "key_store_client.h"

server_connections::server_connections(struct Server_info* servers, uint32_t number_of_servers){
	string key;  
	/* Create connection with all servers */
	for (uint32_t i = 0; i < number_of_servers; i++) {
		key = string(servers[i].ip) + ":" + to_string(servers[i].port);
#ifdef DEBUG_FLAG
                 cout<<"\n\n"<<__func__ <<": Creating new connection with file server :"<< key;
#endif
		connections[key] = new key_store_client(grpc::CreateChannel(key, grpc::InsecureChannelCredentials()));
	}
}
server_connections::~server_connections(){
	/* Delete all the connection */
	map<string, key_store_client*>::iterator it = connections.begin();

	while(it!=connections.end()){
		delete it->second;
		it++;
	}
}
struct Client* client_instance(const uint32_t id, const char* protocol, const struct Server_info* servers, uint32_t number_of_servers)
{
	struct Client* cl  = new Client;
	cl->id = id;
	memcpy(&cl->protocol, protocol, sizeof(protocol));
	cl->number_of_servers = number_of_servers;
	cl->servers = new struct Server_info[number_of_servers];
	cout<<sizeof(struct Server_info)*number_of_servers;
	memcpy(cl->servers, servers, sizeof(struct Server_info)*number_of_servers);

	client_wrapper *cl_w = new struct client_wrapper_i;
	cl_w->cl = cl ;
	cl_w->conn = new server_connections(cl->servers, number_of_servers);

	/*Append the client information to client list */
	client_list_mutex.lock();
	client_list[id] = cl_w;
	client_list_mutex.unlock();
	return cl;
}
void 
make_req_payload (KeyStoreRequest *payload, 
		request_t *req) {
	
	/*Fill protocol*/
	if (req->protocol == CM) {
		payload->set_protocol(KeyStoreRequest::CM);
	}
	if (req->protocol == ABD) {
		payload->set_protocol(KeyStoreRequest::ABD);
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
		payload->set_integer(req->tag.integer);
		payload->set_clientid(req->tag.client_id);
		payload->set_key(req->key,req->key_sz);
		payload->set_keysz(req->key_sz);
		payload->set_value(req->value,req->value_sz);
		payload->set_valuesz(req->value_sz);
	}
	if (req->type ==  READ_QUERY) {
		payload->set_type(KeyStoreRequest::READ_QUERY);
		payload->set_key(req->key,req->key_sz);
		payload->set_keysz(req->key_sz);
		payload->set_valuesz(0);
	}
	// Not needed
	// if (req->type ==  READ) {
	// 	payload->set_type(KeyStoreRequest::READ);
		
	// }
}
request_type
get_c_request_type(KeyStoreRequest::RequestType type) {
	switch (type) {
		case KeyStoreRequest::READ:
			return READ;
		case KeyStoreRequest::READ_QUERY:
			return READ_QUERY;
		case KeyStoreRequest::WRITE:
			return WRITE;
		case KeyStoreRequest::WRITE_QUERY:
			return WRITE_QUERY;
		default:
			cout<<"get_ctype: wrong request type";	
	}
	return READ;
}

return_code
get_c_return_code(KeyStoreResponse::ReturnCode type) {
	switch (type) {
		case KeyStoreResponse::ACK:
			return ACK;
		case KeyStoreResponse::ERROR:
			return ERROR;
		case KeyStoreResponse::OK:
			return OK;
		default:
			cout<<"get_ctype: wrong return code";	
	}
	return OK;
}

void 
print_request(request_t *req) {
	cout<< "Printing request" <<endl;
	cout<<" Type     :"<<req->type<<endl;
	cout<<" Integer  :"<<req->tag.integer<<endl;
	cout<<" Client_id:"<<req->tag.client_id<<endl;
	cout<<" key      :"<<req->key<<endl;
	cout<<" key_sz   :"<<req->key_sz<<endl;
	cout<<" value    :"<<req->value<<endl;
	cout<<" value_sz:"<<req->value_sz<<endl;
}

response_t* 
extract_response_from_payload(KeyStoreResponse *Response, KeyStoreRequest req, bool overide_key ) {
	response_t *c_response = new response_t;
	c_response->code = get_c_return_code(Response->code());

	if (Response->protocol() == KeyStoreResponse::CM) {
		c_response->protocol = CM;
	}
	if (Response->protocol() == KeyStoreResponse::ABD) {
		c_response->protocol = ABD;
	}

	c_response->tag.integer = Response->integer();
	c_response->tag.client_id = Response->clientid();
	if (overide_key) {
#ifdef DEBUG_FLAG
		cout<<__func__<<"Response key size= "<<Response->key().size();
		cout<<__func__<<"Response key received = "<<Response->key();
#endif
		c_response->key = new char[Response->key().size()];
		memcpy(c_response->key, Response->key().c_str(), Response->key().size());
#ifdef DEBUG_FLAG
		cout<<__func__<<"Response after mem_cpy = "<<c_response->key;
#endif
	} else {
		#ifdef DEBUG_FLAG
		cout<<__func__<<" not overridding \n \tResponse key size= "<<req.key().size()<<endl;
		cout<<__func__<<"\tResponse key received = "<<req.key()<<endl;
#endif
		c_response->key = new char[req.key().size()];
		memcpy(c_response->key, req.key().c_str(), req.key().size());
#ifdef DEBUG_FLAG
		cout<<__func__<<"\tResponse after mem_cpy = "<<c_response->key<<endl;
#endif
	}
	c_response->key_sz = Response->keysz();
	if(Response->valuesz()) {
#ifdef DEBUG_FLAG
		cout<<__func__<<"Response value size= "<<Response->value().size()<<endl;
		cout<<__func__<<"Response value received = "<<Response->value()<<endl;
#endif
		c_response->value = new char[Response->value().size()];
		memcpy(c_response->value, Response->value().c_str(), Response->value().size());
#ifdef DEBUG_FLAG
		cout<<__func__<<"Response value after mem_cpy = "<<c_response->value<<endl;
#endif
	} else {
		cout<< "Value from server is 0"<<endl;
	}
	c_response->value_sz = Response->valuesz();
	return c_response;
}

void 
send_to_server_handler(key_store_client* connection_stub, 
				promise<response_t*>& keyStoreResponsePromise, KeyStoreRequest *req){
//send_to_server_handler(key_store_client* connection_stub, KeyStoreRequest *req){
		KeyStoreResponse Response;
		ClientContext Context;
		response_t *c_response = NULL; // This will be returned back in Promise

#ifdef DEBUG_FLAG
			std::cout << "Sending message to server "<<endl;
#endif

		Status status = connection_stub->stub_->KeyStoreRequestHandler(&Context, *req, &Response);
		if (status.ok()) {
				c_response = extract_response_from_payload(&Response, *req, 0);
#ifdef DEBUG_FLAG
			std::cout << "Got the response from server"<<endl;
#endif
				keyStoreResponsePromise.set_value(c_response);
			} else {
				std::cout << status.error_code() << ": " << status.error_message()
					<< std::endl;
				return ;
			}
			return;
}

/* Agnostic to type of message, return the response */
void send_message_to_all_server(promise<vector<response_t*>>& prom, client_wrapper *cw, request_t *c_req) {
	KeyStoreRequest ReqPayload;
	int number_of_servers =  cw->conn->connections.size();
	int majority = (number_of_servers/2+1), num_resp_collected = 0;
	std::chrono::system_clock::time_point span ;
	vector<response_t*> vec_resp ;
	vector<promise<response_t*>> vec_prom; // Stores Promises
	vector<future<response_t*>> vec_fut; // Stores future
	vector<future<void>> vec_temp_fut;	 // Stores async return future, we dont use it but asyn 
										 // become sync if we dont store it.


	make_req_payload(&ReqPayload, c_req);
	print_request(c_req);
	int i = 0;
	for (auto it = cw->conn->connections.begin(); it!=cw->conn->connections.end(); it++) {
		vec_prom.emplace_back(promise<response_t*>());
		vec_fut.emplace_back(vec_prom[i].get_future());
		vec_temp_fut.emplace_back(std::async(std::launch::async, send_to_server_handler, 
								 it->second, ref(vec_prom[i]), &ReqPayload));
//		vec_temp_fut.emplace_back(std::async(std::launch::async, send_to_server_handler, 
//								 it->second, &ReqPayload));
		i++;
	}
	while(1) {
			for (auto &future:vec_fut){
            span = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
           
            if (future.valid() && future.wait_until(span)== std::future_status::ready) {
                cout<<"\nfound true" <<endl;
                vec_resp.push_back(future.get());
                num_resp_collected++;
            } 
        }
        if(num_resp_collected >= majority){
            cout<<"Got the majority:"<< num_resp_collected<< ", returning now"<<endl;
            prom.set_value(vec_resp);
            break;
        }
	}
	// caller will clean the "vec_resp"
	// vec_prom, vec_fut and vec_temp_fut are local variable, will be freed
}
void delete_response_t(response_t *resp) {
	delete resp->key;
	delete resp->value;
	delete resp;
}


response_t* find_majority_tag (vector<response_t*> &vec_c_resp) {
	response_t*  max_resp = vec_c_resp[0];
	for(auto it = vec_c_resp.begin(); it!= vec_c_resp.end();) {
			if ((*it)->tag.integer >= max_resp->tag.integer) {
				if ((*it)->tag.integer == max_resp->tag.integer) {
					if((*it)->tag.client_id > max_resp->tag.client_id){
						delete_response_t (max_resp);
						max_resp = *it;
						vec_c_resp.erase(it);
						continue;
					} else {
						vec_c_resp.erase(it);
						continue;
					}
				}
			}
			delete_response_t(*it);
			vec_c_resp.erase(it);
	}
	return max_resp;
}

int put(const struct Client* c, const char* key, uint32_t key_size, 
	const char* value, uint32_t value_size){
	vector<response_t*> vec_c_resp ;
	response_t *max_resp = NULL;


	/* Based on the client ID, fetch the server connection and call the function */
	if (string(c->protocol) == "CM") {
	#ifdef DEBUG_FLAG
		cout<< "CM protocol: Put request"<<endl;
	#endif

	} else {
		/* ABD algorithm case */
	#ifdef DEBUG_FLAG
		cout<< "ABD protocol: Put request"<<endl;
	#endif
		/* Send the write_query */
		request_t *c_req = new request_t;
		c_req->type = WRITE_QUERY;
		c_req->tag.integer = 0; // Since query will fetch the integer
		c_req->tag.client_id = c->id;
		memcpy(c_req->key, key, key_size);
		c_req->key_sz = key_size;

		promise<vector<response_t*>> pm =  promise<vector<response_t*>>();
    	future <vector<response_t*>> fu = pm.get_future();
		thread t1(send_message_to_all_server, ref(pm), client_list[c->id], c_req);
		t1.detach();
		vec_c_resp = fu.get();

		max_resp = find_majority_tag(vec_c_resp); 
		
		

		/*Send the write request*/
		c_req->type = WRITE;
		c_req->tag.integer = max_resp->tag.integer + 1;	
		memcpy(c_req->value, value, value_size);
		c_req->value_sz = value_size;

		delete_response_t(max_resp); 

		promise<vector<response_t*>> pm1 =  promise<vector<response_t*>>();
    	future <vector<response_t*>> fu1 = pm1.get_future();
		thread t2(send_message_to_all_server, ref(pm1), client_list[c->id], c_req);
		t2.detach();
		vec_c_resp = fu1.get();
		max_resp = find_majority_tag(vec_c_resp); 
		delete_response_t(max_resp); 
		return 0;
	}

	return 1;
}

int get(const struct Client* c, const char* key, uint32_t key_size, 
	char** value, uint32_t *value_size){

	vector<response_t*> vec_c_resp;
	response_t *max_resp = NULL;


	/* Based on the client ID, fetch the server connection and call the function */
	if (string(c->protocol) == "CM") {
		#ifdef DEBUG_FLAG
		cout<< "CM protocol: Put request"<<endl;
	#endif

	} else {
		/* ABD algorithm case */
	#ifdef DEBUG_FLAG
		cout<< "ABD protocol: Get request"<<endl;
	#endif
		/* Send the write_query */
		request_t *c_req = new request_t;
		c_req->type = READ_QUERY;
		c_req->tag.integer = 0; // Since query will fetch the integer
		c_req->tag.client_id = c->id;
		memcpy(c_req->key, key, key_size);
		c_req->key_sz = key_size;

		promise<vector<response_t*>> pm =  promise<vector<response_t*>>();
    	future <vector<response_t*>> fu = pm.get_future();
		thread t1(send_message_to_all_server, ref(pm), client_list[c->id], c_req);
		t1.detach();
		vec_c_resp = fu.get();

		max_resp = find_majority_tag(vec_c_resp); 
		
		/*Send the write request*/
		if (max_resp->value_sz) {
			c_req->type = WRITE;
			c_req->tag.integer = max_resp->tag.integer;
			c_req->tag.client_id = max_resp->tag.client_id;	
			memcpy(c_req->value, max_resp->value, max_resp->value_sz);
			c_req->value_sz = max_resp->value_sz;

			delete_response_t(max_resp); 

			promise<vector<response_t*>> pm1 =  promise<vector<response_t*>>();
			future <vector<response_t*>> fu1 = pm1.get_future();
			thread t2(send_message_to_all_server, ref(pm1), client_list[c->id], c_req);
			t2.detach();
			vec_c_resp = fu1.get();
			max_resp = find_majority_tag(vec_c_resp); 
			delete_response_t(max_resp); 
		} else {
			cout<< "Read was issued on non-existent key" <<endl;
			return -1;
		}
		return 0;
	}
	return -1;
}

int client_delete(struct Client* c)
{
	/*Locking the client list before deleting it, may not be required but locking anyway for thread_safe*/
	client_list_mutex.lock();
	auto it  = client_list.find(c->id);
	if(it!=client_list.end()) {
		int id = c->id;
	#ifdef DEBUG_FLAG
		cout<<"Number of clients before deletion" << client_list.size()<<endl;
		cout<<"Deleting server connecitons"<<endl;
	#endif
		for (auto it =  (client_list[id])->conn->connections.begin(); it != (client_list[id])->conn->connections.end();it++ ) {
			cout<<"Deleteing server connection" <<endl;
			delete it->second;
		}

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
		cout<<"Number of clients after deletion" << client_list.size()<<endl;
	#endif
		client_list_mutex.unlock();
	}
	return 0;
}