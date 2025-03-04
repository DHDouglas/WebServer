#include "log_file.h"
#include "logger.h"

#include <cstring>
#include <unistd.h>
#include <string>

using namespace std;

std::unique_ptr<LogFile> g_logFile;

void outputFunc(const char* msg, int len) {
  g_logFile->append(msg, len);
}

void flushFunc() {
  g_logFile->flush();
}

int main(int argc, char* argv[])
{
  char name[256] = { '\0' };
  strncpy(name, argv[0], sizeof name - 1);
  g_logFile = make_unique<LogFile>(::basename(name), 200*1000);  // 每200KB换一份日志
  Logger::setOutput(outputFunc);
  Logger::setFlush(flushFunc);

  string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  for (int i = 0; i < 10000; ++i)
  {
    LOG_INFO << line << i;
    usleep(1000);
  }
}
