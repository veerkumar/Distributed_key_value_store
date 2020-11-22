
/* This program will create 3 clients and do a read and a write operation with each of them cuncurrently.
 * Please set the varaiable servers before use
 */

#include "client.h"
#include <cstdio>
#include <string>
#include <vector>
#include <ctime>
#include <thread>
#include <iostream>
using namespace std;
typedef unsigned int uint;

//#define NUMBER_OF_CLIENTS 	1
#define SIZE_OF_VALUE 		3

// Define your server information here
static struct Server_info servers[] = {
		{"127.0.0.1", 10000},{"127.0.0.1", 10001}, {"127.0.0.1", 10002}, {"127.0.0.1", 10003}, {"127.0.0.1", 10004}};


// static struct Server_info servers[] = {
// 		{"35.192.70.40", 10000}};


// static struct Server_info servers[] = {
// 		{"34.122.77.87", 10000},{"34.122.77.87", 10001}, {"34.122.77.87", 10002}};



static char key[] = "123456"; // We only have one key in this userprogram

namespace Thread_helper{
	void _put(const struct Client* c, const char* key, uint32_t key_size, const char* value, uint32_t value_size, uint32_t *latency){

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		int status = put(c, key, key_size, value, value_size);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		*latency = std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count();
		if(status == 0){ // Success
			return;
		}
		else{
			exit(-1);
		}

		return;
	}

	void _get(const struct Client* c, const char* key, uint32_t key_size, char** value, uint32_t *value_size, uint32_t *latency){

		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		int status = get(c, key, key_size, value, value_size);
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		*latency = std::chrono::duration_cast<std::chrono::microseconds> (end - begin).count();
		if(status == 0){ // Success
			return;
		}
		else{
			exit(-1);
		}

		return;
	}
}

int main(int argc, char* argv[]){

	if(argc != 2){
		fprintf(stderr, "%s%s%s\n", "Error\n"
		"Usage: ", argv[0], "[ABD/CM]\n\n"
		"Please note to run the servers first\n"
		"This application is just for your testing purposes, "
		"and the evaluation of your code will be done with another initiation of your Client libraries.");
		return -1;
	}

	if(std::string(argv[1]) == "ABD"){

		// Create ABD clients
		

		struct Client* abd_clt[NUMBER_OF_CLIENTS];
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			
			abd_clt[i] = client_instance(i, "ABD", servers, sizeof(servers) / sizeof(struct Server_info));
			if(abd_clt[i] == NULL){
				fprintf(stderr, "%s\n", "Error occured in creating clients");
				return -1;
			}
		}

		// Do write operations concurrently
		std::vector<std::thread*> threads;
		
		srand(time(0));
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			
			// build a random value
			
			char value[SIZE_OF_VALUE];
			for(int i = 0; i < SIZE_OF_VALUE; i++){
				value[i] = '0' + rand() % 10;
			}
			// run the thread
			cout<<"Client number:"<<i<< " will write value:" <<value<<endl;
			threads.push_back(new std::thread(Thread_helper::_put, abd_clt[i], key, sizeof(key), value, sizeof(value)));
			std::this_thread::sleep_for (std::chrono::milliseconds(30));
	    }
	    // Wait for all threads to join
	    for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
	    	threads[i]->join();
	    }
		
		// Do get operations concurrently
		threads.clear();
		//cout<<"ABD Userprogram: Reading the value" <<endl ;
		std::this_thread::sleep_for (std::chrono::milliseconds(1000));
		char* values[NUMBER_OF_CLIENTS];
		uint32_t value_sizes[NUMBER_OF_CLIENTS];
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			
			// run the thread
			threads.push_back(new std::thread(Thread_helper::_get, abd_clt[i], key, sizeof(key), &values[i], &value_sizes[i]));
	    	std::this_thread::sleep_for (std::chrono::milliseconds(50));
	    }
	    // Wait for all threads to join
	    for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
	    	threads[i]->join();
	    }
	    // remmeber after using values, delete them to avoid memory leak

		// Clean up allocated memory in struct Client
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			if(client_delete(abd_clt[i]) == -1){
				fprintf(stderr, "%s\n", "Error occured in deleting clients");
				return -1;
			}
		} 
	}
	else if(std::string(argv[1]) == "CM"){
		
		// Create CM clients
		struct Client* cm_clt[NUMBER_OF_CLIENTS];
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			cm_clt[i] = client_instance(i, "CM", servers, sizeof(servers) / sizeof(struct Server_info));
			if(cm_clt[i] == NULL){
				fprintf(stderr, "%s\n", "Error occured in creating clients");
				return -1;
			}
		}

		// Do write operations concurrently
		std::vector<std::thread*> threads;
		srand(time(0));
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			
			// build a random value
			
			char value[SIZE_OF_VALUE];
			for(int i = 0; i < SIZE_OF_VALUE; i++){
				value[i] = '0' + rand() % 10;
				
			}
			cout<<"CLient "<<i<<" will write:"<<value<<endl;
			// run the thread
			threads.push_back(new std::thread(Thread_helper::_put, cm_clt[i], key, sizeof(key), value, sizeof(value)));
			std::this_thread::sleep_for (std::chrono::milliseconds(50));
	    }
	    // Wait for all threads to join
	    for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
	    	threads[i]->join();
	    }
		
		// Do get operations concurrently
		threads.clear();
		cout<<"Sleeping for 10 sec"<<endl;
		std::this_thread::sleep_for (std::chrono::milliseconds(5000));
		cout<<"after Sleeping  10 sec"<<endl;
		char* values[NUMBER_OF_CLIENTS];
		uint32_t value_sizes[NUMBER_OF_CLIENTS];
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			// run the thread
			threads.push_back(new std::thread(Thread_helper::_get, cm_clt[i], key, sizeof(key), &values[i], &value_sizes[i]));
	    }
	    // Wait for all threads to join
	    for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
	    	threads[i]->join();
	    }
	    for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
	    	cout<< "Userprogram:   Client number: "<<i<<" Value: "<< values[i] <<"Size: "<< value_sizes[i]<<endl;
	    }
	    cout<<"******************************"<<endl;
	    // remmeber after using values, delete them to avoid memory leak

		// Clean up allocated memory in struct Client
		for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
			if(client_delete(cm_clt[i]) == -1){
				fprintf(stderr, "%s\n", "Error occured in deleting clients");
				return -1;
			}
		}
	}
	else{
		fprintf(stderr, "%s%s%s\n", "Error\n"
		"Usage: ", argv[0], "[ABD/CM]\n\n"
		"Please note to run the servers first\n"
		"This application is just for your testing purposes, "
		"and the evaluation of your code will be done with another initiation of your Client libraries.");
		return -1;
	}

	return 0;
}