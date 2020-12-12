#include "mp.h"
#include "utils.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

extern map<string,value_t*> mp_ks_map;
extern mutex log_map_mutex;
extern map<string, commands_list_t*> log_map;
extern mutex mp_mutex;

void 
print_current_log_db_state() {
	//log_map_mutex.lock();
	//printing_mutex.lock();
	cout<<"====================================================" <<endl;
	cout<<"Printing Key Store state:"<<endl;
	cout<<"----------------------------------------------" <<endl;
	for(auto ks_it = mp_ks_map.begin(); ks_it!=mp_ks_map.end();ks_it++) {
		cout<<" key      :"<<(ks_it->second)->key<<endl;
		cout<<" key_sz   :"<<(ks_it->second)->key_sz<<endl;
		if((ks_it->second)->value_sz)
			cout<<" value    :"<<(ks_it->second)->value<<endl;
		cout<<" value_sz:"<<(ks_it->second)->value_sz<<endl;
		cout<<"\n";
	}
	cout<<"----------------------------------------------"<<log_map.size() <<endl;
	
	for(auto log_it = log_map.begin(); log_it!=log_map.end();log_it++) {
		int i = 0 ;
		for (auto it = (log_it)->second->cmd_vec.begin(); it != (log_it)->second->cmd_vec.end(); it++) {
			
			if((log_it)->second->last_touched_index < i){
				break;
			}
			cout<<"\n log_printing: ";print_command_t (*it, 1);
			i++;
			// if( i >=10) {
			// 	cout<<"Looks like infinite loop, touched index was :"<<(log_it)->second->last_touched_index <<endl;
			// 	break;
			// }
		}
	}

	//log_map_mutex.unlock();
	cout<<"====================================================" <<endl;
	//printing_mutex.unlock();
}

command_t*
extract_request_payload_to_command_t(const PaxosRequest* request) {
	command_t *c_req = new command_t;
	c_req->mp_req_type = get_c_mp_req_type(request->mptype());
	c_req->command_type = get_c_command_type(request->commandtype());
	if ( request->keysz()) {
		// #ifdef DEBUG_FLAG
		// 	cout<<"	"<<__func__<<"request key size= "<<request->key().size();
		// 	cout<<"	"<<__func__<<"request key received = "<<request->key();
		// #endif
		c_req->key = new char[request->keysz()+1];
		memset(c_req->key, 0, request->keysz()+1);
		memcpy(c_req->key, request->key().c_str(), request->keysz());
		
		// #ifdef DEBUG_FLAG
		// 	cout<<"	"<<__func__<<"request after mem_cpy = "<<c_req->key;
		// #endif
	}
	c_req->key_sz = request->keysz();

	if (request->valuesz()) {
		// #ifdef DEBUG_FLAG
		// 	cout<<"	"<<__func__<<"request value size= "<<request->value().size()<<endl;
		// 	cout<<"	"<<__func__<<"request value received = "<<request->value()<<endl;
		// #endif
		c_req->value = new char[request->valuesz()+1];
		memset(c_req->value, 0, request->valuesz()+1);
		memcpy(c_req->value, request->value().c_str(), request->valuesz());
		
		// #ifdef DEBUG_FLAG
		// 		cout<<__func__<<"request value after mem_cpy = "<<c_req->value<<endl;
		// #endif
	}
	c_req->value_sz = request->valuesz();
	c_req->accepted_proposal =  new proposal_t;
	c_req->min_proposal_num =  new proposal_t;
	c_req->min_proposal_num->proposal_client_id = -1;
	c_req->min_proposal_num->proposal_num = -1;
	c_req->accepted_proposal->proposal_client_id = request->proposalclientid();
	c_req->accepted_proposal->proposal_num = request->proposalnum();
	c_req->command_id = request->commandid();
	c_req->index = request->index();

	return c_req;
}


Status mp_service_impl::PaxosRequestHandler (ServerContext* context, const  PaxosRequest* request,
		PaxosResponse* reply)  {
	command_t *c_req;

#ifdef DEBUG_FLAG
	std::cout << "\n \n MP_service_impl: Got the New message"<<endl;
	// cout <<"			Node Number:"<<request->nodenum()<<endl;
	
	// cout <<"			Key       :"<<request->key()<< " request->key().size()"<< request->key().size()<<endl;
	// cout <<"			keysz 	  :"<<request->keysz()<<endl;
	// if(request->valuesz())
	// 	cout <<"			Value     :"<<request->value()<<endl;
	// cout <<"			valuesz    :"<<request->valuesz()<<endl;
#endif

	c_req = extract_request_payload_to_command_t(request);
	#ifdef DEBUG_FLAG
		cout<<"\n"<<__func__<< ": ";
		print_command_t(c_req, 1);
	#endif
	commands_list_t *cl =  NULL;
	command_t *cmd = NULL;
	log_map_mutex.lock();
	if (log_map.find(string(c_req->key)) != log_map.end()){
		#ifdef DEBUG_FLAG
			cout<<"Found find the key:"<<string(c_req->key)<<endl;
		#endif
		cl =  (log_map.find(string(c_req->key)))->second;		
		//cl->cmd_vec[curr_idx] = cmd;
	} else {
		// TODO lock
		#ifdef DEBUG_FLAG
			cout<<"Didnt find the key:"<<string(c_req->key)<<endl;
		#endif
		cl = new commands_list_t;
		cl->cmd_vec.reserve(INIT_COMMAND_LENGTH); 
		for (int i = 0; i < INIT_COMMAND_LENGTH; i++ ) {
			command_t *cmd = new command_t;
			cmd->min_proposal_num = new proposal_t; 
			cmd->accepted_proposal = new proposal_t; 
			cmd->min_proposal_num->proposal_client_id = -1;
			cmd->min_proposal_num->proposal_num = -1;
			cmd->accepted_proposal->proposal_client_id = -1;
			cmd->accepted_proposal->proposal_num = -1;
			cmd->key_sz = 0;
			cmd->key = NULL;
			cmd->value_sz = 0;
			cmd->value = NULL;
			cmd->command_id = 0;
			cmd->index = -1; 
			cl->cmd_vec.push_back(cmd) ;
			cl->next_available_slot.insert(i);
		}
		cl->last_touched_index = -1;
		
		//cl->cmd_vec[curr_idx] = cmd;	
	}
	log_map[string(c_req->key)] = cl;
	

	cmd = cl->cmd_vec[c_req->index];
	if(c_req->mp_req_type ==  PREPARE) {
		bool update = false;
		#ifdef DEBUG_FLAG
			cout<< "Prepare request from client_id: "<< c_req->accepted_proposal->proposal_client_id<<endl;
		#endif
			if (c_req->accepted_proposal->proposal_num > cmd->min_proposal_num->proposal_num ) {
				update = true;
			}
			if (c_req->accepted_proposal->proposal_num == cmd->min_proposal_num->proposal_num) {
				if (c_req->accepted_proposal->proposal_client_id > cmd->min_proposal_num->proposal_client_id ) {
					update = true;
				}
			}
			
			if (update) {
				cmd->min_proposal_num->proposal_client_id = c_req->accepted_proposal->proposal_client_id;
				cmd->min_proposal_num->proposal_num = c_req->accepted_proposal->proposal_num;
				reply->set_code(PaxosResponse::ACK);

				reply->set_proposalclientid(cmd->accepted_proposal->proposal_client_id);
				reply->set_proposalnum(cmd->accepted_proposal->proposal_num);
				
				reply->set_commandid(cmd->command_id);

				switch (c_req->command_type) {
					case READ:
						 reply->set_commandtype(PaxosResponse::READ); break;
					case WRITE:
						 reply->set_commandtype(PaxosResponse::WRITE); break;
					default:
						cout<<"Unknown command type "<<endl;	
				}
				reply->set_index(cmd->index);
				reply->set_keysz(cmd->key_sz);
				reply->set_key(cmd->key, cmd->key_sz);
				reply->set_valuesz(cmd->value_sz);
				reply->set_value(cmd->value, cmd->value_sz);
			} else {
				#ifdef DEBUG_FLAG
				cout<<"sending back NACK";
				#endif
				reply->set_code(PaxosResponse::NACK);
				reply->set_proposalclientid(cmd->min_proposal_num->proposal_client_id);
				reply->set_proposalnum(cmd->min_proposal_num->proposal_num);
			}
			#ifdef DEBUG_FLAG
			cout<<"Prepare request completed"<<endl;
			#endif
			cl->last_touched_index = c_req->index;
			print_current_log_db_state();

	} else {
		bool update = false;
		#ifdef DEBUG_FLAG
			cout<< "Accept request from client_id: "<< c_req->accepted_proposal->proposal_client_id<<endl;
			cout<< "local value: min_client_id : "<< cmd->min_proposal_num->proposal_client_id<<" proposal num: "<< cmd->min_proposal_num->proposal_num<<endl;
			cout<< "Recevied value: min_client_id : "<< c_req->accepted_proposal->proposal_client_id<<" proposal num: "<< c_req->accepted_proposal->proposal_num<<endl;
		#endif
			if (c_req->accepted_proposal->proposal_num >= cmd->min_proposal_num->proposal_num ) {
				update = true;
			}
			if (c_req->accepted_proposal->proposal_num == cmd->min_proposal_num->proposal_num) {
				if (c_req->accepted_proposal->proposal_client_id >= cmd->min_proposal_num->proposal_client_id ) {
					update = true;
				}
			}

			if(update) {
				cmd->command_type = c_req->command_type;
				cmd->command_id = c_req->command_id;
				cmd->index = c_req->index;
				if(cmd->key_sz) delete(cmd->key);
				if(cmd->value_sz) delete(cmd->value);

				cmd->key = new char[c_req->key_sz+1];
				memset(cmd->key,0,c_req->key_sz);
				memcpy(cmd->key, c_req->key, c_req->key_sz);
				cmd->key_sz = c_req->key_sz;

				cmd->value = new char[c_req->value_sz+1];
				memset(cmd->value,0,c_req->value_sz);
				memcpy(cmd->value, c_req->value, c_req->value_sz);
				cmd->value_sz = c_req->value_sz;

				cmd->min_proposal_num->proposal_client_id = c_req->accepted_proposal->proposal_client_id;
				cmd->min_proposal_num->proposal_num = c_req->accepted_proposal->proposal_num;
				cmd->accepted_proposal->proposal_client_id = c_req->accepted_proposal->proposal_client_id;
				cmd->accepted_proposal->proposal_num = c_req->accepted_proposal->proposal_num;

				mp_ks_map_mutex.lock();
				if(apply_last_write(string(cmd->key),c_req->index)) {
					#ifdef DEBUG_FLAG
					cout<<"Applied one write"<<endl;
					#endif
				}
				mp_ks_map_mutex.unlock();

				reply->set_code(PaxosResponse::ACK);
				

				reply->set_proposalclientid(cmd->accepted_proposal->proposal_client_id);
				reply->set_proposalnum(cmd->accepted_proposal->proposal_num);
				
				reply->set_commandid(cmd->command_id);
				switch (c_req->command_type) {
					case READ:
						 reply->set_commandtype(PaxosResponse::READ); break;
					case WRITE:
						 reply->set_commandtype(PaxosResponse::WRITE); break;
					default:
						cout<<"get_ctype: wrong command type ";	
				}
				
				reply->set_index(cmd->index);
				reply->set_keysz(cmd->key_sz);
				reply->set_key(c_req->key,cmd->key_sz);
				reply->set_valuesz(cmd->value_sz);
				reply->set_value(c_req->value,cmd->value_sz);
				cl->next_available_slot.erase(c_req->index);
				
			} else {
				#ifdef DEBUG_FLAG
				cout<<"Accpet request processing, sending NACK"<<endl;
				#endif
				reply->set_code(PaxosResponse::NACK);
				reply->set_keysz(0);
				reply->set_valuesz(0);
				reply->set_proposalclientid(cmd->min_proposal_num->proposal_client_id);
				reply->set_proposalnum(cmd->min_proposal_num->proposal_num);
			}
			#ifdef DEBUG_FLAG
			cout<<"ACCEPT request completed"<<endl;
			#endif
			print_current_log_db_state();
	}

	
	log_map_mutex.unlock();

	return Status::OK;
} // End of service implementation


return_code
get_c_return_code(PaxosResponse::ReturnCode type) {
	switch (type) {
		case PaxosResponse::ACK:
			return ACK;
		case PaxosResponse::ERROR:
			return ERROR;
		case PaxosResponse::OK:
			return OK;
		case PaxosResponse::NACK:
			return NACK;
		default:
			cout<<"get_ctype: wrong return code";	
	}
	return OK;
}

request_type
get_c_command_type(PaxosResponse::CommandType type) {
	switch (type) {
		case PaxosResponse::READ:
			return READ;
		case PaxosResponse::WRITE:
			return WRITE;
		default:
			cout<<"get_ctype: wrong return code";	
	}
	return READ;
}
request_type
get_c_command_type(PaxosRequest::CommandType type) {
	switch (type) {
		case PaxosRequest::READ:
			return READ;
		case PaxosRequest::WRITE:
			return WRITE;
		default:
			cout<<"get_ctype: wrong return code"<<endl;	
	}
	return READ;
}

mp_request_type
get_c_mp_req_type(PaxosRequest::RequestType type) {
	switch (type) {
		case PaxosRequest::PREPARE:
			return PREPARE;
		case PaxosRequest::ACCEPT:
			return ACCEPT;
		default:
			cout<<"get_ctype: wrong return code"<<endl;	
	}
	return ACCEPT;
}


void print_mp_req_type(mp_request_type type) {
	switch (type) {
		case PREPARE:
			cout<< "PREPARE"<<endl; break;
		case ACCEPT:
			cout<< "ACCEPT"<<endl; break;
		default:
			cout<< "Unknown"<< endl;
	}
}
void print_command_type (request_type type) {
	switch (type) {
		case READ:
			cout<< "READ"<<endl; break; 
		case KeyStoreRequest::WRITE:
			cout<< "WRITE"<<endl; break;
		default:
			cout<< "Unknown"<< endl;	
	}

}
void
print_return_code(return_code type) {
	switch (type) {
		case ACK:
			cout<< "ACK"<<endl; break;
		case ERROR:
			cout<< "ERROR"<<endl;break;
		case OK:
			cout<< "OK"<<endl;break;
		case NACK:
			cout<< "NACK"<<endl;break;
		default:
			cout<<"Unknown"<<endl;	
	}
}
void 
print_command_t(command_t *req, bool request) {
	//printing_mutex.lock();
	cout<< " Printing MP request" <<endl;
	if(request)
	 { cout<<" 	mp_request_type  :" ;
	   print_mp_req_type(req->mp_req_type);}
	if(!request){
		cout<<" 	Return_code 		 :" ; 
		print_return_code(req->code);
	}
	cout<<" 	command_type  	 :" ; print_command_type(req->command_type);
	cout<<" 	Index  	 :" << (req->index)<<endl;
	cout<<" 	min_proposal_num :" <<endl;
	cout<<"	 		client_id  :"<<req->min_proposal_num->proposal_client_id<<endl;
	cout<<"	 		proposal_num :"<<req->min_proposal_num->proposal_num<<endl;
	cout<<" 	accepted_proposal:" <<endl;
	cout<<"	 		client_id  :"<<req->accepted_proposal->proposal_client_id<<endl;
	cout<<"	 		proposal_num :"<<req->accepted_proposal->proposal_num<<endl;
	cout<<" 	command_id 		 :"<<req->command_id<<endl;
	if(req->key_sz)
	{cout<<" 	key      		 :"<<req->key<<endl;}
	cout<<" 	key_sz   		 :"<<req->key_sz<<endl;
	if(req->value_sz)
	{cout<<" 	value   		 :"<<req->value<<endl;}
	cout<<" 	value_sz		 :"<<req->value_sz<<endl;
	//printing_mutex.unlock();
}


void RunMPServer(string server_address) {

	//std::string server_address("0.0.0.0:50051");
	mp_service_impl service;

	ServerBuilder builder;

	std::string delimiter = ":";
	//chanding address to internal address
  	std::string port = server_address.substr(server_address.find(delimiter)+1, server_address.length());
  	server_address = get_ipaddr() + port;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "MP Server listening on " << server_address << std::endl;

	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	server->Wait();
}