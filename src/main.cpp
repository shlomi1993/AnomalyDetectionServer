/*
 * main.cpp
 *
 * Author: Dr. Eli Khalasche
 * Edited by: Shlomi Ben-Shushan
 */

#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "CLI.h"
#include "HybridAnomalyDetector.h"
#include "Server.h"
#include "commands.h"

using namespace std;

class STDtest : public DefaultIO {
    ifstream in;
    ofstream out;
public:
    STDtest(string inputFile, string outputFile) {
        in.open(inputFile);
        out.open(outputFile);
    }
    virtual string read() {
        string s;
        in >> s;
        return s;
    }
    virtual void write(string text) { out << text; }
    virtual void write(float f) { out << f; }
    virtual void read(float *f) { in >> *f; }
    void close() {
        if (in.is_open()) in.close();
        if (out.is_open()) out.close();
    }
    ~STDtest() { close(); }
};

void check(string outputFile, string expectedOutputFile) {
    size_t chk[] = {31, 62, 63, 74, 75, 86, 87, 98, 99, 110, 111};
    ifstream st(outputFile);
    ifstream ex(expectedOutputFile);
    size_t i = 1, j = 0;
    string lst, lex;
    while (!st.eof() && !ex.eof()) {
        getline(st, lst);
        getline(ex, lex);
        if (i < 13 && lst.compare(lex) != 0) {  // 12
            cout << "line " << i << " expected: " << lex << " you got " << lst << endl;
            cout << "wrong output (-1)" << endl;
        } else if (j < 11 && i == chk[j]) {
            if (lst.compare(lex) != 0) {  // 88
                cout << "line " << i << " expected: " << lex << " you got " << lst << endl;
                cout << "wrong output (-8)" << endl;
            }
            j++;
        }
        i++;
    }
    if (j < 11)
        cout << "wrong output size (-" << (11 - j) * 8 << ")" << endl;
    st.close();
    ex.close();
}

void writeStr(string input, int serverFD) {
    write(serverFD, input.c_str(), input.length());
    write(serverFD, "\n", 1);
}

string readStr(int serverFD) {
    string serverInput = "";
    char c = 0;
    read(serverFD, &c, sizeof(char));
    while (c != '\n') {
        serverInput += c;
        read(serverFD, &c, sizeof(char));
    }
    return serverInput;
}

void readMenue(ofstream &out, int serverFD) {
    bool done = false;
    while (!done) {
        // read string line
        string serverInput = readStr(serverFD);
        if (serverInput == "6.exit")
            done = true;
        out << serverInput << endl;
    }
}

int initClient(int port) throw(const char *) {
    int serverFD, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    serverFD = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFD < 0)
        throw "socket problem";
    server = gethostbyname("localhost");
    if (server == NULL)
        throw "no such host";
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(port);
    if (connect(serverFD, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        throw "connection problem";
    return serverFD;
}

void clientSide1(int port, string outputFile) throw(const char *) {
    int serverFD = initClient(port);
    ofstream out(outputFile);
    readMenue(out, serverFD);
    out.close();
    string input = "6";
    writeStr(input, serverFD);
    close(serverFD);
    cout << "end of client 1" << endl;
}

void clientSide2(int port, string outputFile) throw(const char *) {
    int serverFD = initClient(port);
    ofstream out(outputFile);
    ifstream in("./tests/Server_input.txt");
    string input = "";
    while (input != "6") {
        readMenue(out, serverFD);
        getline(in, input);
        writeStr(input, serverFD);
        if (input == "1") {
            out << readStr(serverFD) << endl;  // please upload...
            while (input != "done") {
                getline(in, input);
                writeStr(input, serverFD);
            }
            out << readStr(serverFD) << endl;  // Upload complete
            out << readStr(serverFD) << endl;  // please upload...
            input = "";
            while (input != "done") {
                getline(in, input);
                writeStr(input, serverFD);
            }
            out << readStr(serverFD) << endl;  // Upload complete
        }
        if (input == "3") {
            out << readStr(serverFD) << endl;  // Anomaly... complete
        }
        if (input == "5") {
            out << readStr(serverFD) << endl;  // please upload...
            while (input != "done") {
                getline(in, input);
                writeStr(input, serverFD);
            }
            out << readStr(serverFD) << endl;  // Upload complete
            out << readStr(serverFD) << endl;  // TPR
            out << readStr(serverFD) << endl;  // FPR
        }
    }
    in.close();
    out.close();
    close(serverFD);
    cout << "end of client 2" << endl;
}

size_t checkSv(string outputFile, string expectedOutputFile) {
    ifstream st(outputFile);
    ifstream ex(expectedOutputFile);
    size_t i = 0;
    string lst, lex;
    while (!ex.eof()) {
        getline(st, lst);
        getline(ex, lex);
        if (lst.compare(lex) != 0) i++;
    }
    st.close();
    ex.close();
    return i;
}

int CLITest() {
    STDtest std("./tests/CLI_input.txt", "./tests/CLI_output.txt");
    CLI cli(&std);
    cli.start();
    std.close();
    check("./tests/CLI_output.txt", "./tests/CLI_expectedOutput.txt");
    cout << "CLI Test is Done" << endl;
    return 0;
}

int ServerTest() {
    srand(time(NULL));
    int port = 5000 + rand() % 1000;
    string outputFile1 = "./tests/Server_output_menu";
    string outputFile2 = "./tests/Server_output";
    int x = rand() % 1000;
    outputFile1 += to_string(x);
    outputFile1 += ".txt";
    outputFile2 += to_string(x);
    outputFile2 += ".txt";
    try {
        AnomalyDetectionHandler adh;
        Server server(port);
        server.start(adh);  // runs on its own thread
        // let's run 2 clients
        clientSide1(port, outputFile1);
        clientSide2(port, outputFile2);
        server.stop();  // joins the server's thread
    } catch (const char *s) {
        cout << s << endl;
    }
    size_t mistakes = checkSv(outputFile1, "./tests/Server_expected_output_menu.txt");
    mistakes += checkSv(outputFile2, "./tests/Server_expected_output.txt");
    if (mistakes > 0)
        cout << "you have " << mistakes << " mistakes in your output (-" << (mistakes * 2) << ")" << endl;
    cout << "Server Test is Done" << endl;
    return 0;
}

int main() {
    CLITest();
    ServerTest();
}
