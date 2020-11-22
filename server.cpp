#include "commons.h"
#include "cm.h"
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


int get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<int> distribution(0, INT_MAX);
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
			
		} else {
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
		} // end of ABD 
		
		return Status::OK;
	} // End of service implementation
};

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
		cout << "Please pass 3 arguments for ABD, \n portnumber, Protocol(ABD/CM)"<<endl;
		return -1;
		}
		
		value_t *temp_value_t;
		server_address =  "127.0.0.1:" + string(argv[1]);

		/* Insert default stringin mapt*/
		temp_value_t = new value_t;
		temp_value_t->tag.integer = 0;
		temp_value_t->tag.client_id = 0;
		temp_value_t->key = new char[sizeof("00000")+1];
		temp_value_t->key_sz = sizeof("00000");
		memcpy(temp_value_t->key, "00000", sizeof("00000"));
		temp_value_t->key_sz = 0;
		abd_ks_map["00000"] = temp_value_t;

	} else {
		if ( argc !=4){
		cout << "Please pass 4 arguments for CM, \n portnumber, Protocol(ABD/CM) mylocation(location of this server ip in server_info.txt, starting from 0)"<<endl;
		return -1;
		}
		server_address =  "127.0.0.1:" + string(argv[1]);
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

	}

	RunServer(server_address);

	return 0;
}