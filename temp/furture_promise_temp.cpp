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
#include <vector>
#include <chrono>
#include <thread>

using namespace std;

int thred_send_to_server1(promise<int>& prom, int a){
    cout<<"\nbefore sleeping,server1 thread\n";
    this_thread::sleep_for(chrono::seconds(5));
    prom.set_value(200+1);
    cout<<"\nAfter sleeping,server1 thread\n";
    return 0;
}
int thred_send_to_server2(promise<int>& prom, int a){
    cout<<"\nbefore sleeping,server2 thread\n";
    this_thread::sleep_for(chrono::seconds(15));
    prom.set_value(200+3);
    cout<<"\nAfter sleeping,server2 thread\n";
    return 0;
}


void send_to_all_server_function(promise<int>& prom){
    //promise<int> pm;
    //promise<int> pm1;

    std::vector<promise<int>> vec_prom;
    std::vector<future<int>> vec_resp;

    for (int i =0 ;i<2;i++) {
      vec_prom.push_back(promise<int>());
      vec_resp.push_back(vec_prom[i].get_future());
    }

    int a = 2;
    future<int> fu= std::async(std::launch::async, thred_send_to_server1, ref(vec_prom[0]),a);
    future<int> fu_= std::async(std::launch::async, thred_send_to_server2, ref(vec_prom[1]),a);
    //thread t1(thred_send_to_server1, ref(pm),a);
    //thread t2 (thred_send_to_server2, ref(pm1),a);

    cout<<"started both thread\n";
    
    //vector<future <int>> vec_resp ;

    //vec_resp.emplace_back(pm.get_future());
    //vec_resp.emplace_back(pm1.get_future());
    
    int majority = 0;
    std::chrono::system_clock::time_point span ;
        
    int count=0;
    int majority_value = 0;
        
    while(1) {
        for (auto it = vec_resp.begin(); it!= vec_resp.end();it++){
            span = std::chrono::system_clock::now() + std::chrono::seconds(1);
            cout<<"\n count" << count++ <<endl;
            if ((*it).valid() && (*it).wait_until(span)==std::future_status::ready) {
                cout<<"\nfound true" <<endl;
                majority_value = (*it).get();
                majority =1;
            }
           
        }
        if(majority){
            cout<< majority_value <<"Got majority"<<endl;
            prom.set_value(majority_value);
            //t1.detach();
            //t2.detach();
            break;
        }
    }
    
    cout<<"send_to_all_server_function thread finished\n";
}

void get_function() {
    promise<int> pm;
    future <int> fu1 = pm.get_future();
    //try {
         thread t1(send_to_all_server_function, ref(pm));
         t1.detach();
    /* } catch (const std::future_error& e) {
           std::cout << "Caught a future_error with code \"" << e.code()
                  << "\"\nMessage: \"" << e.what() << "\"\n";
     }*/
     //future<void> fu= std::async(std::launch::async, send_to_all_server_function, ref(pm));
     cout<< fu1.get()<<"send_to_all_server_function function return this value"<<endl;
      cout<<"\n Leaving get_function value \n";
}

int main()
{
    cout<<"In USER program\n";
    get_function();
    cout<<"\n userprogram: after doing get operation \n";
    //this_thread::sleep_for(chrono::seconds(30));
    
    return 0;
}
