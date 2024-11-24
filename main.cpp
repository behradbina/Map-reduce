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
#include "logger.hpp"

#define STORE "./store_program"
#define PRODUCT "./product_program"

#define MAX 10000
#define WRITE 1
#define READ 0

using namespace std;

Logger logger;

vector <string> find_stores_dir(string path)
{
    vector <string> files;
    vector <string> org_files;
    DIR *dr;
    struct dirent *en;

    dr = opendir(path.c_str());

    if (dr) {
        while ((en = readdir(dr)) != NULL) {
            files.push_back(en->d_name);
        }
        closedir(dr);
    }

    for (size_t i = 0; i< files.size(); i++)
    {
        if ( files[i] != "." && files[i] != ".." && files[i] != "Parts.csv")
        {
            org_files.push_back(files[i]);
        }
    }

    return org_files;
}

vector<string> read_products(const string &filePath) {
    vector<string> columns;
    ifstream file(filePath);
    string line;

    if (file.is_open()) {
        // Read the first line (first row)
        if (getline(file, line)) {
            stringstream ss(line);
            string column;
            
            // Split the line by commas and store the columns in the vector
            while (getline(ss, column, ',')) {
                columns.push_back(column);
            }
        }
        file.close();
    } else {
        cerr << "Unable to open file: " << filePath << endl;
    }

    return columns;
}

void show_stores(string path){

    vector <string> stores = find_stores_dir(path);
    cout << "The number of stores is: " << stores.size() << endl;
    cout<<"Store names are:"<<endl;
    
    for (size_t i = 0; i < stores.size(); i++)
    {
        cout << stores[i] << " - ";
    }
    cout<<endl;

}

vector <string> tokenize_m(string inp_str,char delimiter)
{
    stringstream ss(inp_str); 
    string s; 
    vector <string> str;
    while (getline(ss, s, delimiter)) {    
        str.push_back(s);
    }
    return str; 
}

vector <string> input_chosen_products(string path)
{
    vector <string> products = read_products(path+ "Parts.csv");
    string temp;

    cout << "Enter products:" << endl;

    for (size_t i = 0; i < products.size(); i++)
    {
        cout << i << " : " <<products[i] << endl; 
    }
    
    getline(cin, temp);
    vector <string> chosen_resources = tokenize_m(temp,' ');

    return chosen_resources;
}

string store_data(string file_name, string chosen_resources, int i, string path)
{

    return file_name +','+ chosen_resources +','+to_string(i)+','+path;
}

int create_process(int& write_pipe, int& read_pipe, string executable)
{
    int pipe_fd[2]; // 
    int pipe_fd2[2]; //

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
    }
    if (pipe(pipe_fd2) == -1) {
        perror("pipe");
    }

    int pid = fork();

    if (pid == 0) {
        // Child process
        close(pipe_fd[WRITE]);
        close(pipe_fd2[READ]);
        char read_fd[20];
        char write_fd[20];
        sprintf(read_fd , "%d" , pipe_fd[READ]);
        sprintf(write_fd , "%d" , pipe_fd2[WRITE]);
        execl(executable.c_str(), read_fd , write_fd, NULL);
        perror("execl");
    } else if (pid > 0) {
        // Parent process
        close(pipe_fd[READ]);
        close(pipe_fd2[WRITE]);
        read_pipe = pipe_fd2[READ] ;
        write_pipe = pipe_fd[WRITE];
    }else{
        perror("fork");
    }
    return pid;
}

string coded_resources(vector <string> resources)
{
	string result = resources[0];
	for (size_t i=1; i< resources.size(); i++)
    {
		result = result + ',' + resources[i];
    }
    return result;
}

vector<int> create_stores_process(vector<string> stores, vector<string> chosen_parts, vector <int>& child_pids, string path)
{
    vector<int> read_pipes;

    string parts = coded_resources(chosen_parts);

    for (size_t i = 0; i < stores.size(); i++)
    {
        int write_pipe;
        int read_pipe;
        int pid = create_process(write_pipe,read_pipe, STORE);

        logger.log("STORE Forked and PID: " + to_string(pid) , "green");
        logger.log("STORE Informed about its unnamed pipe" , "green");
        
        string data = store_data(stores[i], parts, i, path);
        write(write_pipe, data.c_str(), 100);

        logger.log("STORE Introduced throw pipe as: " + data   , "green");
        child_pids.push_back(pid);
        close(write_pipe);
        read_pipes.push_back(read_pipe);
    }
    return read_pipes;

}

vector<int> create_parts_process(vector<string> stores, vector<string> chosen_parts, vector <int>& child_pids)
{
    vector<int> read_pipes;

    for (size_t i = 0; i < chosen_parts.size(); i++)
    {
        int write_pipe;
        int read_pipe;
        int pid = create_process(write_pipe,read_pipe, PRODUCT);

        logger.log("PART Forked and PID: " + to_string(pid) , "yellow");
        logger.log("PART Informed about its unnamed pipe" , "yellow");
        string data = chosen_parts[i]+","+to_string(stores.size())+","+coded_resources(stores);
        write(write_pipe, data.c_str(), 100);

        logger.log("PART Introduced throw pipe as: " + chosen_parts[i], "yellow");
        child_pids.push_back(pid);
        close(write_pipe);
        read_pipes.push_back(read_pipe);
    }
    return read_pipes;

}

void show_store_result(vector<int> child_pids, vector<int> read_store_pipes)
{
    double total_user_request_profit = 0;

    logger.log("Wait till all store processes ends", "yellow");
    for (int i  : child_pids)
    {
        int status;
        waitpid(i, &status, 0);
    }
    logger.log("All store processes ended", "yellow");

    for (size_t i = 0; i < read_store_pipes.size(); i++)
    {
        char buf[MAX] = {'\0'};
        read(read_store_pipes[i], buf, MAX);
        close(read_store_pipes[i]);
        string p = string(buf);
        total_user_request_profit += stod(p);
    }
    
    cout << "Total profit of all stores : " << total_user_request_profit <<  endl;

}

void show_product_result(vector<int> child_product_pids, vector<int> read_part_pipes)
{
    for (int i  : child_product_pids)
    {
        int status;
        waitpid(i, &status, 0);
    }

    for (size_t i = 0; i < read_part_pipes.size(); i++)
    {
        char buf[MAX] = {'\0'};
        read(read_part_pipes[i], buf, MAX);
        close(read_part_pipes[i]);
        string p = string(buf);
        vector<string>data=tokenize_m(p, ',');
        cout << data[2] << " : " << endl;
        cout << "   Total left over Quantity : " << data[0] << endl;
        cout << "   Total Leftover Price : " << data[1] << endl;
    }
    
}

int main(int argc, char const *argv[])
{
    string path = argv[1]; 
    show_stores(path);
    vector<string>stores = find_stores_dir(path);
    vector <string> chosen_products = input_chosen_products(path);

    vector<int> child_store_pids;
    vector<int> child_product_pids;

    vector<int> read_store_pipes = create_stores_process(stores, chosen_products, child_store_pids, path);  
    vector<int> read_part_pipes = create_parts_process(stores, chosen_products,child_product_pids);

    show_store_result(child_store_pids, read_store_pipes);
    show_product_result(child_product_pids, read_part_pipes);

    return 0;
}
