#include <vector>
#include <mutex>
#include <unordered_map>
#include <algorithm>

struct Data {
    std::mutex data_mutex;
    std::vector<int>& admin_ids;
    std::vector<int>& client_ids;
    std::unordered_map< int, std::vector<int> >& clients_permission_map;
    std::unordered_map<int, int>& socket_map;
    int client_socket;

    // Constructor
    Data(std::vector<int>& admins, std::vector<int>& clients, std::unordered_map< int, std::vector<int> >& permissions, std::unordered_map<int, int>& sockets, int socket)
        : admin_ids(admins), client_ids(clients), clients_permission_map(permissions), socket_map(sockets), client_socket(socket) {}

    void removeClient(int client_id) {
        
        auto client_to_remove_cid = find(client_ids.begin(), client_ids.end(), client_id);
        if (client_to_remove_cid != client_ids.end()) {
            client_ids.erase(client_to_remove_cid);
        }

        // Delete from admin_ids
        if(client_id != 0) {
            auto client_to_remove_aid = find(admin_ids.begin(), admin_ids.end(), client_id);
            if (client_to_remove_aid != admin_ids.end()) {
                admin_ids.erase(client_to_remove_aid);
            }
        }

        // Delete from clients_permission_map
        auto client_to_remove_pm = clients_permission_map.find(client_id);
        if (client_to_remove_pm != clients_permission_map.end()) {
            clients_permission_map.erase(client_to_remove_pm);
        }
    }
};
