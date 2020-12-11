#include "mp.h"
#include "utils.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;


void
delete_mp_request_t(mp_message_request_t* mp_req){
	if(mp_req->key) delete mp_req->key;
	if(mp_req->value) delete mp_req->value;
	if(mp_req->vec_clk) delete mp_req->vec_clk;
	delete mp_req;
}

command_t*
extract_request_payload_to_command_t(const PaxosRequest* request) {
	command_t *c_req = new command_t;
	c_req->mp_req_type = get_c_mp_req_type(request->mptype());
	c_req->command_type = get_c_command_type(request->commandtype());
	if (c_req->code!= NACK && Response->key().size()) {
		#ifdef DEBUG_FLAG
			cout<<"	"<<__func__<<"request key size= "<<request->key().size();
			cout<<"	"<<__func__<<"request key received = "<<request->key();
		#endif
		c_req->key = new char[request->keysz()+1];
		memset(c_req->key, 0, request->keysz()+1);
		memcpy(c_req->key, request->key().c_str(), request->keysz());
		#ifdef DEBUG_FLAG
			cout<<"	"<<__func__<<"request after mem_cpy = "<<c_req->key;
		#endif
	}
	if (c_req->code != NACK && request->valuesz()) {
		#ifdef DEBUG_FLAG
			cout<<"	"<<__func__<<"request value size= "<<request->value().size()<<endl;
			cout<<"	"<<__func__<<"request value received = "<<request->value()<<endl;
		#endif
		c_req->value = new char[request->valuesz()+1];
		memset(c_req->value, 0, request->valuesz()+1);
		memcpy(c_req->value, request->value().c_str(), request->valuesz());
		#ifdef DEBUG_FLAG
				cout<<__func__<<"request value after mem_cpy = "<<c_req->value<<endl;
		#endif
	}

	c_req->accepted_proposal.proposal_client_id = request->proposalclientid();
	c_req->accepted_proposal.proposal_num = request->proposalnum();
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
		print_command_t(c_req, 0);
	#endif
	commands_list_t *cl =  NULL;

	log_lock_mutex.lock();
	if (log_map.find(string(c_req->key)) != log_map.end()){
		cl =  log_map.find(string(c_req->key));		
		//cl->cmd_vec[curr_idx] = cmd;
	} else {
		// TODO lock
		cl = new commands_list_t;
		cl->cmd_vec.reserve(INIT_COMMAND_LENGTH); 
		for (int i = 0; i < INIT_COMMAND_LENGTH; i++ ) {
			command_t *cmd = new command_t;
			cmd->min_proposal_num.proposal_client_id = -1;
			cmd->min_proposal_num.proposal_num = -1;
			cmd->accepted_proposal.proposal_client_id = -1;
			cmd->accepted_proposal.proposal_num = -1;
			cmd->key_sz = 0;
			cmd->key = NULL;
			cmd->value_sz = 0;
			cmd->value = NULL;
			cmd->command_id = 0;
			cmd->index = -1; 
			cl->cmd_vec[i] = cmd ;
			cl->next_available_slot.insert(i);
		}
		cl->last_applied_index = -1;
		
		//cl->cmd_vec[curr_idx] = cmd;	
	}
	log_map[string(curr_cmd->key)] = cl;
	

	cmd = cl[c_req->index];
	if(c_req->mp_req_type ==  PREPARE) {
		bool update = false;
		#ifdef DEBUG_FLAG
			cout<< "Prepare request from client_id: "<< c_req->accepted_proposal.proposal_client_id<<endl;
		#endif
			if (c_req->accepted_proposal.proposal_num > cmd->min_proposal_num.proposal_num ) {
				update = true;
			}
			if (c_req->accepted_proposal.proposal_num == cmd->min_proposal_num.proposal_num) {
				if (c_req->accepted_proposal.proposal_client_id > cmd->min_proposal_num.proposal_client_id ) {
					update = true;
				}
			}
			
			if (update) {
				cmd->min_proposal_num.proposal_client_id = c_req->accepted_proposal.proposal_client_id;
				cmd->min_proposal_num.proposal_num = c_req->accepted_proposal.proposal_num;
				reply->set_code(PaxosResponse::ACK);

				reply->set_proposalclientid(cmd->accepted_proposal.proposal_client_id);
				reply->set_proposalnum(cmd->accepted_proposal.proposal_num);
				
				reply->set_commandid(cmd->command_id);
				eply->set_commandtype(cmd->command_type);
				reply->set_index(cmd->index);
				reply->set_keysz(cmd->key_sz);
				reply->set_key(cmd->key, cmd->key_sz);
				reply->set_valuesz(cmd->value_sz);
				reply->set_value(cmd->value, cmd->value_sz);
			} else {
				reply->set_code(PaxosResponse::NACK);
				reply->set_proposalclientid(cmd->min_proposal_num.proposal_client_id);
				reply->set_proposalnum(cmd->min_proposal_num.proposal_num);
			}

	} else {
		bool update = false;
		#ifdef DEBUG_FLAG
			cout<< "Accept request from client_id: "<< c_req->accepted_proposal.proposal_client_id<<endl;
		#endif
			if (c_req->accepted_proposal.proposal_num >= cmd->min_proposal_num.proposal_num ) {
				update = true;
			}
			if (c_req->accepted_proposal.proposal_num == cmd->min_proposal_num.proposal_num) {
				if (c_req->accepted_proposal.proposal_client_id >= cmd->min_proposal_num.proposal_client_id ) {
					update = true;
				}
			}mp_request_type mp_req_type;

			if(update) {
				cmd->command_type = c_req->command_type;
				cmd->command_id = c_req->command_id;
				cmd->index = curr_idx;
				if(cmd->key_sz) delete(cmd->key);
				if(cmd->value_sz) delete(cmd->value);

				cmd->key = new char[cmd->key_sz+1];
				memset(cmd->key,0,cmd->key_sz);
				memcpy(cmd->key, c_req->key, cmd->key_sz);
				cmd->value = new char[cmd->value_sz+1];
				memset(cmd->value,0,cmd->value_sz);
				memcpy(cmd->value, c_req->value, cmd->value_sz);

				cmd->min_proposal_num.proposal_client_id = c_req->accepted_proposal.proposal_client_id;
				cmd->min_proposal_num.proposal_num = c_req->accepted_proposal.proposal_num;
				cmd->accepted_proposal.proposal_client_id = c_req->accepted_proposal.proposal_client_id;
				cmd->accepted_proposal.proposal_num = c_req->accepted_proposal.proposal_num;

				mp_ks_map_mutex.lock();
				if(apply_last_write(string(cmd->key),index)) {
					cout<<"Applied one write"<<endl;
				}
				mp_ks_map_mutex.unlock();

				reply->set_code(PaxosResponse::ACK);
				

				reply->set_proposalclientid(cmd->accepted_proposal.proposal_client_id);
				reply->set_proposalnum(cmd->accepted_proposal.proposal_num);
				
				reply->set_commandid(cmd->command_id);
				eply->set_commandtype(cmd->command_type);
				reply->set_index(cmd->index);
				reply->set_keysz(0);
				reply->set_valuesz(0);
				cl->next_available_slot.remove(c_req->index);
				
			} else {
				reply->set_code(PaxosResponse::NACK);
				reply->set_proposalclientid(cmd->min_proposal_num.proposal_client_id);
				reply->set_proposalnum(cmd->min_proposal_num.proposal_num);
			}
	}

	
	log_lock_mutex.unlock();

	return Status::OK;
} // End of service implementation


void RunCMServer(string server_address) {

	//std::string server_address("0.0.0.0:50051");
	mp_service_impl service;

	ServerBuilder builder;

	std::string delimiter = ":";
	//chanding address to internal address
  	std::string port = server_address.substr(server_address.find(delimiter)+1, server_address.length());
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(get_ipaddr() + port, grpc::InsecureServerCredentials());
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	std::unique_ptr<Server> server(builder.BuildAndStart());
	std::cout << "Server listening on " << server_address << std::endl;

	// Wait for the server to shutdown. Note that some other thread must be
	// responsible for shutting down the server for this call to ever return.
	server->Wait();
}