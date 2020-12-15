
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


int num_secs = 2;
mutex file_lock;


// Define your server information here
static struct Server_info servers[] = {
		{"127.0.0.1", 10000},{"127.0.0.1", 10001}, {"127.0.0.1", 10002}};


// static struct Server_info servers[] = {
// 		{"34.75.64.24", 10000},{"34.122.77.87", 10001},{"34.76.84.19",10002},{"104.155.208.169",10003},{"35.228.78.135",10004}};

// static struct Server_info servers[] = {
// 		{"34.75.64.24", 10000},{"34.122.77.87", 10001},{"34.76.84.19",10002}};




static char key[] = "123456"; // We only have one key in this userprogram

namespace Thread_helper{
	void _put(const struct Client* c, const char* key, uint32_t key_size, const char* value, uint32_t value_size, uint64_t *latency){
		ofstream myfile;
		file_lock.lock();
		myfile.open ("mp_file.edn",std::ios_base::app);
		string str = string("{:process ") + to_string(c->id) + string(", :type :invoke, :f :write, :value ") + to_string(atoi(value))+string("}\n");
		myfile<<str;
		myfile.close();
		file_lock.unlock();
		cout<<str<<endl;
		
		int status = put(c, key, key_size, value, value_size);

		file_lock.lock();
		myfile.open ("mp_file.edn",std::ios_base::app);
		string str1 = "{:process " + to_string(c->id) + ", :type :ok, :f :write, :value " + to_string(atoi(value)) +"}\n";
		myfile<<str1;
		myfile.close();
		file_lock.unlock();
		cout<<str1<<endl;
		
		if(status == 0){ // Success

			return;
		}
		else{
			exit(-1);
		}

		return;
	}

	void _get(const struct Client* c, const char* key, uint32_t key_size, char** value, uint32_t *value_size, uint64_t *latency){
		ofstream myfile;
		file_lock.lock();
		myfile.open ("mp_file.edn",std::ios_base::app);
		string str = "{:process " + to_string(c->id) + ", :type :invoke, :f :read, :value nil}\n";
		myfile<<str;
		myfile.close();
		cout<<str<<endl;
		file_lock.unlock();
		
		int status = get(c, key, key_size, value, value_size);
		
		string str1;
		
		if(status == 0){ // Success
			file_lock.lock();
			myfile.open ("mp_file.edn",std::ios_base::app);
			if(*value_size == 0) {
				str1 = "{:process " + to_string(c->id) + ", :type :ok, :f :read, :value nil}\n";
			} else {
				str1 = "{:process " + to_string(c->id) + ", :type :ok, :f :read, :value " + to_string(atoi(*value)) +"}\n";
			}
			myfile<<str1;
			myfile.close();
			file_lock.unlock();
			cout<<str1<<endl;
			return;
		}
		else{
			exit(-1);
		}

		return;
	}
}

// if read_right_ratio is 10% then value is 1
void mp_latency_executer(int read_write_ratio) {
		ofstream myfile;
		myfile.open ("mp_file.edn");
		struct Client* mp_clt[MP_NUMBER_OF_CLIENTS];
		uint64_t latency_value[num_secs*MP_NUMBER_OF_CLIENTS]={0};
		int counter = 0;

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
			
			for(int i = 0; i < MP_NUMBER_OF_CLIENTS; i++) {
				
				// build a random value
				int prob = rand() % 10;
				
				if (prob >= read_write_ratio) {
					
				
					char value[SIZE_OF_VALUE+1] = {0};
					for(int i = 0; i < SIZE_OF_VALUE; i++){
						value[i] = '0' + rand() % 10;
					}
					//remove below line to reproduce false case
					 values[counter] = value;
					 threads.push_back(new std::thread(Thread_helper::_put, mp_clt[i], key, 
					  	sizeof(key), value, sizeof(value), &latency_value[counter]));
					
					
				} else {
					
					
					threads.push_back(new std::thread(Thread_helper::_get, mp_clt[i], key, 
					  	sizeof(key), &values[counter], &value_sizes[counter], &latency_value[counter]));
					
				}
				std::this_thread::sleep_for (std::chrono::milliseconds(10));
				counter++;
		    }
		    for(int i = 0; i < MP_NUMBER_OF_CLIENTS; i++){
	    			threads[i]->join();
	   			}
	   		threads.clear();
		   
		}
	    

	    for(int i = 0; i < MP_NUMBER_OF_CLIENTS; i++){
			if(client_delete(mp_clt[i]) == -1){
				fprintf(stderr, "%s\n", "Error occured in deleting clients");
				return;
			}
		} 
		myfile.close();
}



int main(int argc, char* argv[]){

	if(argc != 2){
		fprintf(stderr, "%s%s%s\n", "Error\n"
		"Usage: ", argv[0], "[MP]\n\n"
		"Please note to run the servers first\n"
		"This application is just for your testing purposes, "
		"and the evaluation of your code will be done with another initiation of your Client libraries.");
		return -1;
	}

	if(std::string(argv[1]) == "MP"){

		//abd_latency_executer(1); //10%

		mp_latency_executer(5); //50%

		//abd_latency_executer(9); //90%
		
	}
	
	else{

		fprintf(stderr, "%s%s%s\n", "Error\n"
		"Usage: ", argv[0], "[MP]\n\n"
		"Please note to run the servers first\n"
		"This application is just for your testing purposes, "
		"and the evaluation of your code will be done with another initiation of your Client libraries.");
		return -1;
	}

	return 0;
}