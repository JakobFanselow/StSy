#include <iostream>
#include <map>
#include <boost/asio.hpp>
#include <utility>
#include <chrono>
#include <chrono>
#include <vector>
#include <mutex>
#include <thread>

using boost::asio::ip::udp;

using Timepoint = std::chrono::_V2::system_clock::time_point;


enum class RequestType {
    LoginRequest = 0,
    LogoutRequest = 1,
    UpdateRequest = 2
};

enum class ResponseType {
    LoginAck = 0,
    UpdatePositions = 1
};



struct Player {
    unsigned long long id = 0;
    int x = 0;
    int y = 0;
    int8_t xMovement = 0;
    int8_t yMovement= 0;
};


struct State {
    std::map<long long,Player> players;
    unsigned long long lastUnusedId = 0;
};





std::mutex stateMutex;
State state;

class UdpServer {
public:
    UdpServer(boost::asio::io_context& io_context, unsigned short port)
        : socket_(io_context, udp::endpoint(udp::v4(), port)) {
        start_receive();
    }

    void start_receive() {
        socket_.async_receive_from(
            boost::asio::buffer(recv_buffer_), remote_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0) {
                    handle_receive(bytes_recvd);
                } else {
                    start_receive(); 
                }
            });
    }

    void handle_receive(std::size_t length) {
        //std::string message(recv_buffer_.data(), length);
        std::cout << "Received: " 
                  << " from " << remote_endpoint_ << "\n";

        
        stateMutex.lock();
        handleRequest(recv_buffer_.data(),length);


        std::cout << "Known clients:\n";
        for (const auto& client : clients_) {
            std::cout << client.first << " - " << client.second.first << "\n";
        }

        stateMutex.unlock();
        start_receive();
    }

    //assumes state is locked and allowed to be modified by this function
    void handleRequest(char* data, int length) {
        if (length == 0)
            return;
        int d = data[0];
        RequestType reqtype = *(RequestType*)&d;
        std::cout << "Code: " << (int)data[0] << "\n";
        data++;

        
        length--;
        Timepoint now = std::chrono::system_clock::now();

        switch (reqtype) {
        case RequestType::LoginRequest:
            {
            std::cout << "LoginRequest detected\n";
            
            clients_.insert(std::make_pair(state.lastUnusedId,std::make_pair(remote_endpoint_,now)));
            unsigned long long id = state.lastUnusedId;
            state.lastUnusedId++;

            std::vector<char> response(9);

            response[0] = 0;

            unsigned long long idcpy = id;

            for (int i = 0; i < 8; i++) {
                response[i+1] = idcpy;
                idcpy >>= 8;
            }


            socket_.async_send_to(boost::asio::buffer(response),remote_endpoint_,
            
            [this](boost::system::error_code , std::size_t ) {

            }
        );  

            
            break;}
        
        case RequestType::LogoutRequest:
            {if (length < 8)
                return;
            unsigned long long id = 0;
            unsigned long long mult = 1;
            for (int i = 0; i < 8; i++) {
                id += data[i] * mult;
                mult *= 256;
            }
            clients_.erase(id);
            state.players.erase(id);
            break;}
        case RequestType::UpdateRequest:
            {if (length < (8+8+2))
                return;
            
            Player p;

            unsigned long long mult = 1;
            for (int i = 7; i >=0; i--) {
                p.id = ((uint8_t)data[i]) | (p.id << 8);
            }
    

            unsigned int x = 0;
            mult = 1;
            data += 8;

            for (int i = 3; i >= 0; i--) {
                x = ((uint8_t)data[i]) | (x << 8);
            }
            p.x = *(int*)&x;
            data += 4;
            unsigned int y = 0;
            mult = 1;

            for (int i = 3; i >= 0; i--) {
                y = ((uint8_t)data[i]) | (y << 8);
            }
            p.y = *(int*)&y;
            data += 4;

            p.xMovement = data[0];
            data+=1;
            p.yMovement = data[0];

            std::cout << "Id: " << p.id << "\n";
            std::cout << "x: " << p.x << "\n";
            std::cout << "y: " << p.y << "\n";
            std::cout << "dx: " << (int)p.xMovement << "\n";
            std::cout << "dy: " << (int)p.yMovement << "\n";


            clients_[p.id] = std::make_pair(remote_endpoint_,now);
            state.players[p.id] = p;
            break;}
        }
    }

    void broadcast() {
        
        stateMutex.lock();
        std::vector<uint8_t> message;

        auto it = state.players.begin();

        Timepoint now = std::chrono::system_clock::now();

        while (it != state.players.end()) {
            unsigned long long id = (*it).first;
            std::chrono::duration<double> seconds = now - clients_[id].second;

            if (seconds.count() > 100) {
                auto del = it;
                it++;

                state.players.erase(del);
                clients_.erase(id);
                std::cout << "Timeout from id: " << id << "\n";

                continue;
            }


            it++;
        }


        message.push_back(1);

        int playercount = state.players.size();

        if (bc%100 == 0) {
            std::cout << "Sent 100 broadcasts" << "\n";
            std::cout << playercount << " active players connected\n";
        }
        bc++;

        for (int i = 0; i < 4; i++) {
            message.push_back(playercount& 0xff);
            playercount >>= 8;
        }

        for (auto a : state.players) {
            
            Player p = a.second;
            for (int i = 0; i < 8; i++) {
                message.push_back( p.id& 0xff);
                p.id >>= 8;
            }

            for (int i = 0; i < 4; i++) {
                message.push_back(p.x & 0xff);
                p.x >>= 8;
            }

            for (int i = 0; i < 4; i++) {
                message.push_back(p.y& 0xff);
                p.y >>= 8;
            }

            message.push_back(p.xMovement);

            message.push_back(p.yMovement);
        }
        message.push_back(0xee);

        for (auto a : clients_) {
            socket_.async_send_to(boost::asio::buffer(message),a.second.first,
            [this](boost::system::error_code , std::size_t ) {

            });
        }






        stateMutex.unlock();
    }

    unsigned long long bc = 0;

    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    std::array<char, 1024> recv_buffer_;
    std::map<long long, std::pair<udp::endpoint,Timepoint>> clients_;
};

void permanentBroadcast(UdpServer& server) {

    while(true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    server.broadcast();
    }
    
}

int main() {
    static_assert(sizeof(int) == 4);
    try {
        boost::asio::io_context io_context;
        UdpServer server(io_context, 16632);
        std::cout << "UDP server listening on port 16632...\n";
        std::thread broadcastThread(permanentBroadcast, std::ref(server));
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}