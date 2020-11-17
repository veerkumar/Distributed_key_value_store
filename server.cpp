#include "commons.h"
#include "server.h"
#include "utils.h"


int get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<int> distribution(0, INT_MAX);
	return distribution(generator);
}


class key_store_service_impl: public KeyStoreService::Service {
	Status KeyStoreRequestHandler (ServerContext* context, const  KeyStoreRequest* request,
			KeyStoreResponse* reply) override {
#ifdef DEBUG_FLAG
		std::cout << "\n \n Got the New message "<<endl;

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

#ifdef DEBUG_FLAG
			std::cout << "	Got CM protocol "<<endl;
#endif
			reply->set_code(KeyStoreResponse::ACK);
			reply->set_protocol(KeyStoreResponse::CM);
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
				if(request->key() != "00000" && (abd_ks_map.find(string(request->key())) != abd_ks_map.end())) {
					/* Found the key */
					abd_ks_map_mutex.lock();
					value_t *temp_value_t = abd_ks_map[string(request->key())];
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
				cout<<"	Locked in WRITE_QUERY\n";
				if(abd_ks_map.find(string(request->key())) != abd_ks_map.end()){
					 temp_value_t = abd_ks_map[string(request->key())];	
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
				cout<<"	Unlocked WRITE_QUERY\n";

			}
			if (request->type() ==  KeyStoreRequest::WRITE) {
#ifdef DEBUG_FLAG
			std::cout << "	Got request type: WRITE" <<endl;
#endif
				value_t *temp_value_t;
				abd_ks_map_mutex.lock();
				cout<<"	WRITE Locked\n";
				if(abd_ks_map.find(string(request->key())) != abd_ks_map.end()){
					 temp_value_t = abd_ks_map[string(request->key())];
					 if(temp_value_t->tag.integer < request->integer()) {
					#ifdef DEBUG_FLAG
						cout<< "Writing value with new value" <<endl;
					#endif
						temp_value_t->tag.integer = request->integer();
						temp_value_t->tag.client_id = request->clientid();

						//value can change so first delete and the allocate new
						delete temp_value_t->value;
						temp_value_t->value = new char[request->valuesz()];
						temp_value_t->value_sz = request->valuesz();
						memcpy(temp_value_t->value, request->value().c_str(), request->value().size());

						abd_ks_map[string(request->key())] = temp_value_t;
					 } else if (temp_value_t->tag.integer == request->integer()) {

					 	 if(temp_value_t->tag.client_id < request->clientid()) {
					#ifdef DEBUG_FLAG
						cout<< "Writing value from new bigger client id" <<endl;
					#endif
					 	 	temp_value_t->tag.client_id = request->clientid();

						//value can change so first delete and the allocate new
							delete temp_value_t->value;
							temp_value_t->value = new char[request->valuesz()];
							temp_value_t->value_sz = request->valuesz();
							memcpy(temp_value_t->value, request->value().c_str(), request->valuesz());

							abd_ks_map[string(request->key())] = temp_value_t;
					 	 }

					 } else {
					 	cout << "Ignoring this value, as KeyStore has more latest value" << endl;
					 }
				} else {
					#ifdef DEBUG_FLAG
						cout<< "Writing new value" <<endl;
					#endif
					temp_value_t = new value_t;
					temp_value_t->tag.integer = request->integer();
					temp_value_t->tag.client_id = request->clientid();

					temp_value_t->key = new char[request->key().size()];
					temp_value_t->key_sz = request->keysz();
					memcpy(temp_value_t->key, request->key().c_str(), request->key().size());

					temp_value_t->value = new char[request->value().size()];
					temp_value_t->value_sz = request->valuesz();
					memcpy(temp_value_t->value, request->value().c_str(), request->value().size());

					abd_ks_map[string(request->key())] = temp_value_t;
				}


				cout<< "	Size of key_value store : " << abd_ks_map.size();
				abd_ks_map_mutex.unlock();
				cout<<"	  WRITE Unlocked\n";
				
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

int main(int argc, char** argv) {
	value_t *temp_value_t;
	if (argc != 4){
		cout << "Please pass 3 arguments, \n serverip portnumber Protocol(ABD/CM)"<<endl;
	}
	string server_address =  string(argv[1]) + ":" + string(argv[2]);

	/* Insert default stringin mapt*/
	temp_value_t = new value_t;
	temp_value_t->tag.integer = 0;
	temp_value_t->tag.client_id = 0;
	temp_value_t->key = new char[sizeof("00000")];
	temp_value_t->key_sz = sizeof("00000");
	memcpy(temp_value_t->key, "00000", sizeof("00000"));
	temp_value_t->key_sz = 0;
	abd_ks_map["00000"] = temp_value_t;

	RunServer(server_address);

	return 0;
}