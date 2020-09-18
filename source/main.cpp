#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <uv.h>


class HttpServer
{
private:
    uv_loop_t* loop;
    uv_tcp_t server;
    struct sockaddr_in addr;
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

    static void on_new_connection(uv_stream_t* server, int status) {
        uv_loop_t* loop = uv_default_loop();
        printf("New connection \n");
        if (status < 0) {
            fprintf(stderr, "New connection error %s\n", uv_strerror(status));
            return;
        }

        uv_tcp_t* client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
        uv_tcp_init(loop, client);
        if (uv_accept(server, (uv_stream_t*)client) == 0) {
            uv_read_start((uv_stream_t*)client, alloc_buffer, echo_read);
        }
        else
        {
            uv_close((uv_handle_t*)client, NULL);
        }
    }

    static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
        buf->base = (char*)malloc(suggested_size);
        buf->len = suggested_size;
    }

    static void echo_write(uv_write_t* req, int status) {
        if (status) {
            fprintf(stderr, "Write error %s\n", uv_strerror(status));
        }
        free(req);
    }

    static void echo_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf) {
        if (nread < 0) {
            if (nread != UV_EOF) {
                fprintf(stderr, "Read error %s\n", uv_err_name(nread));
                uv_close((uv_handle_t*)client, NULL);
            }
        }
        else if (nread > 0) {
            uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
            std::string ret_str = "HTTP/1.1 200 OK\r\n" \
                "Content-Type: text/plain\r\n" \
                "Content-Length: 14\r\n" \
                "\r\n"\
                "Hello, World!\n";
            char* ret = (char*)malloc(ret_str.length());
            memcpy(ret, ret_str.c_str(), ret_str.length());
            // uv_buf_t wrbuf = uv_buf_init(buf->base, nread);
            uv_buf_t wrbuf = uv_buf_init(ret, ret_str.length());
            uv_write(req, client, &wrbuf, 1, echo_write);
        }

        if (buf->base) {
            free(buf->base);
        }
    }

};

int main() {
    uv_loop_t* loop = uv_default_loop();
    HttpServer server("0.0.0.0", 7000);
    server.listen();
    return uv_run(loop, UV_RUN_DEFAULT);
}