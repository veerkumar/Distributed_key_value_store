#include "commons.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
using namespace std;

std::ifstream infile("server_info.txt");

int main(){
	std::vector<string> server_list;
	string s1, s2;
	while (infile >> s1 >> s2)
	{
    // process pair (a,b)
		cout<< s1+":"+s2<<endl;
		server_list.push_back(s1+":"+s2);
	}


}