#pragma once

void PerProc_init();
void pp_on_connect(uv_connect_t *conn, int status);
void pp_alloc_cb(uv_handle_t* handle, size_t size, uv_buf_t* buf);
void pp_on_close(uv_handle_t* handle);
void pp_on_read(uv_stream_t* tcp, ssize_t nread, const uv_buf_t *buf);
void pp_on_write(uv_write_t* req, int status);
