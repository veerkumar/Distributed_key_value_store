#include "commons.h"
#include "mp.h"
#include "utils.h"

mp_server_connections::mp_server_connections(vector<string> server_list, int mylocation){
	string key;  
	std::string delimiter = ":";

	/* Create connection with all servers */
	for (int i = 0; i < int(server_list.size()); i++) {
		if(i!= mylocation) {
			std::string port = server_list[i].substr(server_list[i].find(delimiter)+1, server_list[i].length());
			key = get_ipaddr() + port;
			#ifdef DEBUG_FLAG
				cout<<"\n\n"<<__func__ <<": Creating new connection with file server :"<< key <<endl;
			#endif
			connections[server_list[i]] = new mp_client(grpc::CreateChannel(key, 
											grpc::InsecureChannelCredentials()));
		} else {
			#ifdef DEBUG_FLAG
				cout<< "Not creating conneciton with self"<<endl;
			#endif
		}
	}
}

mp_server_connections::~mp_server_connections(){
	/* Delete all the connection */
	map<string, mp_client*>::iterator it = connections.begin();

	while(it!=connections.end()){
		delete it->second;
		it++;
	}
}

void
make_mp_request_payload (PaxosRequest *payload, command_t* c_req) {
	switch (c_req->mp_req_type) {
		case PREPARE:
			payload->set_mptype( PaxosRequest::PREPARE); break;
		case ACCEPT:
			payload->set_mptype( PaxosRequest::ACCEPT); break;
		default:
			cout<<"get_ctype: wrong command type ";	
	}
	

	switch (c_req->command_type) {
		case READ:
			 payload->set_commandtype(PaxosRequest::READ); break;
		case WRITE:
			payload->set_commandtype( PaxosRequest::WRITE); break;
		default:
			cout<<"get_ctype: wrong command type ";	
	}
	
	
	if (c_req->key_sz) {
		payload->set_key(c_req->key,c_req->key_sz);
		payload->set_keysz(c_req->key_sz);
	}
	if(c_req->value_sz) {
		payload->set_value(c_req->value,c_req->value_sz);
		payload->set_valuesz(c_req->value_sz);
	}
	payload->set_proposalclientid(c_req->accepted_proposal.proposal_client_id);
	payload->set_proposalnum(c_req->accepted_proposal.proposal_num);
	payload->set_index(c_req->index);
	payload->set_commandid(c_req->command_id);
}

command_t* 
extract_response_from_payload(PaxosResponse *Response) {
	command_t *c_response = new command_t;
	c_response->code = get_c_return_code(Response->code());
	if (Response->keysz()) {
		// #ifdef DEBUG_FLAG
		// 	cout<<"	"<<__func__<<"Response key size= "<<Response->key().size();
		// 	cout<<"	"<<__func__<<"Response key received = "<<Response->key();
		// #endif
		c_response->key = new char[Response->keysz()+1];
		memset(c_response->key, 0, Response->keysz()+1);
		memcpy(c_response->key, Response->key().c_str(), Response->keysz());

		// #ifdef DEBUG_FLAG
		// 	cout<<"	"<<__func__<<"Response after mem_cpy = "<<c_response->key;
		// #endif
	}
	c_response->key_sz = Response->keysz();
	
	if (Response->valuesz()) {
		// #ifdef DEBUG_FLAG
		// 	cout<<"	"<<__func__<<"Response value size= "<<Response->value().size()<<endl;
		// 	cout<<"	"<<__func__<<"Response value received = "<<Response->value()<<endl;
		// #endif
		c_response->value = new char[Response->valuesz()+1];
		memset(c_response->value, 0, Response->valuesz()+1);
		memcpy(c_response->value, Response->value().c_str(), Response->valuesz());
		// #ifdef DEBUG_FLAG
		// 		cout<<__func__<<"Response value after mem_cpy = "<<c_response->value<<endl;
		// #endif
	}
	c_response->value_sz = Response->valuesz();

	c_response->min_proposal_num.proposal_client_id = Response->proposalclientid();
	c_response->min_proposal_num.proposal_num = Response->proposalnum();
	c_response->accepted_proposal.proposal_client_id = Response->proposalclientid();
	c_response->accepted_proposal.proposal_num = Response->proposalnum();
	c_response->command_id = Response->commandid();
	c_response->index = Response->index();

	return c_response;
}

void 
send_to_mp_server_handler(mp_client* connection_stub, 
				promise<command_t*>&& paxosResponsePromise, PaxosRequest *req){
//send_to_server_handler(key_store_client* connection_stub, KeyStoreRequest *req){
		PaxosResponse Response;
		ClientContext Context;
		command_t *c_response = NULL; // This will be returned back in Promise

		#ifdef DEBUG_FLAG
					std::cout << "Sending message to server "<<endl;
		#endif

		Status status = connection_stub->stub_->PaxosRequestHandler(&Context, *req, &Response);
		if (status.ok()) {
				c_response = extract_response_from_payload(&Response);
			#ifdef DEBUG_FLAG
						std::cout << "Got the response from server, returning now"<<endl;
			#endif
				paxosResponsePromise.set_value(c_response);
				
			} else {
				std::cout << status.error_code() << ": " << status.error_message()
					<< std::endl;
				return ;
			}
			return;
}

void send_message_to_all_mp_server(promise<vector<command_t*>>& prom,  command_t *c_req) {
	#ifdef DEBUG_FLAG
		print_command_t(c_req,1);
	#endif
	PaxosRequest ReqPayload;
	uint32_t number_of_servers =  mp_connection_obj->connections.size();
	int majority = ((number_of_servers+1)/2), num_resp_collected = 0;
	std::chrono::system_clock::time_point span ;
	vector<command_t*> vec_resp ;
	vector<promise<command_t*>> vec_prom; // Stores Promises
	vector<future<command_t*>> vec_fut; // Stores future
	vector<future<void>> vec_temp_fut;
	std::vector<std::thread*> threads;

	make_mp_request_payload(&ReqPayload, c_req);
	#ifdef DEBUG_FLAG
		print_command_t(c_req, 1);
	#endif
	int i = 0;
	for (auto it = mp_connection_obj->connections.begin(); it!= mp_connection_obj->connections.end(); it++) {
		#ifdef DEBUG_FLAG
			cout<<"MP_client: Sending to server handler" <<endl;
		#endif
		promise<command_t*> pm = promise<command_t*>();
		future<command_t*> fu = pm.get_future();
		vec_prom.emplace_back(std::move(pm));
		vec_fut.emplace_back(std::move(fu));
		vec_temp_fut.emplace_back(std::async(std::launch::async, send_to_mp_server_handler, 
								 it->second, std::move(vec_prom[i]), &ReqPayload));
		i++;
	}
	
	#ifdef DEBUG_FLAG
		cout << "Majority to get: " <<majority<<endl;
	#endif
	while(1) {
			for (auto &future:vec_fut){
            	span = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
           		//cout<<count++ <<endl;
	            if (future.valid() && future.wait_until(span)== std::future_status::ready) {
	                vec_resp.push_back(future.get());
	                num_resp_collected++;
	            } 
        	}
        if(num_resp_collected >= majority){
        	#ifdef DEBUG_FLAG
            	cout<<"Got the majority:"<< num_resp_collected<< ", returning now"<<endl;
            #endif
            	
            prom.set_value(vec_resp);
            break;
        }
	}   	
}