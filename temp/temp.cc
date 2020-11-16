/******************************************************************************

                              Online C++ Compiler.
               Code, Compile, Run and Debug C++ program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/

#include <iostream>
#include<string>
#include <cstdint>
#include <cstring>
#include <future>
struct Server_info{
    char ip[16]; // IP Sample: "192.168.1.2"
    uint16_t port;
};


struct Client{
    uint32_t id;
    char protocol[4]; // ABD or CM
    struct Server_info* servers; // An array containing the infromation to access each server
    uint32_t number_of_servers; // Number of elements in the array servers
};
using namespace std;
struct Client* client_instance(const uint32_t id, const char* protocol, const struct Server_info* servers, uint32_t number_of_servers)
{
    struct Client* cl  = new Client;
    cl->id = id;
    memcpy(cl->protocol, protocol, sizeof(protocol));
    cl->number_of_servers = number_of_servers;
    cl->servers = new struct Server_info[number_of_servers];
    cout<<sizeof(struct Server_info)*number_of_servers;
    memcpy(cl->servers, servers, sizeof(struct Server_info)*number_of_servers);
    return cl;

}
// int main()
// {
//     static struct Server_info servers[] = {
//         {"127.0.0.1", 10000},
//         {"127.0.0.1", 10001},
//         {"127.0.0.1", 10002}};
//         //cout<<servers[0].port<<endl;
//     future<struct Client*> fu = std::async(std::launch::async, client_instance,0, "ABD", servers, sizeof(servers) / sizeof(struct Server_info));
//     //struct Client* abd_clt = client_instance(0, "ABD", servers, sizeof(servers) / sizeof(struct Server_info));
    
//     struct Client* abd_clt = fu.get();
    
    
//     cout<<abd_clt->servers[0].port<<endl;
//     cout<<abd_clt->servers[sizeof(servers) / sizeof(struct Server_info) -1 ].port<<endl;
    
//     //if(strcmp(abd_clt->protocol, "CM") {
//       //  cout<< "CD";
//     //}
//     if(string(abd_clt->protocol) == "ABD") {
//         cout<< "ABD";
//     }
//      if(string(abd_clt->protocol) == "ABDD") {
//         cout<< "ABDD";
//     }
//     delete abd_clt;
//     cout<<"Hello World";
    

//     return 0;
// }
int main(int argc, char** argv) {
    //cout<< "inside mail funtion";
    cout << argc <<endl;
    cout<<argv[0]<<endl;
    string server_address =  string(argv[1]) + ":" + string(argv[2]);
    cout<<argv[1]<<endl;
    cout<<argv[2]<<endl;
    cout<<server_address<<endl;
    cout<<argv[3]<<endl;

    return 0;
}