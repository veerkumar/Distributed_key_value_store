
#include "utils.h"

string
get_ipaddr() {
	string ipAddress;
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	// retrieve the current interfaces - returns 0 on success
	success = getifaddrs(&interfaces);
	if (success == 0) {
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while(temp_addr != NULL) {
			if(temp_addr->ifa_addr->sa_family == AF_INET) {
				if(strcmp(temp_addr->ifa_name, INTERFACE)==0){
					ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
#ifdef DEBUG_FLAG				
					cout<<"\n IPaddress"<<ipAddress<<endl;
#endif
				}
			}
			temp_addr = temp_addr->ifa_next;
		}
	}
	// Free memory
	freeifaddrs(interfaces);
	return ipAddress+":";
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

string
getstring_grpc_request_type(KeyStoreRequest::RequestType type) {
	switch (type) {
		case KeyStoreRequest::READ:
			return "READ"; 
		case KeyStoreRequest::READ_QUERY:
			return "READ_QUERY"; break;
		case KeyStoreRequest::WRITE:
			return "WRITE"; break;
		case KeyStoreRequest::WRITE_QUERY:
			return "WRITE_QUERY"; break;
		default:
			return "Unknown";	
	}
	return "Unknown";
}

string
getstring_c_request_type(request_type type) {
	switch (type) {
		case READ:
			return "READ"; break;
		case READ_QUERY:
			return "READ_QUERY"; break;
		case WRITE:
			 return"WRITE"; break;
		case WRITE_QUERY:
			return "WRITE_QUERY"; break;
		default:
			return "Unknown";	
	}
	return "Unknown";
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

string
get_c_return_code_string(KeyStoreResponse::ReturnCode type) {
	switch (type) {
		case KeyStoreResponse::ACK:
			return "ACK";
		case KeyStoreResponse::ERROR:
			return "ERROR";
		case KeyStoreResponse::OK:
			return "OK";
		default:
			cout<<"get_ctype: wrong return code";	
	}
	return "OK";
}

void
print_grpc_return_code(KeyStoreResponse::ReturnCode type) {
	switch (type) {
		case KeyStoreResponse::ACK:
			cout<< "ACK"; break;
		case KeyStoreResponse::ERROR:
			cout<< "ERROR"; break;
		case KeyStoreResponse::OK:
			cout<< "OK"; break;
		default:
			cout<<"Unknown return code";	
	}
}



