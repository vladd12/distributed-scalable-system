# Library roadmap

## TODO Features:
- server: don't shutdown connection after performing request if client passed "Keep-Alive" and "Timeout" headers;
- client: queue with requests;
- client: using timeouts in connection class;
- ~~simplify code: http_session and client_session must be one class;~~ DONE
- and much more good things...

## P1 major priority
- `http::server::do_accept` - busy-loop in case of fatal error:
  - If the `m_acceptor` is closed/broken (for example, the port is busy after a restart, or resources are running out), `async_accept` will return an error instantly, and do_accept will be called indefinitely → 100% CPU.
- Need implement graceful shutdown:
  - Neither `http::server` nor `http::client` have `stop()`/`close()`. The only way to stop it is Ctrl+C. It's bad for the daemon (you can't save the state, wait for the connections to complete).
  - Fix: add `void http::server::stop()` -> `m_acceptor.close()` and `void http::client::close()` -> `m_session->shutdown()`.
- `request::parse` ignores pipelining:
  - If there is another request in the buffer after one (HTTP/1.1 pipelining), both will be glued into one body. Right now, the server is always `Connection: close`, so it doesn't fire, but when you add keep-alive, it breaks.
  - Fix: parse by `Content-Length`. Read exactly `min(header_size + content_length, buffer.size())`.
- `request::serialize` — does not validate the absence of a `Host`:
  - In HTTP/1.1 `Host` is required for all requests. If the user forgot, serialize will send it silently without a Host. `http::client::async_request` adds it, but `request::serialize` is a public API that can be bypassed.
  - Fixed: throw error if there is no `Host`.

## P2 minor priority
- `response::text` / `response::json` — take `std::string_view`, but copy it to `std::string`:
  - OK, but if text is a temporary string passed as `std::string_view` it's UB. `std::string_view` does not own it. The API is dangerous.
  - Fix: accept `std::string` by value (with move).
- `headers_t::content_length` — does not support duplicate headers:
  - If two `Content-Lengths` (attacking vector) arrive, `headers_t` (`std::unordered_map`) will leave the last one. According to RFC 7230, this must be an error. Now it's being silently overwritten.
  - Fix: when duplicating `Content-Length`/`Host` — `throw core::http_error`.
- `response::serialize` — `200` with empty body does not setup `Content-Length: 0`
  - If `body.empty()` and there is no `Content-Length`, nothing is written. For `204 No Content` is correct (no body, no `Content-Length`). But for `200 OK` with an empty body, it's wrong: the client must know that there is no body.
  - According to `RFC 7230 §3.3.2` in the absence of `Content-Length` and `Transfer-Encoding`, the client reads the body before EOF. This means that the client will wait for close, but the server will not close in keep—alive.
