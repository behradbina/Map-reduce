#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <sys/wait.h>
#include <vector>
#include <bits/stdc++.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <algorithm>
#include <unordered_map>
#include "logger.hpp"

#define MAX_BUF_SIZE 1000

using namespace std;

Logger logger;

vector <string> tokenize_m(string chosen_pos,char delimiter)
{
    stringstream ss(chosen_pos); 
    string s; 
    vector <string> str;
    while (getline(ss, s, delimiter)) {    
        str.push_back(s);
    }
    return str; 
}

vector<string> get_commands(int &read_fd){
    char buf [100] ; 
    read(read_fd , buf , 100);
    close(read_fd);
    vector <string> commands = tokenize_m(string(buf),',');
    return commands;
}

void update_data(unordered_map<string, pair<int, double>> &data, string store, int quantity, double value)
{
    if (data.find(store) == data.end())
    { 
        data[store].first = quantity; 
        data[store].second = value;
    } 
}

unordered_map<string, pair<int, double>> read_data_named_pipe(string name, int store_count)
{
    unordered_map<string, pair<int, double>> remaining_data;
    string fifo_name =  "fifo_" + name;
    logger.log(name + " : waiting to create "+ fifo_name+ " fifo file", "red");

    while (access(fifo_name.c_str(), F_OK) == -1)
    {
        this_thread::sleep_for(chrono::milliseconds(500));
    }

    logger.log(name + "Fifo file created", "red");


    int fifo_fd = open(fifo_name.c_str(), O_RDONLY);

    if (fifo_fd == -1)
    {
        logger.log("FILED to open Fifo filed", "red");
    }
    
    logger.log("Fifo file " + fifo_name +" open", "red");

    int valid_message_recieved = 0;

    while (true)
    {
        char buf[MAX_BUF_SIZE] = {'\0'};

        while (read(fifo_fd, buf, MAX_BUF_SIZE) > 0 )
        {
            string message = string(buf);
            size_t pos;
            while((pos = message.find('\n')) != string::npos)
            {
                string m = message.substr(0, pos);
                valid_message_recieved ++;
                logger.log(name + " : " + m, "yellow");

                cout << "store_count: " << store_count << " "<< valid_message_recieved<< endl;
                vector<string>row = tokenize_m(m, ',');
                update_data(remaining_data, row[2], stoi(row[0]), stod(row[1]));
                message = message.erase(0, pos+1);
                if (valid_message_recieved == store_count) break;
                
            }
            if (valid_message_recieved == store_count) break;
        }
        if (valid_message_recieved == store_count)
        {
            logger.log(name + " Recieved all data from all sources" , "green");
            break;
        }   
    }  

    return remaining_data;
}

string total_remaining(unordered_map<string, pair<int, double>>remain_data, string name)
{
    int sum1=0;
    double sum2=0;
    for (const auto& entry : remain_data) 
    { 
        sum1 += entry.second.first;
        sum2 += entry.second.second; 
    }

    logger.log("FINAL RESULT PRODUCT "+ to_string(sum1)+","+to_string(sum2), "green");

    return to_string(sum1)+","+to_string(sum2)+","+name;
}

int main(int argc, char const *argv[])
{
    int read_fd = atoi(argv[0]);
    string name;
    int write_fd = atoi(argv[1]);
    vector <string> commands = get_commands(read_fd);
    name = commands[0];
    int store_count = stoi(commands[1]);
    logger.log("PRODUCT " + name, "blue");
    unordered_map<string, pair<int, double>> remain_data = read_data_named_pipe(name, store_count);
    string result = total_remaining(remain_data, name);
    write(write_fd, result.c_str(), result.length());
    close(write_fd);
    return 0;
}
