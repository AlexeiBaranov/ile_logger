#include <stdio.h>
#include <string.h>
#include "logger.h"

int main(int argv, void *argc[]) {

   void *l;

   l = open_logger("its_me", 16, 0, LOGGER_UDP, "localhost", "514");

   log_it(l, 0, "message: %s", "123");

   close_logger(l);

   return 0;
}