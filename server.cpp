#include "commons.h"
#include "server.h"



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
		std::cout << "\nGot the message ";

		cout<<"Reqtype\n"<<request->type(); 
		cout <<"Protocol\n"<<request->protocol();
		cout <<"Integer\n"<<request->integer();
		cout <<"Client_id\n"<<request->cliendid();
		cout <<"Key\n"<<request->key();
		cout <<"Value\n"<<request->value();
		cout <<"Client_id\n"<<request->size();
#endif
		if (request->protocol() = KeyStoreRequest::CM) {
#ifdef DEBUG_FLAG
			std::cout << "\nGot CM protocol ";
#endif
			reply->set_code(KeyStoreResponse::ACK)
			replu->set_protocol(KeyStoreResponse:CM);
		} else {
#ifdef DEBUG_FLAG
			std::cout << "\nGot ABD protocol ";
			reply->set_code(KeyStoreResponse::ACK)
			replu->set_protocol(KeyStoreResponse:CM);
#endif

		}
		

	}
}

void RunServer() {

	std::string server_address("0.0.0.0:50051");
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
	m_m = new meta_data_manager;
	RunServer();

	return 0;
}