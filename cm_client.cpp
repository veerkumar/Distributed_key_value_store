#include "commons.h"
#include "cm.h"
#include "utils.h"




cm_server_connections::cm_server_connections(vector<string> server_list, int mylocation){
	string key;  
	/* Create connection with all servers */
	for (int i = 0; i < int(server_list.size()); i++) {
		if(i!= mylocation) {
			#ifdef DEBUG_FLAG
				cout<<"\n\n"<<__func__ <<": Creating new connection with file server :"<< key <<endl;
			#endif
			connections[server_list[i]] = new cm_client(grpc::CreateChannel(server_list[i], 
											grpc::InsecureChannelCredentials()));
		} else {
			#ifdef DEBUG_FLAG
				cout<< "Not creating conneciton with self";
			#endif
		}
	}
}

cm_server_connections::~cm_server_connections(){
	/* Delete all the connection */
	map<string, cm_client*>::iterator it = connections.begin();

	while(it!=connections.end()){
		delete it->second;
		it++;
	}
}

void
make_cm_request_payload(CMRequest* payload, cm_message_request_t *req) {
	payload->set_nodenum(req->nodenum);
	payload->set_keysz(req->key_sz);
	if(req->key_sz){
		payload->set_key(req->key,req->key_sz);
	}
	payload->set_valuesz(req->value_sz);
	if(req->value_sz){
		payload->set_value(req->value, req->value_sz);
	}
		
	payload->set_vecclksz(req->vecclk_sz*INT_SIZE);
	if(req->vecclk_sz){
		payload->set_vecclk(req->vec_clk, req->vecclk_sz*INT_SIZE);
	}
		
}
string
get_c_return_code_string(CMResponse::ReturnCode type) {
	switch (type) {
		case CMResponse::ACK:
			return "ACK";
		case CMResponse::ERROR:
			return "ERROR";
		case CMResponse::OK:
			return "OK";
		default:
			cout<<"get_ctype: wrong return code";	
	}
	return "OK";
}
string 
extract_cm_response_from_payload (CMResponse *Response) {
	return get_c_return_code_string(Response->code());
}

void send_to_cm_server_handler(cm_client* connection_stub, CMRequest *req) {

		CMResponse Response;
		ClientContext Context;
		string c_response;

		#ifdef DEBUG_FLAG
			std::cout << "Sending message to server "<<endl;
		#endif

		Status status = connection_stub->stub_->CMRequestHandler(&Context, *req, &Response);
		if (status.ok()) {
				c_response = extract_cm_response_from_payload(&Response);
				#ifdef DEBUG_FLAG
					std::cout << "Got the response from server: "<<c_response <<endl;
					std::cout << "Wrote back"<<endl;
				#endif
			} else {
				std::cout << status.error_code() << ": " << status.error_message()
					<< std::endl;
				return ;
			}
			return;
}
void print_cm_request(cm_message_request_t *req) {
	cout<< "Printing CM request" <<endl;
	cout<<" key      :"<<req->key<<endl;
	cout<<" key_sz   :"<<req->key_sz<<endl;
	if(req->value_sz)
		cout<<" value    :"<<req->value<<endl;
	cout<<" value_sz:"<<req->value_sz<<endl;
	print_vec_clk(req->vec_clk, req->vecclk_sz);
}

void send_message_to_all_cm_server(promise<string>& prom,  cm_message_request_t *c_req) {
	CMRequest ReqPayload;
	uint32_t number_of_servers =  cm_connection_obj->connections.size();
	std::vector<std::thread*> threads;

	make_cm_request_payload(&ReqPayload, c_req);
	#ifdef DEBUG_FLAG
	print_cm_request(c_req);
	#endif
	
	for (auto it = cm_connection_obj->connections.begin(); it!= cm_connection_obj->connections.end(); it++) {
		#ifdef DEBUG_FLAG
			cout<<"CM_client: Sending to server handler" <<endl;
		#endif
		threads.push_back(new std::thread(send_to_cm_server_handler, 
								 it->second, &ReqPayload));
	}
	
	for(uint32_t i = 0; i < number_of_servers; i++){
	    	threads[i]->join();
	}
	#ifdef DEBUG_FLAG
		cout<<" Heard back from all the CM servers " <<endl; 
	#endif
        
    prom.set_value("OK");
	
}