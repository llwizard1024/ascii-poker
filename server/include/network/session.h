#pragma once

#include "network/i_connection.h"
#include "poker/protocol.h"

#include <array>
#include <asio/ip/tcp.hpp>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace poker::server {

class IMessageProcessor;

class Session : public IConnection, public std::enable_shared_from_this<Session> {
public:
    explicit Session(asio::ip::tcp::socket socket, std::shared_ptr<IMessageProcessor> processor);
    ~Session() override;
    void start();
    void close_session();

    void send(const poker::protocol::ServerMessage& msg) override;
    std::string get_name() const override { return player_name_; }
    bool is_connected() const override { return !closed_; }
    bool is_authenticated() const override { return authenticated_; }
    void set_player_name(std::string name) override;

private:
    void do_read_header();
    void do_read_body(uint32_t length);
    void do_write();
    void on_message(const std::string& json);

    asio::ip::tcp::socket socket_;
    std::array<uint8_t, 4> header_buffer_;
    std::string body_buffer_;
    std::deque<std::vector<uint8_t>> write_queue_;
    std::shared_ptr<IMessageProcessor> processor_;
    std::string player_name_;
    bool authenticated_ = false;
    bool disconnect_notified_ = false;
    bool closed_ = false;
    bool write_in_progress_ = false;
};

} // namespace poker::server
