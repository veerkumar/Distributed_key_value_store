#include "cm.h"
#include "utils.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

void 
print_vec_clk(int *vecclk, uint32_t size){
	//int *int_vecclk = new int[size/4];
	//memcpy(int_vecclk, vecclk, size);
	cout<<"Vector clock: ";
	for (uint32_t i=0; i< size; i++) {
		cout<<vecclk[i]<<" ";
	}
	cout<<"\n";
	//delete int_vecclk;
}

void 
print_grpc_vec_clk(const char *vecclk, int size){
	int *int_vecclk = new int[size/4];
	memcpy(int_vecclk, vecclk, size);
	cout<<" GRPC Vector clock: ";
	for (int i=0; i< int(size/INT_SIZE); i++) {
		cout<<int_vecclk[i]<<" ";
	}
	cout<<"\n";
	delete int_vecclk;
}

bool 
vectclk_comparator::operator() (cm_message_request_t const *m1, cm_message_request_t const *m2) {
	 	bool m1_strictly_smaller = true;
	 	uint32_t i = 0,count=0;

	 	// we have to store them in order of smaller time ahead;
	 	for(i = 0; i < m1->vecclk_sz; i++) {
			if(m1->vec_clk[i] >= m2->vec_clk[i]){
				m1_strictly_smaller = false;
				count++;
			}
	 	}
	 	/*the logic two compare, 
	 		false means the current position is okay (that is no swap required). 
	 		true means swap required */

	 	if(m1_strictly_smaller) {
          
	 		return false;
	 	} else {
	 		// it means m1 is not strickely smaller
	 		if(count == m1->vecclk_sz) {
	 			// it means m2 is stricly smaller, hence swap;
              	
	 			return true;
	 		} else {
              
	 			return false; //mixed case
	 		}
	 	}
	 	return false; //mixed case
	 }

void
delete_cm_request_t(cm_message_request_t* cm_req){
	if(cm_req->key) delete cm_req->key;
	if(cm_req->value) delete cm_req->value;
	if(cm_req->vec_clk) delete cm_req->vec_clk;
	delete cm_req;
}

cm_message_request_t*
make_cm_request_t(const CMRequest* request) {
	cm_message_request_t *cm_req =  new cm_message_request_t;
	cm_req->nodenum = request->nodenum();

	cm_req->key = new char[request->keysz()+1];
	memset(cm_req->key,0,request->keysz()+1);
	cm_req->value = new char[request->valuesz()+1];
	memset(cm_req->value,0,request->valuesz()+1);
	cm_req->vec_clk = new int[request->vecclksz()/INT_SIZE];
	memset(cm_req->vec_clk,0,request->vecclksz());

	cm_req->key_sz = request->keysz();
	cm_req->value_sz = request->valuesz();
	cm_req->vecclk_sz = request->vecclksz()/INT_SIZE;

	memcpy(cm_req->key, request->key().c_str(),request->keysz());
	memcpy(cm_req->value, request->value().c_str(),request->valuesz());
	memcpy(cm_req->vec_clk, request->vecclk().c_str(), request->vecclksz());

	return cm_req;

}


Status cm_service_impl::CMRequestHandler (ServerContext* context, const  CMRequest* request,
		CMResponse* reply)  {
	cm_message_request_t *cm_req;

#ifdef DEBUG_FLAG
	std::cout << "\n \n CM_service_impl: Got the New message"<<endl;
	cout <<"			Node Number:"<<request->nodenum()<<endl;
	
	cout <<"			Key       :"<<request->key()<< " request->key().size()"<< request->key().size()<<endl;
	cout <<"			keysz 	  :"<<request->keysz()<<endl;
	if(request->valuesz())
		cout <<"			Value     :"<<request->value()<<endl;
	cout <<"			valuesz    :"<<request->valuesz()<<endl;
	if(request->vecclksz()) {
		cout<<"			";
		print_grpc_vec_clk(request->vecclk().c_str(),request->vecclksz());
		cout<<"			vector size(int):"<<request->vecclksz()/INT_SIZE<<endl;
	}
#endif
	
	cm_req = make_cm_request_t(request);
	cm_pq_mutex.lock();
	cm_message_pq.push(cm_req);
	cm_pq_mutex.unlock();

	reply->set_code(CMResponse::ACK);

	return Status::OK;
} // End of service implementation


void RunCMServer(string server_address) {

	//std::string server_address("0.0.0.0:50051");
	cm_service_impl service;

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