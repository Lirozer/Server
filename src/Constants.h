constexpr int PORT = 8080;
constexpr int NUMBER_CONNECTION = 10;
constexpr int MAX_EVENTS = 64;
constexpr int WAIT_TIME = 100;

constexpr int MESSAGE_BUFFER_SIZE = 256;

constexpr char TIME_FORMAT[] = "%Y-%m-%d %H:%M:%S";

namespace Command
{
    constexpr char TIME[] = "/time";
    constexpr char STATS[] = "/stats";
    constexpr char DISCONNECT[] = "/shutdown";
}