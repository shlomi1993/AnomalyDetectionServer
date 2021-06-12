/*
 * CLI.h
 *
 * Author: Shlomi Ben-Shushan
 */

#ifndef CLI_H_
#define CLI_H_

#include <string.h>

#include "commands.h"

using namespace std;

// CLI class - well I've changed nothing in this class.
class CLI {
    vector<Command*> commands;
    DefaultIO* dio;
public:
    CLI(DefaultIO* dio);
    void start();
    virtual ~CLI();
};

#endif /* CLI_H_ */
