
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "ngx_http_test_module.h"

static char *ngx_http_print_hello(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_print_var(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_test_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_test_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

typedef struct {
    ngx_str_t value;

    /* compiled script */
    ngx_array_t *value_lengths;
    ngx_array_t *value_values;
} ngx_http_test_loc_conf_t;

static ngx_command_t ngx_http_test_commands[] = {
    {
        ngx_string("print_hello_world"),
        NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
        ngx_http_print_hello,
        0,
        0,
        NULL
    },
    {
        ngx_string("print_var"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_print_var,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },
    ngx_null_command
};

static ngx_http_module_t ngx_http_test_module_ctx = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_test_create_loc_conf,
    ngx_http_test_merge_loc_conf
};

ngx_module_t ngx_http_test_module = {
    NGX_MODULE_V1,
    &ngx_http_test_module_ctx,
    ngx_http_test_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static void *ngx_http_test_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_test_loc_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_test_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->value.len = strlen("default");
    conf->value.data = (u_char*)"default";

    return conf;
}

static char *
ngx_http_test_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_test_loc_conf_t *prev = parent;
    ngx_http_test_loc_conf_t *conf = child;

    ngx_conf_merge_str_value(conf->value, prev->value, "default");

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_print_hello_handler(ngx_http_request_t *r)
{
    u_char *ngx_hello_world = (u_char *) "Hello World!";
    size_t sz = strlen((char*)ngx_hello_world);

    r->headers_out.content_type.len = strlen("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = sz;
    ngx_http_send_header(r);

    ngx_buf_t *b;
    ngx_chain_t *out;

    b = ngx_calloc_buf(r->pool);

    out = ngx_alloc_chain_link(r->pool);

    out->buf = b;
    out->next = NULL;

    b->pos = ngx_hello_world;
    b->last = ngx_hello_world + sz;
    b->memory = 1;
    b->last_buf = 1;

    return ngx_http_output_filter(r, out);
}

static char *ngx_http_print_hello(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_print_hello_handler;
    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_print_var_handler(ngx_http_request_t *r)
{
    ngx_str_t data;
    ngx_http_test_loc_conf_t *lcf = ngx_http_get_module_loc_conf(r, ngx_http_test_module);

    if (ngx_http_script_run(r, &data, lcf->value_lengths->elts, 0,
                            lcf->value_values->elts)
        == NULL) {
        return NGX_ERROR;
    }

    r->headers_out.content_type.len = strlen("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = data.len;
    ngx_http_send_header(r);

    ngx_buf_t *b;
    ngx_chain_t *out;

    b = ngx_calloc_buf(r->pool);

    out = ngx_alloc_chain_link(r->pool);

    out->buf = b;
    out->next = NULL;

    b->pos = data.data;
    b->last = data.data + data.len;
    b->memory = 1;
    b->last_buf = 1;

    return ngx_http_output_filter(r, out);
}

static char *ngx_http_print_var(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    ngx_http_test_loc_conf_t *lcf = conf;
    ngx_http_script_compile_t sc;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_print_var_handler;

    ngx_str_t *value;
    value = cf->args->elts;
    lcf->value = value[1];

    /* compile */
    ngx_memzero(&sc, sizeof(ngx_http_script_compile_t));
    sc.cf = cf;
    sc.source = &lcf->value;
    sc.lengths = &lcf->value_lengths;
    sc.values = &lcf->value_values;
    /* TODO: optimize if == 0 */
    sc.variables = ngx_http_script_variables_count(&lcf->value);
    /*sc.complete_lengths = 1;*/
    /*sc.complete_values = 1;*/
    if (ngx_http_script_compile(&sc) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}
