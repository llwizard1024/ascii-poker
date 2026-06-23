#include "network/session.h"

#include "messaging/i_message_processor.h"
#include "poker/packet_framing.h"
#include "poker/protocol.h"

#include <json/json.hpp>
#include <spdlog/spdlog.h>

#include <asio/buffer.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

namespace poker::server {

namespace pp = poker::protocol;

Session::Session(asio::ip::tcp::socket socket, std::shared_ptr<IMessageProcessor> processor)
    : socket_(std::move(socket))
    , processor_(std::move(processor))
{
}

void Session::set_player_name(std::string name)
{
    player_name_ = std::move(name);
    authenticated_ = true;
}

Session::~Session()
{
    if (socket_.is_open()) {
        std::error_code ec;
        socket_.close(ec);
    }
}

void Session::send(const pp::ServerMessage& msg)
{
    if (closed_) {
        return;
    }

    const std::string json_str = nlohmann::json(msg).dump();
    if (json_str.size() > poker::network::MAX_PACKET_SIZE) {
        spdlog::error("Message too large to send");
        return;
    }

    auto packet = poker::network::encode_packet(json_str);

    write_queue_.push_back(std::move(packet));

    if (!write_in_progress_) {
        do_write();
    }
}

void Session::do_write()
{
    if (closed_ || write_in_progress_ || write_queue_.empty()) {
        return;
    }

    write_in_progress_ = true;
    auto self = shared_from_this();
    auto packet = std::make_shared<std::vector<uint8_t>>(write_queue_.front());

    asio::async_write(
        socket_,
        asio::buffer(*packet),
        [this, self, packet](const std::error_code& ec, std::size_t bytes_transferred) {
            write_in_progress_ = false;

            if (closed_) {
                return;
            }

            if (!ec) {
                spdlog::debug("Response successfully sent. Bytes: {}", bytes_transferred);

                if (!write_queue_.empty()) {
                    write_queue_.pop_front();
                }

                if (!write_queue_.empty()) {
                    do_write();
                }
            } else {
                spdlog::error("Write error: {}. Closing session.", ec.message());
                close_session();
            }
        });
}

void Session::start()
{
    do_read_header();
}

void Session::do_read_header()
{
    if (closed_) {
        return;
    }

    auto self = shared_from_this();
    asio::async_read(
        socket_,
        asio::buffer(header_buffer_, 4),
        asio::transfer_exactly(4),
        [this, self](const std::error_code& ec, std::size_t bytes_transferred) {
            if (closed_) {
                return;
            }

            if (!ec && bytes_transferred == 4) {
                uint32_t length = 0;
                if (!poker::network::decode_packet_length(header_buffer_.data(), length)) {
                    spdlog::warn("Invalid packet size. Closing session.");
                    close_session();
                    return;
                }
                body_buffer_.resize(length);
                do_read_body(length);
            } else {
                spdlog::warn("Read error: {}. Closing session.", ec.message());
                close_session();
            }
        });
}

void Session::do_read_body(uint32_t length)
{
    if (closed_) {
        return;
    }

    auto self = shared_from_this();
    asio::async_read(
        socket_,
        asio::buffer(body_buffer_, length),
        asio::transfer_exactly(length),
        [this, self](const std::error_code& ec, std::size_t) {
            if (closed_) {
                return;
            }

            if (!ec) {
                on_message(body_buffer_);
                do_read_header();
            } else {
                close_session();
            }
        });
}

void Session::on_message(const std::string& json)
{
    processor_->process_message(json, shared_from_this());
}

void Session::close_session()
{
    if (closed_) {
        return;
    }

    closed_ = true;

    if (processor_ && !disconnect_notified_) {
        disconnect_notified_ = true;
        processor_->on_disconnect(shared_from_this());
    }

    std::error_code ec;
    if (socket_.is_open()) {
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }

    spdlog::info("Session successfully closed and cleaned up.");
}

} // namespace poker::server
