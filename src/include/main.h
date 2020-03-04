#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "fat.h"
#include "commands.h"

#define PROG_NAME "fat-util"
#define PROG_VERSION "0.1"
#define AUTHOR "Beraldo Leal"

int main(int argc, char *argv[]);

#endif