/******************************************************
Name: Jonathan Leonard
Class: CSC 2770 - Intro To Systems and Networking
Assignment: Assignment 3
Professor: Dr. Mike Rogers
Date: 4/6/2023
*******************************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

/* MAX_LINES and MAX_LINE_LENGTH forms a 2D character array that I used 
to avoid using vectors because I have never worked with them before */
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 100

// Filter function prototype
void filter(char lines[MAX_LINES][MAX_LINE_LENGTH], int num_lines, const char *user_cmd);

int main() {
    int num_lines = 0;                       // Number of current lines
    char lines[MAX_LINES][MAX_LINE_LENGTH];  // Number of possible lines and characters per line

    cout << "Line editor\n";

    while (true) {
        cout << "edlin> ";
        char cmd[MAX_LINE_LENGTH];                              // User choice

        if (fgets(cmd, MAX_LINE_LENGTH, stdin) == NULL) {       // Reads in user choice and checks for null value
            break;  
        }
        
        cmd[strcspn(cmd, "\n")] = '\0';

        if (strcmp(cmd, "q") == 0) {                            // If 'q', exits
            cout << "Exiting the editor\n";
            break;
        } 
        else{
            if (strcmp(cmd, "l") == 0) {
                for (int i = 0; i < num_lines; i++) {           // Iterates through and prints lines
                    cout << lines[i] << endl;
                }
            } 

            else if (strncmp(cmd, "r ", 2) == 0) {              // Reads from a designated file
                char *filename = cmd + 2;
                FILE *file = fopen(filename, "r");              // Opens file

                if (file == NULL) {                             // File can't be opened
                    printf("Cannot open file %s\n", filename);
                }
                else {                                          // File is opened
                    num_lines = 0;
                    while (fgets(lines[num_lines], MAX_LINE_LENGTH, file) != NULL && num_lines < MAX_LINES) {  // Loads all lines from the file into the buffer
                        lines[num_lines][strcspn(lines[num_lines], "\n")] = '\0';
                        num_lines++;
                    }
                    fclose(file); // Closes file
                }
            } 

            else if (strncmp(cmd, "s ", 2) == 0) { // Writes buffer to file
                char *filename = cmd + 2;
                ofstream file(filename);           // Opens file

                if (!file.is_open()) {
                    cout << "Cannot open file " << filename << endl;
                } 
                else {
                    for (int i = 0; i < num_lines; i++) {  // Iterates through lines, copying buffer to file
                        file << lines[i] << endl;
                    }
                    file.close();                  // Closes file
                }
            } 

            else if (strncmp(cmd, "e ", 2) == 0) {              // Edits buffer
                int line_no;
                if (sscanf(cmd + 2, "%d", &line_no) != 1) {     // Checks that user enetered a correct line number
                    cout << "Invalid line number" << endl;
                } 
                else {
                    if (line_no < 0 || line_no >= num_lines) {  // Appends num_lines to lines
                        strncpy(lines[num_lines], cmd + 2, MAX_LINE_LENGTH);
                        num_lines++;
                    } 
                    else {                                      // Sets lines[line_no] = rest of line
                        strncpy(lines[line_no], cmd + 2, MAX_LINE_LENGTH);
                    }
                }
            } 

            else if (cmd[0] == '!') {
                filter(lines, num_lines, cmd);       // Calls filter function and passes unix command
            } 

            else {
                cout << "Invalid command" << endl;   // Catch all for if nothing else worked
            }
        }
    }
}   


// Filter function: Responsible for creating a child that executes a unix command
void filter(char lines[MAX_LINES][MAX_LINE_LENGTH], int num_lines, const char *user_cmd) {
    
    char *endptr;
    int line_no = strtol(user_cmd + 1, &endptr, 10);

    // Checks for line number input
    if (endptr == user_cmd + 1) {
        cout << "No line number given" << endl;
        return;
    }

    const char *ucmd;
    // Checks for user comand and loads it into ucmd variable
    if (*(endptr) == ' ') {
        ucmd = endptr + 1; 
    }
    else if (*(endptr) == '\0') { 
        cout << "No Unix command given" << endl;
        return;
    }  
    else if (*(endptr) == '!') {
        ucmd = endptr + 1; 
    } 
    else {
        cerr << "Command syntax is incorrect" << endl;
        return;
    }
    
    if (line_no < 0 || line_no >= num_lines) {  // If line number does not exist in the file yet
        cout << "Line number does not exist" << endl;
        return;
    }
    
    int from_parent[2], to_parent[2];           // Creates a pipe
    if (pipe(from_parent) < 0 || pipe(to_parent) < 0) {  // Checks that the pipe was created successfully
        cerr << "Could not create pipes" << endl;
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        cerr << "Could not execute fork function" << endl;
        return;
    }

    if (pid == 0) { // If I am the child
        
        dup2(from_parent[0], STDIN_FILENO);  // copy read end of from_parent to stdin
        dup2(to_parent[1], STDOUT_FILENO);   // copy write end of to_parent to stdou
        close(from_parent[0]);               // close read end of from_parent and write end of from_parent
        close(from_parent[1]);               
        close(to_parent[0]);                 // close read end of to_parent and write end of to_parent
        close(to_parent[1]);                 
        
        char *args[] = { "/bin/sh","-c", (char *) ucmd, NULL };  // create args array to run ucmd in a shell (gives an error because of type but doesn't effect functionality. I can't be bothered)
        if (execve("/bin/sh", args, NULL) < 0) {
            cerr << "Could not execute Unix command" << endl;
            exit(1);
        }
    } 
    else { // If I am the parent
        
        // close read and write ends
        close(from_parent[0]);
        close(to_parent[1]);
        
        FILE *from_parent_fp = fdopen(from_parent[1], "w");  // open file
        fprintf(from_parent_fp, "%s\n", lines[line_no]);     // print to file

        // close file
        fclose(from_parent_fp);
        close(from_parent[1]);
        
        FILE *to_parent_fp = fdopen(to_parent[0], "r");      // Reads result from pipe
        char result[MAX_LINE_LENGTH];

        if (fgets(result, MAX_LINE_LENGTH, to_parent_fp) == NULL) {
            cerr << "Error reading Unix command output" << endl;
        } 
        else {
            result[strcspn(result, "\n")] = '\0';
            strcpy(lines[line_no], result);
        }

        fclose(to_parent_fp);
        close(to_parent[0]);

        int status;
        waitpid(pid, &status, 0); // Waits for child process to finish execution
    }
}