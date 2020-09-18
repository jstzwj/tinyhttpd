#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <vector>
#include <utility>
#include <string>
#include <memory>
#include <uv.h>



inline std::string getHttpHeader(std::uint16_t status_code, std::string status, std::vector<std::pair<std::string, std::string>> headers)
{
    std::string response;
    response += "HTTP/1.1 " + std::to_string(status_code) + " " + status + "\r\n";
    for(const auto& each_pair: headers)
    {
        response += each_pair.first + ": " + each_pair.second + "\r\n";
    }
    return response;
}



class HttpServer
{
private:
    uv_loop_t* loop;
    uv_tcp_t server;
    struct sockaddr_in addr;

    inline static std::map<int, std::string> status_code{
        {200, "OK"},
        {404, "Not Found"}
    };
public:
    HttpServer(const char* ip, std::uint16_t port)
    {
        loop = uv_default_loop();
        uv_tcp_init(loop, &server);
        uv_ip4_addr(ip, port, &addr);
        uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);
    }
    int listen()
    {
        int r = uv_listen((uv_stream_t*)&server, 128, on_new_connection);
        if (r) {
            fprintf(stderr, "Listen error %s\n", uv_strerror(r));
            return 1;
        }
    }

    static void free_handle(uv_handle_t* handle) {
        delete handle;
    }

    static void on_new_connection(uv_stream_t* server, int status) {
        uv_loop_t* loop = uv_default_loop();
        if (status < 0) {
            fprintf(stderr, "New connection error %s\n", uv_strerror(status));
            return;
        }

        uv_tcp_t* client = new uv_tcp_t;
        uv_tcp_init(loop, client);
        if (uv_accept(server, (uv_stream_t*)client) == 0) {
            uv_read_start((uv_stream_t*)client, alloc_buffer, read_cb);
        }
        else
        {
            uv_close((uv_handle_t*)client, free_handle);
        }
    }

    static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = new char[suggested_size];
        buf->len = suggested_size;
    }

    static void write_cb(uv_write_t* req, int status) {
        if (status) {
            fprintf(stderr, "Write error %s\n", uv_strerror(status));
        }
        delete[] req->data;
        delete req;
    }

    static void read_cb(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
        if (nread < 0) {
            if (nread != UV_EOF) {
                fprintf(stderr, "Read error %s\n", uv_err_name(nread));
                uv_close((uv_handle_t*)client, free_handle);
            }
        }
        else if (nread > 0) {
            uv_write_t* req = new uv_write_t;
            std::string body = "Hello, World!\n";
            std::string header = getHttpHeader(200, status_code[200],
                {
                    {"Content-Type", "text/plain"},
                    {"Content-Length", std::to_string(body.length())}
                }
            );
            std::string response = header + "\r\n" + body;
            char* ret = new char[response.length()];
            memcpy(ret, response.c_str(), response.length());
            // uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
            uv_buf_t wrbuf = uv_buf_init(ret, response.length());
            req->data = ret;
            uv_write(req, client, &wrbuf, 1, write_cb);
        }

        if (buf->base) {
            delete buf->base;
        }
    }

};

int main() {
    uv_loop_t* loop = uv_default_loop();
    HttpServer server("0.0.0.0", 7000);
    server.listen();
    return uv_run(loop, UV_RUN_DEFAULT);
}