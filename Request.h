#include <string>

struct Request {
    int client_id;
    std::string action;
    int target_client_1;
    int target_client_2;

    Request() = default;
    Request(int id, std::string a, int target1, int target2)
        : client_id(id), action(a), target_client_1(target1), target_client_2(target2) {}
};