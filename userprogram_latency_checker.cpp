
/* This program will create 3 clients and do a read and a write operation with each of them cuncurrently.
 * Please set the varaiable servers before use
 */

#include "client.h"
#include "commons.h"
#include <cstdio>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <iostream>
using namespace std;
typedef unsigned int uint;

//#define NUMBER_OF_CLIENTS 	1


int num_secs = 10;


// Define your server information here
// static struct Server_info servers[] = {
// 		{"127.0.0.1", 10000},{"127.0.0.1", 10001}, {"127.0.0.1", 10002}, {"127.0.0.1", 10003}, {"127.0.0.1", 10004}};

static struct Server_info servers[] = {
		{"127.0.0.1", 10000},{"127.0.0.1", 10001}, {"127.0.0.1", 10002}};

 // static struct Server_info servers[] = {
 // 		{"34.86.218.217", 10000},{"34.70.0.254", 10001},{"34.76.84.19",10002},{"104.155.208.169",10003},{"35.228.78.135",10004}};

// static struct Server_info servers[] = {
// 		{"34.75.64.24", 10000},{"34.122.77.87", 10001},{"34.76.84.19",10002}};




static char key[] = "123456"; // We only have one key in this userprogram

namespace Thread_helper{
	void _put(const struct Client* c, const char* key, uint32_t key_size, const char* value, uint32_t value_size, uint64_t *latency){

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		int status = put(c, key, key_size, value, value_size);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		*latency = std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count();
		if(status == 0){ // Success
			return;
		}
		else{
			exit(-1);
		}

		return;
	}

	void _get(const struct Client* c, const char* key, uint32_t key_size, char** value, uint32_t *value_size, uint64_t *latency){

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		int status = get(c, key, key_size, value, value_size);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		*latency = std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count();
		if(status == 0){ // Success
			return;
		}
		else{
			exit(-1);
		}

		return;
	}
}

void calculate_read_and_write_latency(char * latency_type, uint64_t *latency_value, string protocol){
	uint64_t write_count = 0;
	uint64_t read_count = 0;
	uint64_t read_sum = 0;
	uint64_t write_sum = 0;
	if(protocol == "ABD") {
		
			for (int i = 0; i< num_secs*ABD_NUMBER_OF_CLIENTS;i++) {
				if(latency_type[i] == 'W') {
					write_count++;
					//cout<<write_count<<" ";
					write_sum += latency_value[i];
				} else {
					read_count++;
					read_sum += latency_value[i];
				}
			}	
	} 
	if(protocol == "CM") {
		
			for (int i = 0; i< CM_NUMBER_OF_CLIENTS;i++) {
				if(latency_type[i] == 'W') {
					write_count++;
					//cout<<write_count<<" ";
					write_sum += latency_value[i];
				} else {
					read_count++;
					read_sum += latency_value[i];
				}
			}
	}
	if(protocol == "MP") {
		for (int i = 0; i< num_secs*MP_NUMBER_OF_CLIENTS;i++) {
				if(latency_type[i] == 'W') {
					write_count++;
					//cout<<write_count<<" ";
					write_sum += latency_value[i];
				} else {
					read_count++;
					read_sum += latency_value[i];
				}
		}		
	} 
	
	cout<<"	read_count	:"<<read_count<<endl;
	//cout<<"read_sum :"<<read_sum<<endl;
	cout<<"	write_count	:"<<write_count<<endl;
	//cout<<"write_sum"<<write_sum<<endl;
	
	cout<<"	Get latency 	:";
	if(read_count==0) {
		cout<<"0ms"<<endl;
	}	else{
		cout<<read_sum/read_count<<"ms"<<endl;
	}
	cout<<"	Put latency 	:";
	if(write_count ==0){
		cout<<"0ms"<<endl;
	} else { 
		cout<< write_sum/write_count<<"ms"<<endl;
	}
	cout<<"\n";
}
// if read_right_ratio is 10% then value is 1
void abd_latency_executer(int read_write_ratio) {
		
		struct Client* abd_clt[ABD_NUMBER_OF_CLIENTS];
		char latency_type[num_secs*ABD_NUMBER_OF_CLIENTS];
		uint64_t latency_value[num_secs*ABD_NUMBER_OF_CLIENTS]={0};
		int counter = 0;
		cout<<"Read_write_ratio: "<< read_write_ratio*10 <<endl;
		for(uint i = 0; i < ABD_NUMBER_OF_CLIENTS; i++){
			abd_clt[i] = client_instance(i, "ABD", servers, sizeof(servers) / sizeof(struct Server_info));
			if(abd_clt[i] == NULL){
				fprintf(stderr, "%s\n", "Error occured in creating clients");
				return;
			}
		}

		std::vector<std::thread*> threads;
		srand(time(0));
		char* values[num_secs*ABD_NUMBER_OF_CLIENTS];
		uint32_t value_sizes[num_secs*ABD_NUMBER_OF_CLIENTS];

		for (int j = 0; j< num_secs;j++) {
			//std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			for(int i = 0; i < ABD_NUMBER_OF_CLIENTS; i++) {
				
				// build a random value
				int prob = rand() % 10;
				
				if (prob >= read_write_ratio) {
					latency_type[counter] = 'W';
				
					char value[SIZE_OF_VALUE] = {0};
					for(int i = 0; i < SIZE_OF_VALUE; i++){
						value[i] = '0' + rand() % 10;
					}
					// run the thread
					//cout<<"Client number:"<<i<< "will write value:" <<value<<endl;
					threads.push_back(new std::thread(Thread_helper::_put, abd_clt[i], key, 
					 	sizeof(key), value, sizeof(value), &latency_value[counter]));
					
				} else {
					latency_type[counter] = 'R';
					 threads.push_back(new std::thread(Thread_helper::_get, abd_clt[i], key, 
					 	sizeof(key), &values[counter], &value_sizes[counter], &latency_value[counter]));
				}
				std::this_thread::sleep_for (std::chrono::milliseconds(98));
				counter++;
				//cout<<counter<<" ";
		    }
		   // std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		   // cout<<"\ntime elapsed"<<std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count();
		}
	    for(int i = 0; i < num_secs*ABD_NUMBER_OF_CLIENTS; i++){
	    	threads[i]->join();
	    }
	    threads.clear();

	    calculate_read_and_write_latency(latency_type, latency_value,"ABD");
	    for(int i = 0; i < ABD_NUMBER_OF_CLIENTS; i++){
			if(client_delete(abd_clt[i]) == -1){
				fprintf(stderr, "%s\n", "Error occured in deleting clients");
				return;
			}
		} 
}

void cm_latency_executer(int read_write_ratio){

		struct Client* cm_clt[CM_NUMBER_OF_CLIENTS];
		char latency_type[CM_NUMBER_OF_CLIENTS];
		uint64_t latency_value[CM_NUMBER_OF_CLIENTS]={0};
		int counter = 0;
		cout<<"Read_write_ratio: "<< read_write_ratio*10 <<endl;
		for(uint i = 0; i < CM_NUMBER_OF_CLIENTS; i++){
			cm_clt[i] = client_instance(i, "CM", servers, sizeof(servers) / sizeof(struct Server_info));
			if(cm_clt[i] == NULL){
				fprintf(stderr, "%s\n", "Error occured in creating clients");
				return;
			}
		}

		std::vector<std::thread*> threads;
		srand(time(0));
		char* values[CM_NUMBER_OF_CLIENTS];
		uint32_t value_sizes[CM_NUMBER_OF_CLIENTS];

		for (int j = 0; j< int(CM_NUMBER_OF_CLIENTS/10);j++) {
			//std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			for(int i = 0; i < 10; i++) {
				
				// build a random value
				int prob = rand() % 10;
				
				if (prob >= read_write_ratio) {
					latency_type[counter] = 'W';
				
					char value[SIZE_OF_VALUE] = {0};
					for(int i = 0; i < SIZE_OF_VALUE; i++){
						value[i] = '0' + rand() % 10;
					}
					// run the thread
					//cout<<"Client number:"<<i<< "will write value:" <<value<<endl;
					threads.push_back(new std::thread(Thread_helper::_put, cm_clt[i], key, 
					 	sizeof(key), value, sizeof(value), &latency_value[counter]));
					
				} else {
					latency_type[counter] = 'R';
					 threads.push_back(new std::thread(Thread_helper::_get, cm_clt[i], key, 
					 	sizeof(key), &values[counter], &value_sizes[counter], &latency_value[counter]));
				}
				std::this_thread::sleep_for (std::chrono::milliseconds(98));
				counter++;
				//cout<<counter<<" ";
		    }
		    //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		    //cout<<"\ntime elapsed"<<std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count();
		}
	    for(int i = 0; i < CM_NUMBER_OF_CLIENTS; i++){
	    	threads[i]->join();
	    }
	    threads.clear();

	    calculate_read_and_write_latency(latency_type, latency_value, "CM");
	    for(int i = 0; i < CM_NUMBER_OF_CLIENTS; i++){
			if(client_delete(cm_clt[i]) == -1){
				fprintf(stderr, "%s\n", "Error occured in deleting clients");
				return;
			}
		}
}

void mp_latency_executer(int read_write_ratio){

		struct Client* mp_clt[MP_NUMBER_OF_CLIENTS];
		char latency_type[num_secs*MP_NUMBER_OF_CLIENTS];
		uint64_t latency_value[num_secs*MP_NUMBER_OF_CLIENTS]={0};
		int counter = 0;
		cout<<"Read_write_ratio: "<< read_write_ratio*10 <<endl;
		for(uint i = 0; i < MP_NUMBER_OF_CLIENTS; i++){
			mp_clt[i] = client_instance(i, "MP", servers, sizeof(servers) / sizeof(struct Server_info));
			if(mp_clt[i] == NULL){
				fprintf(stderr, "%s\n", "Error occured in creating clients");
				return;
			}
		}

		std::vector<std::thread*> threads;
		srand(time(0));
		char* values[num_secs*MP_NUMBER_OF_CLIENTS];
		uint32_t value_sizes[num_secs*MP_NUMBER_OF_CLIENTS];

		for (int j = 0; j< num_secs;j++) {
			//std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
			for(int i = 0; i < MP_NUMBER_OF_CLIENTS; i++) {
				
				// build a random value
				int prob = rand() % 10;
				
				if (prob >= read_write_ratio) {
					latency_type[counter] = 'W';
				
					char value[SIZE_OF_VALUE] = {0};
					for(int i = 0; i < SIZE_OF_VALUE; i++){
						value[i] = '0' + rand() % 10;
					}
					// run the thread
					//cout<<"Client number:"<<i<< "will write value:" <<value<<endl;
					threads.push_back(new std::thread(Thread_helper::_put, mp_clt[i], key, 
					 	sizeof(key), value, sizeof(value), &latency_value[counter]));
					
				} else {
					latency_type[counter] = 'R';
					 threads.push_back(new std::thread(Thread_helper::_get, mp_clt[i], key, 
					 	sizeof(key), &values[counter], &value_sizes[counter], &latency_value[counter]));
				}
				counter++;
		    }
		    std::this_thread::sleep_for (std::chrono::milliseconds(900)); // For 3 req/sec
		    for(int i = 0; i < MP_NUMBER_OF_CLIENTS; i++){
	    			threads[i]->join();
	   			}
	   		threads.clear();
		   // std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		   // cout<<"\ntime elapsed"<<std::chrono::duration_cast<std::chrono::milliseconds> (end - begin).count();
		}
	    

	    calculate_read_and_write_latency(latency_type, latency_value,"MP");
	    for(int i = 0; i < MP_NUMBER_OF_CLIENTS; i++){
			if(client_delete(mp_clt[i]) == -1){
				fprintf(stderr, "%s\n", "Error occured in deleting clients");
				return;
			}
		} 
}

int main(int argc, char* argv[]){

	if(argc != 2){
		fprintf(stderr, "%s%s%s\n", "Error\n"
		"Usage: ", argv[0], "[ABD/CM/MP]\n\n"
		"Please note to run the servers first\n"
		"This application is just for your testing purposes, "
		"and the evaluation of your code will be done with another initiation of your Client libraries.");
		return -1;
	}

	if(std::string(argv[1]) == "ABD"){

		abd_latency_executer(1); //10%

		abd_latency_executer(5); //50%

		abd_latency_executer(9); //90%
		
	}
	else if(std::string(argv[1]) == "CM"){
		
		cm_latency_executer(1); // 10%
		std::this_thread::sleep_for (std::chrono::milliseconds(5000));
		cm_latency_executer(5); // 50%
		std::this_thread::sleep_for (std::chrono::milliseconds(5000));
		cm_latency_executer(9); // 90%
	}
	else if (std::string(argv[1]) == "MP"){ 
	
			mp_latency_executer(1); // 10%
			std::this_thread::sleep_for (std::chrono::milliseconds(5000));
			mp_latency_executer(5); // 50%
			std::this_thread::sleep_for (std::chrono::milliseconds(5000));
			mp_latency_executer(9); // 90%

	} else {

		fprintf(stderr, "%s%s%s\n", "Error\n"
		"Usage: ", argv[0], "[ABD/CM]\n\n"
		"Please note to run the servers first\n"
		"This application is just for your testing purposes, "
		"and the evaluation of your code will be done with another initiation of your Client libraries.");
		return -1;
	}

	return 0;
}