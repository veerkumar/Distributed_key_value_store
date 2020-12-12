#include "commons.h"
#include "cm.h"
#include "mp.h"
#include "server.h"
#include "utils.h"

mutex abd_ks_map_mutex;
map<string,value_t*> abd_ks_map;

mutex cm_ks_map_mutex;
map<string,value_t*> cm_ks_map;

mutex cm_pq_mutex;
priority_queue <cm_message_request_t*, vector<cm_message_request_t*>, vectclk_comparator > cm_message_pq;

mutex cm_outque_mutex;
std::queue<cm_message_request_t* > cm_message_outqueue;

mutex cm_t_mutex;
int *t;
int serverlist_size;

int mynodenumber;


cm_server_connections *cm_connection_obj;

mutex mp_ks_map_mutex;
map<string,value_t*> mp_ks_map;

mp_server_connections *mp_connection_obj;

mutex log_map_mutex;
map<string, commands_list_t*> log_map;
mutex mp_mutex;
mutex printing_mutex;


uint32_t get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<uint32_t> distribution(0, INT_MAX);
	return distribution(generator);
}

void enqueue_in_outgoing_queue(value_t *req){
	cm_message_request_t *cm_req =  new cm_message_request_t;
	cm_req->nodenum = mynodenumber;

	cm_req->key = new char[req->key_sz+1];
	memset(cm_req->key, 0,req->key_sz+1);
	cm_req->value = new char[req->value_sz+1];
	memset(cm_req->value, 0, req->value_sz+1);
	cm_req->vec_clk = new int[serverlist_size];

	cm_req->key_sz = req->key_sz;
	cm_req->value_sz = req->value_sz;
	cm_req->vecclk_sz = serverlist_size;

	memcpy(cm_req->key, req->key, req->key_sz);
	memcpy(cm_req->value, req->value, req->value_sz);
	memcpy(cm_req->vec_clk, t, INT_SIZE*serverlist_size);

	cm_outque_mutex.lock();
	cm_message_outqueue.push(cm_req);
	cm_outque_mutex.unlock();
}

bool apply_last_write(string key, uint32_t index) {

	commands_list_t *cl =  log_map.find(string(key))->second;

	command_t *prev_cmd = NULL, *temp_cmd;
	uint32_t curr_idx = 0;

	for (auto it = cl->cmd_vec.begin(); it != cl->cmd_vec.end(); it++) {
		temp_cmd = *it;
		 if(curr_idx > index) {
		 	break;
		 } else {
		 	if (temp_cmd->command_type == WRITE) {
		 		prev_cmd = temp_cmd;
		 	}
		 	curr_idx ++;
		 }
	}

	if(prev_cmd == NULL ) {
		return 0;
	}

	if (mp_ks_map.find(key) !=mp_ks_map.end()) {
		value_t  *temp_value_t = mp_ks_map[key];
					
		#ifdef DEBUG_FLAG
			cout<< "	MP: Writing with new value" <<endl;
		#endif
			//value can change so first delete and the allocate new
		delete temp_value_t->value;
		temp_value_t->value = new char[prev_cmd->value_sz+1];
		memset(temp_value_t->value, 0, prev_cmd->value_sz+1);
		temp_value_t->value_sz = prev_cmd->value_sz;
		memcpy(temp_value_t->value, prev_cmd->value, prev_cmd->value_sz);

		mp_ks_map[key] = temp_value_t;

	} else {
		value_t *temp_value_t = new value_t;
		temp_value_t->tag.integer = 0;
		temp_value_t->tag.client_id = 0;

		temp_value_t->key = new char[prev_cmd->key_sz+1];
		memset(temp_value_t->key,0, prev_cmd->key_sz+1);
		temp_value_t->key_sz = prev_cmd->key_sz;
		memcpy(temp_value_t->key, prev_cmd->key, prev_cmd->key_sz);

		temp_value_t->value = new char[prev_cmd->value_sz+1];
		memset(temp_value_t->value, 0, prev_cmd->value_sz+1);
		temp_value_t->value_sz = prev_cmd->value_sz;
		memcpy(temp_value_t->value, prev_cmd->value, prev_cmd->value_sz);

		mp_ks_map[key] = temp_value_t;
	}
	return 1;
}


class key_store_service_impl: public KeyStoreService::Service {
	Status KeyStoreRequestHandler (ServerContext* context, const  KeyStoreRequest* request,
			KeyStoreResponse* reply) override {
		#ifdef DEBUG_FLAG
		std::cout << "\n \n Got the New message from Client"<<endl;

		cout<<"			Reqtype   :"<<getstring_grpc_request_type(request->type())<<endl; 
		cout <<"			Protocol  :"<<request->protocol()<<endl;
		cout <<"			Integer   :"<<request->integer()<<endl;
		cout <<"			Client_id :"<<request->clientid()<<endl;
		cout <<"			Key       :"<<request->key()<<endl;
		cout <<"			keysz 	  :"<<request->keysz()<<endl;
		if(request->valuesz())
			cout <<"			Value     :"<<request->value()<<endl;
		cout <<"			valuesz    :"<<request->valuesz()<<endl;
		#endif
		if (request->protocol() == KeyStoreRequest::CM) {
			reply->set_protocol(KeyStoreResponse::CM);

			#ifdef DEBUG_FLAG
				std::cout << "	Got CM protocol "<<endl;
			#endif
			// 
			// reply->set_protocol(KeyStoreResponse::CM);

			if (request->type() ==  KeyStoreRequest::READ) {

				#ifdef DEBUG_FLAG
				std::cout << "	Got request type: READ   "<<endl;
				#endif
				/* Fetch the value for the given key and send the response */
				if(request->key() != "00000" && (cm_ks_map.find(string(request->key().c_str())) != cm_ks_map.end())) {
					/* Found the key */
					cm_ks_map_mutex.lock();
					value_t *temp_value_t = cm_ks_map[string(request->key().c_str())];
					reply->set_key(temp_value_t->key,temp_value_t->key_sz);
					reply->set_keysz(temp_value_t->key_sz);
					reply->set_value(temp_value_t->value,temp_value_t->value_sz);
					#ifdef DEBUG_FLAG
					cout<<"Returning value"<<temp_value_t->value<<endl;
					#endif
					reply->set_valuesz(temp_value_t->value_sz);
					reply->set_code(KeyStoreResponse::OK);
					cm_ks_map_mutex.unlock();
				} else {
					cout<<"	In CM Read, key doesnt exists" << endl;
					reply->set_integer(0);
					reply->set_clientid(0);
					reply->set_keysz(0);
					reply->set_valuesz(0);
					reply->set_code(KeyStoreResponse::ERROR);
				}

			}

			if (request->type() ==  KeyStoreRequest::WRITE) {
				value_t *temp_value_t ;
				cm_ks_map_mutex.lock();
				if(cm_ks_map.find(string(request->key().c_str())) != cm_ks_map.end()) {
					temp_value_t = cm_ks_map[string(request->key().c_str())];
					
					#ifdef DEBUG_FLAG
						cout<< "	CM: Writing value with new value" <<endl;
					#endif
						//value can change so first delete and the allocate new
						delete temp_value_t->value;
						temp_value_t->value = new char[request->valuesz()+1];
						memset(temp_value_t->value, 0, request->valuesz()+1);
						temp_value_t->value_sz = request->valuesz();
						memcpy(temp_value_t->value, request->value().c_str(), request->valuesz());

						cm_ks_map[string(request->key().c_str())] = temp_value_t;
				} else {
					#ifdef DEBUG_FLAG
						cout<< "	CM: Writing new value" <<endl;
					#endif
					temp_value_t = new value_t;
					temp_value_t->tag.integer = request->integer();
					temp_value_t->tag.client_id = request->clientid();

					temp_value_t->key = new char[request->keysz()+1];
					memset(temp_value_t->key,0, request->keysz()+1);
					temp_value_t->key_sz = request->keysz();
					memcpy(temp_value_t->key, request->key().c_str(), request->keysz());

					temp_value_t->value = new char[request->valuesz()+1];
					memset(temp_value_t->value, 0, request->valuesz()+1);
					temp_value_t->value_sz = request->valuesz();
					memcpy(temp_value_t->value, request->value().c_str(), request->valuesz());

					cm_ks_map[string(temp_value_t->key)] = temp_value_t;
					
				}
				//Append this new value to outqueue
				t[mynodenumber] = t[mynodenumber]+1;
				enqueue_in_outgoing_queue(temp_value_t);
				cm_ks_map_mutex.unlock();
				reply->set_integer(0);
				reply->set_clientid(0);
				reply->set_keysz(0);
				reply->set_valuesz(0);
				reply->set_code(KeyStoreResponse::OK);

			}
			
		} else  if (request->protocol() == KeyStoreRequest::ABD) {
			#ifdef DEBUG_FLAG
			std::cout << "	Got ABD protocol "<<endl;
			#endif
			reply->set_code(KeyStoreResponse::ACK);
			reply->set_protocol(KeyStoreResponse::ABD);
			/*In some cases, We dont need to return the key and values*/
			reply->set_keysz(0);
			reply->set_valuesz(0);

			if (request->type() ==  KeyStoreRequest::READ_QUERY) {
				#ifdef DEBUG_FLAG
				std::cout << "	Got request type: READ_QUERY  "<<endl;
				#endif
				/* Fetch the value for the given key and send the response */
				if(request->key() != "00000" && (abd_ks_map.find(string(request->key().c_str())) != abd_ks_map.end())) {
					/* Found the key */
					abd_ks_map_mutex.lock();
					value_t *temp_value_t = abd_ks_map[string(request->key().c_str())];
					reply->set_integer(temp_value_t->tag.integer);
					reply->set_clientid(temp_value_t->tag.client_id);
					reply->set_key(temp_value_t->key,temp_value_t->key_sz);
					reply->set_keysz(temp_value_t->key_sz);
					reply->set_value(temp_value_t->value,temp_value_t->value_sz);
					reply->set_valuesz(temp_value_t->value_sz);
					abd_ks_map_mutex.unlock();
				} else {
					cout<<"	In Read query, key doesnt exists" << endl;
					reply->set_integer(0);
					reply->set_clientid(0);
					reply->set_keysz(0);
					reply->set_valuesz(0);
				}
			}

			if (request->type() ==  KeyStoreRequest::READ) {
				#ifdef DEBUG_FLAG
				std::cout << "	Got request type: READ"<<endl;
				#endif
			}

			if (request->type() ==  KeyStoreRequest::WRITE_QUERY) {
				/* This query should return tag value, no need of writing value */
				#ifdef DEBUG_FLAG
				std::cout << "	Got request type: WRITE_QUERY" <<endl;
				#endif
				value_t *temp_value_t;
				abd_ks_map_mutex.lock();
				if(abd_ks_map.find(string(request->key().c_str())) != abd_ks_map.end()) {
					 temp_value_t = abd_ks_map[string(request->key().c_str())];	
				} else {
					temp_value_t = abd_ks_map[string("00000")];
					/* Do not allocate memory for new key, 
					   as someone else might issue read request*/
					#ifdef DEBUG_FLAG
					cout<< "Didnt find this key, hence returning value from 00000" <<endl;
					#endif 
					
				}
				reply->set_integer(temp_value_t->tag.integer);
				reply->set_clientid(temp_value_t->tag.client_id);
				abd_ks_map_mutex.unlock();

			}
			if (request->type() ==  KeyStoreRequest::WRITE) {
				#ifdef DEBUG_FLAG
				std::cout << "	Got request type: WRITE" <<endl;
				#endif
				value_t *temp_value_t;
				abd_ks_map_mutex.lock();
				if(abd_ks_map.find(string(request->key().c_str())) != abd_ks_map.end()) {
					 temp_value_t = abd_ks_map[string(request->key().c_str())];
					 if(temp_value_t->tag.integer < request->integer()) {
					#ifdef DEBUG_FLAG
						cout<< "Writing value with new value" <<endl;
					#endif
						temp_value_t->tag.integer = request->integer();
						temp_value_t->tag.client_id = request->clientid();

						//value can change so first delete and the allocate new
						delete temp_value_t->value;
						temp_value_t->value = new char[request->valuesz()+1];
						memset(temp_value_t->value, 0, request->valuesz()+1);
						temp_value_t->value_sz = request->valuesz();
						memcpy(temp_value_t->value, request->value().c_str(), request->valuesz());

						abd_ks_map[string(request->key().c_str())] = temp_value_t;
					 } else if (temp_value_t->tag.integer == request->integer()) {

					 	 if(temp_value_t->tag.client_id < request->clientid()) {
					#ifdef DEBUG_FLAG
						cout<< "Writing value from new bigger client id" <<endl;
					#endif
					 	 	temp_value_t->tag.client_id = request->clientid();

						//value can change so first delete and the allocate new
							delete temp_value_t->value;
							temp_value_t->value = new char[request->valuesz()+1];
							memset(temp_value_t->value, 0, request->valuesz()+1);
							temp_value_t->value_sz = request->valuesz();
							memcpy(temp_value_t->value, request->value().c_str(), request->valuesz());
							abd_ks_map[string(request->key().c_str())] = temp_value_t;
					 	 }

					 } else {
					 	cout << "Ignoring this Interger value :"<<request->integer()<<", as KeyStore has more latest value:"<< temp_value_t->tag.integer<< endl;
					 }
				} else {
					#ifdef DEBUG_FLAG
						cout<< "Writing new value" <<endl;
					#endif
					temp_value_t = new value_t;
					temp_value_t->tag.integer = request->integer();
					temp_value_t->tag.client_id = request->clientid();

					temp_value_t->key = new char[request->keysz()+1];
					memset(temp_value_t->key, 0, request->keysz()+1);
					temp_value_t->key_sz = request->keysz();
					memcpy(temp_value_t->key, request->key().c_str(), request->keysz());

					temp_value_t->value = new char[request->valuesz()+1];
					memset(temp_value_t->value, 0, request->valuesz()+1);
					temp_value_t->value_sz = request->valuesz();
					memcpy(temp_value_t->value, request->value().c_str(), request->valuesz());

					abd_ks_map[string(request->key().c_str())] = temp_value_t;
				}

				#ifdef DEBUG_FLAG
				cout<< "	Size of key_value store : " << abd_ks_map.size();
				#endif
				abd_ks_map_mutex.unlock();
				
				reply->set_integer(0);
				reply->set_clientid(0);
				reply->set_keysz(0);
				reply->set_valuesz(0);
				
			} // end of WRITE 
		} else {
			//MP protocol
			mp_mutex.lock();
			reply->set_protocol(KeyStoreResponse::MP);

			#ifdef DEBUG_FLAG
				std::cout << "Got MP protocol "<<endl;
			#endif
			if (request->type() ==  KeyStoreRequest::READ) {

				#ifdef DEBUG_FLAG
				std::cout << "Got request type: READ   "<<endl;
				#endif
				uint32_t index = insert_command(request);
				/* Fetch the value for the given key and send the response */
				//if(request->key() != "00000" && (mp_ks_map.find(string(request->key().c_str())) != mp_ks_map.end())) {
				mp_ks_map_mutex.lock();
				if(apply_last_write(request->key().c_str(),index)) {
					/* Found the key */
					cout<<"Found the key" <<endl;
					value_t *temp_value_t = mp_ks_map[string(request->key().c_str())];
					reply->set_integer(temp_value_t->tag.integer);
					reply->set_clientid(temp_value_t->tag.client_id);
					reply->set_key(temp_value_t->key,temp_value_t->key_sz);
					reply->set_keysz(temp_value_t->key_sz);
					reply->set_value(temp_value_t->value,temp_value_t->value_sz);
					reply->set_valuesz(temp_value_t->value_sz);
				} else {
					cout<<"	In MP Read, key doesnt exists" << endl;
					reply->set_integer(0);
					reply->set_clientid(0);
					reply->set_keysz(0);
					reply->set_valuesz(0);
					reply->set_code(KeyStoreResponse::ERROR);
				}
				mp_ks_map_mutex.unlock();
				print_current_log_db_state();
					
			} else {
				#ifdef DEBUG_FLAG
				std::cout << "	 Got request type: WRITE   "<<endl;
				#endif
				uint32_t index = insert_command(request);

				cout<<" Inserted at index :" <<index;

				// First send message to all server till this command is not written is not written

				reply->set_integer(0);
				reply->set_clientid(0);
				reply->set_keysz(0);
				reply->set_valuesz(0);
				reply->set_code(KeyStoreResponse::OK);
				print_current_log_db_state();
			}
			mp_mutex.unlock();
	}
		
		return Status::OK;
	} // End of service implementation
};

command_t* get_new_alloc_command(int client_id, int proposal_num) {
	command_t *cmd = new command_t;
	cmd->command_id = get_random_number() + (uint32_t) client_id;
	
	cmd->min_proposal_num =  new proposal_t;
	cmd->min_proposal_num->proposal_client_id = client_id;
	cmd->min_proposal_num->proposal_num = proposal_num;

	cmd->accepted_proposal =  new proposal_t;
	cmd->accepted_proposal->proposal_client_id = client_id;
	cmd->accepted_proposal->proposal_num = proposal_num;
	cmd->key_sz = 0;
	cmd->key = NULL;
	cmd->value_sz = 0;
	cmd->value = NULL;
	return cmd;
}

void fill_command_from_request_t(const KeyStoreRequest *req, command_t **cmd) {
	(*cmd)->key = new char[req->keysz()+1];
	memset ((*cmd)->key, 0 ,req->keysz()+1 );
	memcpy ((*cmd)->key, req->key().c_str(), req->keysz());
	(*cmd)->key_sz = req->keysz();

	(*cmd)->value = new char[req->valuesz()+1];
	memset ((*cmd)->value, 0 ,req->valuesz()+1 );
	memcpy ((*cmd)->value, req->value().c_str(), req->valuesz());
	(*cmd)->value_sz = req->valuesz();
	(*cmd)->command_type =  get_c_request_type(req->type());
}

 
void delete_command_t (command_t *cmd) {
	if(cmd->key_sz) delete cmd->key;
	if(cmd->value_sz)delete cmd->value;
	if(cmd->min_proposal_num) delete cmd->min_proposal_num;
	if(cmd->accepted_proposal) delete cmd->accepted_proposal;
	delete cmd;
}

command_t*
get_highest_accepted_proposal(vector<command_t*> &vec_c_resp, bool prepare_phase) {
	#ifdef DEBUG_FLAG
		cout<<"\nget_highest_accepted_proposal function" <<endl;
	#endif
	command_t*  max_resp = vec_c_resp[0];
	// int total_size = vec_c_resp.size();
	// int i = 0;
	int max_nack_proposal_num = -1;
	for(auto it = vec_c_resp.begin(); it!= vec_c_resp.end();) {
		if ((*it)->code == NACK) {
			max_resp = *it;
			if( (*it)->accepted_proposal->proposal_num > max_nack_proposal_num) {
					max_nack_proposal_num = (*it)->accepted_proposal->proposal_num;
			}
			//Keep atmost 1, for returning.
			if (vec_c_resp.size()>1){
				delete_command_t (max_resp);
			}
			vec_c_resp.erase(it);
			continue;
		}
		it++;
	}
	#ifdef DEBUG_FLAG
		cout<<"Done checking NACK max_nack_proposal_num:" <<max_nack_proposal_num<<endl;
	#endif

	if (vec_c_resp.size() !=0) {
		cout<<"Size of responses:"<< vec_c_resp.size()<< endl;
		 max_resp = vec_c_resp[0];
		 auto it = vec_c_resp.begin();
		 it++;
		for(;it!= vec_c_resp.end();) {
				if ((*it)->accepted_proposal->proposal_num >= max_resp->accepted_proposal->proposal_num) {
					if ((*it)->accepted_proposal->proposal_num == max_resp->accepted_proposal->proposal_num) {
						if((*it)->accepted_proposal->proposal_client_id >= max_resp->accepted_proposal->proposal_client_id){
							delete_command_t (max_resp);
							max_resp = *it;
							vec_c_resp.erase(it);
							continue;
						} else {
							vec_c_resp.erase(it);
							continue;
						}
					}
				}
				delete_command_t(*it);
				vec_c_resp.erase(it);
		}
		cout<<"Selected max_resp :";
		print_command_t(max_resp,1);
	} else {
		max_resp->accepted_proposal->proposal_num = max_nack_proposal_num + 1;
		#ifdef DEBUG_FLAG
			cout<<" All responses were NACK and "<<endl;
		#endif
		return max_resp;
	}

	if (max_nack_proposal_num > -1 && !prepare_phase) {
		#ifdef DEBUG_FLAG
			cout<<"In accept phase, receviced one NACK"<<endl;
		#endif
		delete_command_t(max_resp);
		command_t *cmd = new command_t;
		cmd->code = NACK;
		cmd->key_sz = 0;
		cmd->key = NULL;
		cmd->value_sz = 0;
		cmd->value = NULL;
		cmd->accepted_proposal = new proposal_t;
		cmd->min_proposal_num = new proposal_t;;
		cmd->accepted_proposal->proposal_num = max_nack_proposal_num + 1;
		return cmd;
	}

	if(max_resp->accepted_proposal->proposal_num >= -1 && max_nack_proposal_num > -1) {
		// It means we have some node which has higher proposal number and some has lower proposal number
		// Hence we need to restart prepare phase with higher proposalnumber as in accept phase it will be rejected.
		#ifdef DEBUG_FLAG
			cout<<"Maximum proposal number: -1, Some higher client_id had some value but over proposal number was small"<<endl;
		#endif
		max_resp->code = NACK;
		max_resp->key_sz = 0;
		max_resp->value_sz = 0;
		max_resp->accepted_proposal->proposal_num = max_nack_proposal_num + 1;
	} else if(max_resp->accepted_proposal->proposal_num == -1){
		#ifdef DEBUG_FLAG
			cout<<"Maximum proposal number: -1, all client_id had no value"<<endl;
		#endif
		max_resp->code = OK;
		max_resp->key_sz = 0;
		max_resp->value_sz = 0;
	}

	// if(max_resp->accepted_proposal->proposal_num > -1 && max_nack_proposal_num > -1) {
	// 	// It means we have some node which has higher proposal number and some has lower proposal number
	// 	// Hence we need to restart prepare phase with higher value as, in accpet phase it will get rejected.
	// 	// Also will be overwritten 
	// }
	
	#ifdef DEBUG_FLAG 
		cout<<"\n"<<__func__<< ": ";
		cout<<"Selected max value:" <<endl;
		print_command_t(max_resp,0);
	#endif

	return max_resp;
}
void update_local_values(command_t *orig_cmd) {
	cout<<"Updating local copy"<<endl;
	commands_list_t *cl =  log_map.find(string(orig_cmd->key))->second;
	command_t *temp_cmd = cl->cmd_vec[orig_cmd->index];
	temp_cmd->min_proposal_num->proposal_client_id = orig_cmd->accepted_proposal->proposal_client_id;
	temp_cmd->min_proposal_num->proposal_num = orig_cmd->accepted_proposal->proposal_num;
	temp_cmd->index = orig_cmd->index;
	//temp_cmd->command_id = 0;
}

command_t*  
propose (command_t *orig_cmd) {
	command_t *temp_cmd = orig_cmd;
	command_t *sent_cmd = NULL;
	vector<command_t*> vec_c_resp ;
	while (1) {
		//Propose phase
		update_local_values(orig_cmd);
		temp_cmd->mp_req_type =  PREPARE;
		promise<vector<command_t*>> pm =  promise<vector<command_t*>>();
    	future <vector<command_t*>> fu = pm.get_future();
    	//cout<<"New proposal_num selected is :" << temp_cmd->accepted_proposal->proposal_num<<endl;
    	//print_command_t(temp_cmd,1);
		thread t1(send_message_to_all_mp_server, ref(pm), temp_cmd);
		t1.detach();
		vec_c_resp = fu.get();
		#ifdef DEBUG_FLAG
			cout<<"PREPARE phase completed"<<endl;
		#endif
		
		// Find the highest accepted proposal number
		temp_cmd = get_highest_accepted_proposal (vec_c_resp, 1);
		if (temp_cmd->code == NACK) {
			orig_cmd->accepted_proposal->proposal_num = temp_cmd->accepted_proposal->proposal_num ;
			cout<<"New proposal_num selected is :" << orig_cmd->accepted_proposal->proposal_num<<endl;
			delete_command_t(temp_cmd);
			temp_cmd = orig_cmd;
			cout<<"Restarting the proposal"<<endl;
			//Since we got all NACK, it means over proposal number was lowest, no meaning of accept phase
			continue;
		}
		if (temp_cmd->code == OK) {
			// we got all -1, It means our value will be accepted with current proposal number
			delete_command_t(temp_cmd);
			temp_cmd = orig_cmd;
		}

		// Modify, datatype, proposal number

		temp_cmd->mp_req_type =  ACCEPT;
		temp_cmd->accepted_proposal->proposal_client_id = orig_cmd->accepted_proposal->proposal_client_id;
		temp_cmd->accepted_proposal->proposal_num = orig_cmd->accepted_proposal->proposal_num;
		sent_cmd = temp_cmd;
		#ifdef DEBUG_FLAG
			cout<<"---------------------------------"<<endl;
			cout<<"ACCEPT phase starting"<<endl;
		#endif
		promise<vector<command_t*>> pm1 =  promise<vector<command_t*>>();
    	future <vector<command_t*>> fu1 = pm1.get_future();
		thread t2(send_message_to_all_mp_server, ref(pm1), temp_cmd);
		t2.detach();
		vec_c_resp = fu1.get();
		#ifdef DEBUG_FLAG
			cout<<"ACCEPT phase completed"<<endl;
			cout<<"---------------------------------"<<endl;
		#endif

		temp_cmd = get_highest_accepted_proposal (vec_c_resp, 0);
		if (temp_cmd == NULL) {
			cout<<" Somthing went wrong" <<endl;
		}
		if(temp_cmd->command_id == sent_cmd->command_id) {
			cout<<"Proposal got accpeted" <<endl;
			break;
		} else {
			cout<< "Server list size:" <<serverlist_size <<endl;
			orig_cmd->accepted_proposal->proposal_num = temp_cmd->accepted_proposal->proposal_num;
			cout<<"Proposal got REJECTED, Retrying with this proposal_num:"<< orig_cmd->accepted_proposal->proposal_num<<endl;
			delete_command_t(temp_cmd);
			if(sent_cmd->command_id != orig_cmd->command_id) {
				delete_command_t(sent_cmd);
			}
			temp_cmd = orig_cmd;
			cout<<"Proposal got REJECTED, Retrying with this proposal_num:"<< temp_cmd->accepted_proposal->proposal_num<<endl;
		}

	}
	return temp_cmd;
}
// This function implements multipaxos 
uint32_t insert_command(const KeyStoreRequest  *req) {
	cout<<"Inside insert command" <<endl;
	command_t *orig_cmd = get_new_alloc_command(req->clientid(), 0);

	fill_command_from_request_t(req, &orig_cmd);
	uint32_t curr_idx;

	command_t *temp_cmd = orig_cmd; // Intially temperory command is equal to original command
	commands_list_t *cl =  NULL;
	command_t *local_cmd = NULL;

	//cout<<"Before lock log_map_mutex" <<endl;
	log_map_mutex.lock();
	cout<<"After lock log_map_mutex" <<endl;
	if (log_map.find(string(orig_cmd->key)) != log_map.end()){
		cl =  log_map.find(string(orig_cmd->key))->second;
		curr_idx = *(cl->next_available_slot.begin());
	} else {
		cl = new commands_list_t;
		cl->cmd_vec.reserve(INIT_COMMAND_LENGTH); 
		for (int i = 0; i < INIT_COMMAND_LENGTH; i++ ) {
			command_t *cmd = new command_t;
			cmd->accepted_proposal =  new proposal_t;
			cmd->min_proposal_num =  new proposal_t;
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
			cl->cmd_vec.push_back (cmd) ;
			cl->next_available_slot.insert(i);
		}
		cl->last_touched_index = -1;
		curr_idx =  *(cl->next_available_slot.begin());
		
		//cl->cmd_vec[curr_idx] = cmd;
		
	}
	#ifdef DEBUG_FLAG
		cout<<"First empty index :" << curr_idx<<endl;
	#endif
	log_map[string(orig_cmd->key)] = cl;
	log_map_mutex.unlock();
	orig_cmd->index = curr_idx;
	//cout<<"after unlock log_map_mutex" <<endl;

	//temp_cmd->command_id =  cmd->command_id;
	// Multi Paxos
	while (1) {
		 temp_cmd = propose (orig_cmd);

		log_map_mutex.lock();
		if( orig_cmd->command_id == temp_cmd->command_id ) {
			bool update = false;
			#ifdef DEBUG_FLAG
				cout<<" MP: value is been accepted by other nodes  at index: " << curr_idx<<endl;
			#endif
			local_cmd = cl->cmd_vec[curr_idx];
			if (temp_cmd->accepted_proposal->proposal_num == local_cmd->min_proposal_num->proposal_num && 
				temp_cmd->accepted_proposal->proposal_client_id == local_cmd->min_proposal_num->proposal_client_id) {
				update = true;
			}
			delete_command_t(temp_cmd);
			if(update) {
				cout<<"Finally updating local copy"<<endl;
				cout<< "local value: min_client_id : "<< local_cmd->min_proposal_num->proposal_client_id<<" proposal num: "<< local_cmd->min_proposal_num->proposal_num<<endl;
				
				orig_cmd->min_proposal_num->proposal_client_id = local_cmd->min_proposal_num->proposal_client_id; //temp_cmd to use but it is deleted
				orig_cmd->min_proposal_num->proposal_num = local_cmd->min_proposal_num->proposal_num;
				delete_command_t(cl->cmd_vec[curr_idx]);
				cl->cmd_vec[curr_idx] = orig_cmd;
				cl->next_available_slot.erase(curr_idx);
				cl->last_touched_index = curr_idx;
				log_map_mutex.unlock();
				break;
			} else {
				cout<<"In mean time, We have seen bigger proposal number so dont overwrite,"<<endl;
				orig_cmd->accepted_proposal->proposal_num = local_cmd->min_proposal_num->proposal_num + 1;
				temp_cmd = orig_cmd;
			}
			
		} else {
			#ifdef DEBUG_FLAG
				cout<<" MP: value is defeated by other nodes at index: "<< curr_idx<<endl;
			#endif
			delete_command_t(cl->cmd_vec[curr_idx]);
			cl->cmd_vec[curr_idx] = temp_cmd;
			cl->last_touched_index = curr_idx;
			cl->next_available_slot.erase(curr_idx);
			curr_idx = *(cl->next_available_slot.begin());
			orig_cmd->index = curr_idx;
			orig_cmd->accepted_proposal->proposal_num = 0; // Clear previous index proposals
			orig_cmd->min_proposal_num->proposal_num = 0;	
		}
		log_map_mutex.unlock();
	}
	return curr_idx;
}

void RunServer(string server_address) {

	//std::string server_address("0.0.0.0:50051");
	key_store_service_impl service;

	ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
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

void cm_sending_thread () {

	cout<<"Starting CM Message sending thread"<<endl;
	
	while(1) {

		int count = 2; //Send these many message in single shot
		int message_sent = 0;

		
		while (message_sent < count && !cm_message_outqueue.empty()) {
			promise<string> pm =  promise<string>();
    		future <string> fu = pm.get_future();
    		
    		cm_outque_mutex.lock();
			cm_message_request_t *cm_req =  cm_message_outqueue.front();
			cm_outque_mutex.unlock();
			
			send_message_to_all_cm_server(ref(pm), cm_req);
			string result =  fu.get();
			#ifdef DEBUG_FLAG
			cout<<"Got result from send_message_to_all_cm_server :" << result;
			#endif 
			cm_outque_mutex.lock();
			cm_message_outqueue.pop();
			cm_outque_mutex.unlock();
			delete_cm_request_t(cm_req);
			
			message_sent++;
		}
		std::this_thread::sleep_for (std::chrono::milliseconds(10));
	}
}

void print_priority_queue() {
	cm_pq_mutex.lock();
	priority_queue <cm_message_request_t*, vector<cm_message_request_t*>, vectclk_comparator > temp = cm_message_pq;
    cout<<"Printing priority_queue";
    while (!temp.empty()) {
        cm_message_request_t *req = temp.top();
        for (uint32_t i=0; i< req->vecclk_sz; i++) {
			cout<<req->vec_clk[i]<<" ";
		}
        temp.pop();
        cout<<endl;
    }
	cm_pq_mutex.unlock();

}

void cm_internal_processing_thread (){

	#ifdef DEBUG_FLAG
	cout<<"cm_internal_processing_thread started"<<endl;
	int count = 0;
	#endif
	
	while (1) {
		if(!cm_message_pq.empty()) {
			#ifdef DEBUG_FLAGD
				print_priority_queue();
			#endif
			
			cm_pq_mutex.lock();
			cm_message_request_t *req = cm_message_pq.top();
			cm_pq_mutex.unlock();
			bool all_but_j = true;
			#ifdef DEBUG_FLAG
				cout<<"Front of the priority queue"<<endl;
				print_vec_clk(req->vec_clk, serverlist_size);
				cout<<endl;

				cout<<"My current vector clock" <<endl;
				print_vec_clk(t, serverlist_size);
				cout<<endl;
			#endif 
			for (uint32_t i = 0; i < req->vecclk_sz; i++) {
				if (i != req->nodenum && req->vec_clk[i] > t[i]){
					all_but_j =  false;
					#ifdef DEBUG_FLAG
						cout<<"Front is not yet ready"<<endl;
					#endif
				}
			}
			if(all_but_j && req->vec_clk[req->nodenum] == (t[req->nodenum] +1)){
				string key = string(req->key);
				value_t *temp_value_t;
				cm_ks_map_mutex.lock();
				if(cm_ks_map.find(key) != cm_ks_map.end()){
					temp_value_t = cm_ks_map[key];
					delete temp_value_t->key;
					delete temp_value_t->value;
				} else {
					temp_value_t = new value_t;
				}

				temp_value_t->key = new char[req->key_sz+1];
				memset(temp_value_t->key, 0, req->key_sz+1);
				temp_value_t->key_sz = req->key_sz;
				memcpy(temp_value_t->key, req->key, req->key_sz);

				temp_value_t->value = new char[req->value_sz+1];
				memset(temp_value_t->value, 0, req->value_sz+1);
				temp_value_t->value_sz = req->value_sz;
				memcpy(temp_value_t->value, req->value, req->value_sz);
				#ifdef DEBUG_FLAG
					cout<<"New value became"<<temp_value_t->value<<endl;
				#endif
				cm_ks_map[key] = temp_value_t; 

				cm_ks_map_mutex.unlock();

				cm_pq_mutex.lock();
				cm_message_pq.pop();
				#ifdef DEBUG_FLAG
					cout<<"Processed one elemeted from the queue, remaining:"<< cm_message_pq.size()<<endl;
				#endif
				cm_pq_mutex.unlock();

				t[req->nodenum] = req->vec_clk[req->nodenum];

				delete_cm_request_t(req); //Free memory for message
			}
		} 
		#ifdef DEBUG_FLAG
		count++;
		//if(cm_message_pq.empty()) {
		if(count >1000) {
			cout<<"Size of cm_ks_map"<<cm_ks_map.size()<<endl;
			 for(auto it  =cm_ks_map.begin(); it!= cm_ks_map.end();it++){
			// 	cout<<"Key:"<<it->first<<" Size:"<<it->first.length()<<endl;
			 	cout<<"Key_in_value_t:"<<(it->second)->key<<" Size:"<<(it->second)->key_sz<<endl;
			 	cout<<"Key_in_value_t:"<<(it->second)->value<<" Size:"<<(it->second)->value_sz<<endl;
			 }
			cout<<"My vectore clock: ";
			for(auto it  =0; it!= serverlist_size ;it++){
				cout<<t[it]<<" ";
			}
			cout<<endl;
			cout<<"\n Done processing" << endl;
			count = 0;
		}
		#endif
		    
			std::this_thread::sleep_for (std::chrono::milliseconds(20));
		
	}
}

int main(int argc, char** argv) {
	
	string server_address;
	int mylocation = 0;
	if ( argc < 2){
		cout << "Please pass 4 arguments for CM and 3 for ABD, \n For ABD: portnumber, Protocol(ABD/CM) \n For CM: portnumber, Protocol(ABD/CM) mylocation(location of this server ip in server_info.txt, starting from 0)"<<endl;
		return -1;
	}

		
	if(std::string(argv[2]) == "ABD")
	{
		if ( argc !=3){
		cout << "Please pass 3 arguments for ABD, \n portnumber, Protocol(ABD/CM/MP)"<<endl;
		return -1;
		}
		
		value_t *temp_value_t;
		server_address =  get_ipaddr() + string(argv[1]);

		/* Insert default stringin mapt*/
		temp_value_t = new value_t;
		temp_value_t->tag.integer = 0;
		temp_value_t->tag.client_id = 0;
		temp_value_t->key = new char[sizeof("00000")+1];
		temp_value_t->key_sz = sizeof("00000");
		memcpy(temp_value_t->key, "00000", sizeof("00000"));
		temp_value_t->key_sz = 0;
		abd_ks_map["00000"] = temp_value_t;

	} else if(std::string(argv[2]) == "CM"){
		if ( argc !=4){
		cout << "Please pass 4 arguments for CM, \n portnumber, Protocol(ABD/CM/MP) mylocation(location of this server ip in server_info.txt, starting from 0)"<<endl;
		return -1;
		}
		server_address =  get_ipaddr() + string(argv[1]);
		mylocation = stoi(string(argv[3]));
		#ifdef DEBUG_FLAG
		cout<<"mylocation:" <<mylocation<<endl;
		#endif
		//Reading file
		std::ifstream infile("server_info.txt");
		std::vector<string> server_list;
		string s1, s2;
		while (infile >> s1 >> s2)
		{
			#ifdef DEBUG_FLAG
				cout<< s1+":"+s2<<endl;
			#endif
			server_list.push_back(s1+":"+s2);
		}
		cm_connection_obj = new cm_server_connections(server_list, mylocation);
		t =  new int[server_list.size()];
		memset(t, 0, server_list.size()*INT_SIZE);
		serverlist_size = server_list.size();
		mynodenumber = mylocation;

		thread t1(cm_sending_thread);
		thread t2(RunCMServer, server_list[mylocation]);
		thread t3(cm_internal_processing_thread);

		t1.detach();
		t2.detach();
		t3.detach();

	} else {
		cout<< "Starting MP server" <<endl;
		if ( argc !=4){
		cout << "Please pass 4 arguments for MP, \n portnumber, Protocol(ABD/CM/MP) mylocation(location of this server ip in server_info.txt, starting from 0)"<<endl;
		return -1;
		}
		server_address =  get_ipaddr() + string(argv[1]);
		mylocation = stoi(string(argv[3]));
		#ifdef DEBUG_FLAG
		cout<<"mylocation:" <<mylocation<<endl;
		#endif
		//Reading file
		std::ifstream infile("server_info.txt");
		std::vector<string> server_list;
		string s1, s2;
		while (infile >> s1 >> s2)
		{
			#ifdef DEBUG_FLAG
				cout<< s1+":"+s2<<endl;
			#endif
			server_list.push_back(s1+":"+s2);
		}
		mp_connection_obj = new mp_server_connections(server_list, mylocation);
		t =  new int[server_list.size()];
		memset(t, 0, server_list.size()*INT_SIZE);
		serverlist_size = server_list.size();
		mynodenumber = mylocation;
		thread t1(RunMPServer, server_list[mylocation]);
		t1.detach();
	}

	RunServer(server_address);

	return 0;
}