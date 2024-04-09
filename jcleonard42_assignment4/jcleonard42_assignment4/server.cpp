/*****************************************************
Author Name: Jonathan Leonard
Assignment Name: Assignment 4
Class: CSC 2770, Intro to Systems and Networking
Professor: Dr. Rogers
Date: 4/22/2023
******************************************************/

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <pthread.h>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <limits>
#include <stdexcept>

using namespace std;

// Funtion prototypes
void handle_connect(int arg);
void parse(FILE *fp, map<string, string> &options, string &doc);
void ASErrorHandler(int errorNumber, const char* errorMessage);
char* ASMemoryAlloc(unsigned long memoryNeeded);
extern "C" char* AStyleMain(const char* sourceIn, const char* optionsIn, void (* fpError)(int, const char*), char* (* fpAlloc)(unsigned long));

// Global variables
const int MAX_FILE_SIZE = 1000000;
const int PORT  = 8007;
bool Req_Error;
string Req_Estr;


int main(int argc, char* argv[]) {

    struct sockaddr_in client_address;    // client address
    struct sockaddr_in server_address;    // server address
    socklen_t clientlength;               // size of client address
    int listening_socket;
    int new_listening_socket;

    if ((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  // creates a socket for a TCP/IP connection, if return < 0 then an error occurred
        throw runtime_error("Connection unsuccessful.");
    }

    bzero((void *) &server_address, sizeof(server_address));  // Sets all bytes of server address to 0
    server_address.sin_family = AF_INET;                      // Indicated server address type
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);       // htonl used to convert for server sommunication
    server_address.sin_port = htons(PORT);                    // converts port number

    if (bind(listening_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) { // Binds socket to server address
        throw runtime_error("Connection unsuccessful.");
    }

    listen(listening_socket, 5);  // Makes listening socket able to accept incoming connection with queue of 5

    for (;;) {

        clientlength = sizeof(client_address);
        new_listening_socket = accept(listening_socket,(struct sockaddr *) &client_address, &clientlength);  // Accepts incoming client connection request
        cout << "Connection successful." << endl;

        if (new_listening_socket < 0) { // Checks that client request was accepted successfully
            throw runtime_error("Connection unsuccessful.");
        }

        pthread_t thread_id;  // pointer to the newly created thread
        pthread_create(&thread_id, NULL, (void* (*)(void*))handle_connect,(void *) (long) new_listening_socket);
    }
}


void parse(FILE *fp, map<string, string> &options, string &doc) {
    char head[100];
    long unsigned int size = 0;
    char key[100];
    char value[100];

    // Reads in the first line of the header and checks for newline (eof)
    if (fgets(head, sizeof(head), fp) != NULL) { 
        // Trim the newline
        char* newline = strchr(head, '\n');
        if (newline) {
            *newline = '\0';
        } 
        else {
            throw std::runtime_error("Unexpected end of file when reading header");
        }
    } 
    else {
        throw std::runtime_error("Unexpected end of file when reading header");
    }

    // If head is not ASTYLE
    if (string(head) != "ASTYLE") {
        throw runtime_error("Expected header ASTYLE, but got " + string(head));
    }
    
    // For each line read from fp
    for (char line[100]; fgets(line, sizeof(line), fp) != nullptr;) {
        
        // If line is a newline, break
        line[strcspn(line, "\r\n")] = '\0'; 
        if (line[0] == '\0') { 
            break;
        }

        // Checks for proper number of options
        int num_options = sscanf(line, "%99[^=]=%99s", key, value); 
        if (num_options != 2) {
            throw runtime_error("Bad option: " + string(line));
        }

        // if option is not size, mode, or style
        if (string(key) != "SIZE" && string(key) != "mode" && string(key) != "style") {
            throw runtime_error("Bad option: " + string(key));
        }

        // if the option is size, convert to integer and store in size
        if (std::string(key) == "SIZE") {
            char* end_pointer;
            errno = 0;
            long int temp_size = strtol(value, &end_pointer, 10);

            if (errno != 0 || *end_pointer != '\0') {
                throw std::runtime_error("Bad SIZE value: " + std::string(value));
            }

            size = static_cast<unsigned long int>(temp_size);  // Sets size equal to temp_size
        }
        options[key] = value;  // inserts into options map
    }

    if (size < 0) {
        throw runtime_error("Bad code size");
    } 
    else if (size > MAX_FILE_SIZE) {
        throw runtime_error("Code size exceeds maximum allowed size");
    } 
    else {
        doc.resize(size);
        if (fread(&doc[0], 1, size, fp) != size) {
            throw runtime_error("Failed to read code");
        }
    }
}

void handle_connect(int ns) {
    FILE *fp = fdopen(ns, "r+"); 
    string header, doc; 
    map<string, string> options; 

    try {  // Parse options and code passed to server
        parse(fp, options, doc);
    } 
    catch (const std::exception& error) {  // If parse fails, inform user of error and exits
        std::string error_message = "Error: " + std::string(error.what());
        cout << error_message << endl;
        fclose(fp);
        return;
    }

    if(doc.empty()){  // If no code is inserted
        cout << "No code to parse." << endl;
    }

    string parameter = ""; 
    for (const auto& [key, value] : options) { 
        if (key != "SIZE") { 
            parameter += key + "=" + value + "\n"; 
        }
    }

    Req_Error = false;
    Req_Estr = "";
    char* textOut = AStyleMain(doc.c_str(), parameter.c_str(), ASErrorHandler, ASMemoryAlloc);

    if (Req_Error) {
        std::string ret_message("ERR\nSIZE=" + std::to_string(Req_Estr.size()-1) + "\n\n" + Req_Estr.c_str());
        fprintf(fp, "%s", ret_message.c_str());
    } 
    else {
        std::string ret_message("OK\nSIZE=" + std::to_string(strlen(textOut)-1) + "\n\n" + textOut);
        fprintf(fp, "%s", ret_message.c_str());
    }

    fclose(fp); // Close file
}

void ASErrorHandler(int errorNumber, const char* errorMessage) {   
    cout << "astyle error " << errorNumber << "\n" << errorMessage << endl;
    Req_Error = true;
    Req_Estr += errorMessage + std::string("\n");
}

char* ASMemoryAlloc(unsigned long memoryNeeded) {  
    char* buffer = new (std::nothrow) char [memoryNeeded];
    return buffer;
}