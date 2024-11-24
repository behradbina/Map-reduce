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

#define FIFO ".fifo"
#define WRITE 1
#define READ 0

using namespace std;

Logger logger;
string name;

struct store_data
{
    double total_profit;
    unordered_map<string, pair<int, double>> remaining_parts;
};

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

int arg_find_top_most_input_product(string part, vector<vector<string>>&data)
{
    for (size_t i = 0; i < data.size(); i++)
    {
        if (data[i][0]==part && data[i][3]=="input" && stoi(data[i][2]) > 0)
        {
            return i;
        }
        
    }
    printf("Something is wrong in process\n");
    return -1;
}

string trim(const string& str) 
{ 
    size_t first = str.find_first_not_of(" \t\n\r"); size_t last = str.find_last_not_of(" \t\n\r"); return str.substr(first, (last - first + 1)); 
}

vector<vector<string>> read_csv(string path) 
{ 
    vector<vector<string>> data;
    ifstream file(path);
    string line;
    while (getline(file, line)) 
    { 
        stringstream ss(line);
        string cell;
        vector<string> row;
        while (getline(ss, cell, ',')) 
        { 
            row.push_back(trim(cell)); 
        } 
        data.push_back(row); 
    }

    file.close(); 
    return data;
}

unordered_map<string, pair<int, double>> compute_remaining_parts(vector<vector<string>>data)
{
    unordered_map<string, pair<int, double>> umap;// part->quantity remained, value remained

    for (size_t i = 0; i < data.size(); i++)
    {
        if (data[i][3] == "input" && data[i][2] != "0")
        {
            if (umap.find(data[i][0]) != umap.end())
            { 
                umap[data[i][0]].first += stoi(data[i][2]); 
                umap[data[i][0]].second += stoi(data[i][2]) * stod(data[i][1]); 
            } 
            else 
            { 
                umap[data[i][0]].first = stoi(data[i][2]);  
                umap[data[i][0]].second = stoi(data[i][2]) * stod(data[i][1]);      
            }
        
        }
    }
    return umap;
}

string encode_part_data(unordered_map<string, pair<int, double>> &data, string part)
{
    return to_string(data[part].first)+","+ to_string(data[part].second)+","+name+ "\n" ;
}

bool is_requested_part(vector<string>parts, string part)
{
    for (size_t i = 0; i < parts.size(); i++)
    {
        if (part == parts[i])
        {
            return true;
        }
        
    }
    return false;
    
}

store_data compute_total_profit(string path, vector<string>parts)
{
    store_data sd;
    double profit = 0;

    vector<vector<string>>data = read_csv(path);
    
    for (size_t i = 0; i < data.size(); i++)
    {
        if (data[i][3] == "output" && is_requested_part(parts, data[i][0]))
        {
            while (stoi(data[i][2]) != 0)
            {
                int first_inp = arg_find_top_most_input_product(data[i][0], data);
                if( stoi(data[i][2]) > stoi(data[first_inp][2]))
                {
                    profit += (stod(data[i][1])-stod(data[first_inp][1])) * stoi(data[first_inp][2]);
                    data[i][2] = to_string(stoi(data[i][2])-stoi(data[first_inp][2]));
                    data[first_inp][2] = "0";
                }
                else
                {
                    profit += (stod(data[i][1])-stod(data[first_inp][1])) * stoi(data[i][2]);
                    data[first_inp][2] = to_string(stoi(data[first_inp][2])-stoi(data[i][2]));
                    data[i][2] = "0";
                }
            } 
        }
    }
    
    sd.total_profit = profit;
    sd.remaining_parts = compute_remaining_parts(data);
    return sd;
}

void make_named_pipe(vector<string>&parts, unordered_map<string, pair<int, double>> data)
{
    for (size_t i = 0 ; i < parts.size() ; i++)
    {
        string fifo_name = "fifo_"+parts[i];
        logger.log(fifo_name, "red");
		mkfifo(fifo_name.c_str() , 0666);
        logger.log("FIFO " + parts[i] + " created" , "green");
		int write_fd = open(fifo_name.c_str() , O_WRONLY);
        string part_data = encode_part_data(data, parts[i]);
        logger.log(part_data + " sent to " + parts[i] , "blue");
        write(write_fd , part_data.c_str() , part_data.length());
        logger.log(name + " : " + to_string(i) + " wrote sucessfully", "yellow");
		close(write_fd);
        logger.log(name + " : " + to_string(i)+ " close sucessfully", "yellow");
	}
}

vector<string> get_chosen_parts(vector <string> commands)
{
    vector<string>parts;
    for (size_t i = 1; i < commands.size()-2; i++)
    {
        parts.push_back(commands[i]);
    }
    return parts;
    
}

int main(int argc, char const *argv[])
{
    int read_fd = atoi(argv[0]);
    int write_fd = atoi(argv[1]);
    vector <string> commands = get_commands(read_fd);
    name = commands[0];
    string path =  commands[commands.size()-1]+commands[0];
    vector<string>parts = get_chosen_parts(commands); 
    logger.log(path, "blue");
    store_data sd = compute_total_profit(path, parts);

    string x;

    for (size_t i = 0; i < parts.size(); i++)
    {
        x += "'"+ parts[i]+"'";
    }
    
    logger.log(x, "red");

    make_named_pipe(parts, sd.remaining_parts);

    logger.log("ALL named_pipes created", "green");

    string total_p = to_string(sd.total_profit);
    logger.log(name +" profit " + total_p, "blue");
    write(write_fd, total_p.c_str(), total_p.length());
    close(write_fd);
    return 0;
}
