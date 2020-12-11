
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
					cout<<"\n IPaddress"<<ipAddress;
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

mp_request_type
get_c_mp_req_type(PaxosRequest::RequestType type) {
	switch (type) {
		case PaxosRequest::PREPARE:
			return PREPARE;
		case PaxosRequest::ACCEPT:
			return ACCEPT;
		default:
			cout<<"get_ctype: wrong return code";	
	}
	return ACCEPT;
}

PaxosResponse::ReturnCode
get_grpc_return_code(return_code type) {
	switch (type) {
		case ACK:
			return PaxosResponse::ACK;
		case ERROR:
			return PaxosResponse::ERROR;
		case OK:
			return PaxosResponse::OK;
		case NACK:
			return PaxosResponse::NACK;
		default:
			cout<<"get_ctype: wrong return code";	
	}
	return PaxosResponse::OK;
}

PaxosRequest::CommandType
get_grpc_req_command_type(request_type type) {
	switch (type) {
		case READ:
			return PaxosRequest::READ;
		case WRITE:
			return PaxosRequest::WRITE;
		default:
			cout<<"get_ctype: wrong command type ";	
	}
	return PaxosRequest::READ;
}

PaxosRequest::RequestType
get_grpc_req_type(mp_request_type type) {
	switch (type) {
		case PREPARE:
			return PaxosRequest::PREPARE;
		case ACCEPT:
			return PaxosRequest::ACCEPT;
		default:
			cout<<"get_ctype: wrong command type ";	
	}
	return PaxosRequest::PREPARE;
}

PaxosResponse::CommandType
get_grpc_resp_command_type(request_type type) {
	switch (type) {
		case READ:
			return PaxosResponse::READ;
		case WRITE:
			return PaxosResponse::WRITE;
		default:
			cout<<"get_ctype: wrong command type";	
	}
	return PaxosResponse::READ;
}

void print_mp_req_type(mp_req_type type) {
	switch (type) {
		case PREPARE:
			cout<< "PREPARE"<<endl; break;
		case ACCEPT:
			cout<< "ACCEPT"<<endl; break;
		default:
			cout<<"get_ctype: wrong command type ";	
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
	return "OK";
}
void 
print_command_t(command_t *req, bool request) {
	cout<< "Printing MP request" <<endl;
	if(request)
	cout<<" mp_request_type  :" << print_mp_req_type(req->mp_req_type);
	if(!request)
	cout<<" Retun_code 		 :" << print_return_code(req->return_code);
	cout<<" command_type  	 :" << print_command_type(req->command_type);
	cout<<" min_proposal_num :" <<endl;
	cout<<"	 	client_id 	 :"<<req->min_proposal_num.proposal_client_id<<endl;
	cout<<"	 	proposal_num :"<<req->min_proposal_num.proposal_client_id<<endl;
	cout<<" accepted_proposal:" <<endl;
	cout<<"	 	client_id 	 :"<<req->accepted_proposal.proposal_client_id<<endl;
	cout<<"	 	proposal_num :"<<req->accepted_proposal.proposal_client_id<<endl;
	cout<<" command_id 		 :"<<req->command_id;
	if(req->key_sz)
	cout<<" key      		 :"<<req->key<<endl;
	cout<<" key_sz   		 :"<<req->key_sz<<endl;
	if(req->value_sz)
	cout<<" value   		 :"<<req->value<<endl;
	cout<<" value_sz		 :"<<req->value_sz<<endl;
	
}
