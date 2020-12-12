//
// Created for CSE513 course, Project 1
// EECS Department
// Pennsylvania State University
//

/* This program will create 3 clients and do a read and a write operation with each of them cuncurrently.
 * Please set the varaiable servers before use
 */

#include "client.h"
#include <cstdio>
#include <string>
#include <vector>
#include <ctime>
#include <thread>

typedef unsigned int uint;

#define NUMBER_OF_CLIENTS 	3
#define SIZE_OF_VALUE 		3 //bytes

// Define your server information here
static struct Server_info servers[] = {
		{"127.0.0.1", 10000},
		{"127.0.0.1", 10001},
		{"127.0.0.1", 10002}};

static char key[] = "123456"; // We only have one key in this userprogram

namespace Thread_helper{
	void _put(const struct Client* c, const char* key, uint32_t key_size, const char* value, uint32_t value_size){
		
		int status = put(c, key, key_size, value, value_size);

		if(status == 0){ // Success
			return;
		}
		else{
			exit(-1);
		}

		return;
	}

	void _get(const struct Client* c, const char* key, uint32_t key_size, char** value, uint32_t *value_size){

		int status = get(c, key, key_size, value, value_size);

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

	if(argc != 1){
		fprintf(stderr, "%s%s%s\n", "Error\n"
		"Usage: ", argv[0], "\n\n"
		"Please note to run the servers first\n"
		"This application is just for your testing purposes, "
		"and the evaluation of your code will be done with another initiation of your Client libraries.");
		return -1;
	}

	// Running mp Client (Multi-Paxos)
	// Create clients
	struct Client* mp_clt[NUMBER_OF_CLIENTS];
	for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
		mp_clt[i] = client_instance(i, "MP", servers, sizeof(servers) / sizeof(struct Server_info));
		if(mp_clt[i] == NULL){
			fprintf(stderr, "%s\n", "Error occured in creating clients");
			return -1;
		}
	}

	// Do write operations concurrently
	std::vector<std::thread*> threads;
	srand(time(0));
	for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
		
		// build a random value
		
		char value[SIZE_OF_VALUE]={0};
		for(int i = 0; i < SIZE_OF_VALUE; i++){
			value[i] = '0' + rand() % 10 ;
		}

		// run the thread
		threads.push_back(new std::thread(Thread_helper::_put, mp_clt[i], key, sizeof(key), value, sizeof(value)));
    }
    // Wait for all threads to join
    for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
    	threads[i]->join();
    }

    // Do get operations concurrently
	threads.clear();
	char* values[NUMBER_OF_CLIENTS];
	uint32_t value_sizes[NUMBER_OF_CLIENTS];
	for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
		
		// run the thread
		threads.push_back(new std::thread(Thread_helper::_get, mp_clt[i], key, sizeof(key), &values[i], &value_sizes[i]));
    }
    // Wait for all threads to join
    for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
    	threads[i]->join();
    }
    // remmeber after using values, delete them to avoid memory leak

	// Clean up allocated memory in struct Client
	for(uint i = 0; i < NUMBER_OF_CLIENTS; i++){
		if(client_delete(mp_clt[i]) == -1){
			fprintf(stderr, "%s\n", "Error occured in deleting clients");
			return -1;
		}
	}

	return 0;
}