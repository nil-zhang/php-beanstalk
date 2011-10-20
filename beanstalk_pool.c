/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Antony Dovgal <tony2001@phpclub.net>                        |
  |          Mikael Johansson <mikael AT synd DOT info>                  |
  +----------------------------------------------------------------------+
*/

/* $Id: beanstalk_pool.c 310129 2011-04-11 04:44:27Z hradtke $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/crc32.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_smart_str.h"
#include "beanstalk_pool.h"

//ZEND_EXTERN_MODULE_GLOBALS(beanstalk)

static unsigned int bsc_hash_crc32_init()						{ return ~0; }
static unsigned int bsc_hash_crc32_finish(unsigned int seed)	{ return ~seed; }

static unsigned int bsc_hash_crc32_combine(unsigned int seed, const void *key, unsigned int key_len) /*
	CRC32 hash {{{ */
{
	const char *p = (const char *)key, *end = p + key_len;
	while (p < end) {
		CRC32(seed, *(p++));
	}

  	return seed;
}
/* }}} */

bsc_hash_function_t bsc_hash_crc32 = {
	bsc_hash_crc32_init,
	bsc_hash_crc32_combine,
	bsc_hash_crc32_finish
};

static unsigned int bsc_hash_fnv1a_combine(unsigned int seed, const void *key, unsigned int key_len) /*
	FNV-1a hash {{{ */
{
	const char *p = (const char *)key, *end = p + key_len;
	while (p < end) {
		seed ^= (unsigned int)*(p++);
		seed *= FNV_32_PRIME;
    }

    return seed;
}
/* }}} */

static unsigned int bsc_hash_fnv1a_init()						{ return FNV_32_INIT; }
static unsigned int bsc_hash_fnv1a_finish(unsigned int seed)	{ return seed; }

bsc_hash_function_t bsc_hash_fnv1a = {
	bsc_hash_fnv1a_init,
	bsc_hash_fnv1a_combine,
	bsc_hash_fnv1a_finish
};

/*
double timeval_to_double(struct timeval tv) {
	return (double)tv.tv_sec + ((double)tv.tv_usec) / 1000000;
}

struct timeval double_to_timeval(double sec) {
	struct timeval tv;
	tv.tv_sec = (long)sec;
	tv.tv_usec = (sec - tv.tv_sec) * 1000000;
	return tv;
}
*/

void bsc_error_callback(bsc *svr, bsc_error_t error)
{
    char errorstr[BSC_ERRSTR_LEN];

    switch (error) {
        case BSC_ERROR_INTERNAL:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: recieved BSC_ERROR_INTERNAL, quitting");
            break;
        case BSC_ERROR_MEMORY:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: recieved BSC_ERROR_MEMORY, quitting");
            break;
        case BSC_ERROR_SOCKET:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "error: recieved BSC_ERROR_SOCKET, quitting ...");
            break;
        default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", error);
    }
}

bsc *bsc_server_new(
    const char *host, int host_len, unsigned short tcp_port, 
	double timeout, int retry_interval TSRMLS_DC) /* {{{ */
{
	bsc * svr;
	char errorstr[BSC_ERRSTR_LEN];
	char port[BSC_PORT_LEN];
	sprintf(port, "%d", tcp_port);
	svr = bsc_new_w_defaults(host, port, BSC_DEFAULT_TUBE, bsc_error_callback, errorstr);
    return svr;
}
/* }}} */

void bsc_server_free(bsc *svr TSRMLS_DC) /* {{{ */
{
	bsc_free(svr);
}
/* }}} */

static void bsc_pool_init_hash(bsc_pool_t *pool TSRMLS_DC) /* {{{ */
{
    bsc_hash_function_t *hash;

    switch (BSC_STANDARD_HASH) {
        case BSC_CONSISTENT_HASH:
            pool->hash = &bsc_consistent_hash;
            break;
        default:
            pool->hash = &bsc_standard_hash;
    }

    switch (BSC_HASH_CRC32) {
        case BSC_HASH_FNV1A:
            hash = &bsc_hash_fnv1a;
            break;
        default:
            hash = &bsc_hash_crc32;
    }

    pool->hash_state = pool->hash->create_state(hash);
}
/* }}} */

bsc_pool_t *bsc_pool_new(TSRMLS_D) /* {{{ */
{
    bsc_pool_t *pool = emalloc(sizeof(bsc_pool_t));
    memset(pool, 0, sizeof(*pool));

    bsc_pool_init_hash(pool TSRMLS_CC);

    return pool;
}
/* }}} */

void bsc_pool_free(bsc_pool_t *pool TSRMLS_DC) /* {{{ */
{
    int i;

    for (i=0; i<pool->num_servers; i++) {
        if (pool->servers[i] != NULL) {
			bsc_server_free(pool->servers[i]);
            pool->servers[i] = NULL;
        }
    }

    if (pool->num_servers) {
        efree(pool->servers);
    }

    pool->hash->free_state(pool->hash_state);

    efree(pool);
}
/* }}} */

void bsc_pool_add(bsc_pool_t *pool, bsc *svr, unsigned int weight) /*
    adds a server to the pool and hash strategy {{{ */
{
    pool->hash->add_server(pool->hash_state, svr, weight);
    pool->servers = erealloc(pool->servers, sizeof(*pool->servers) * (pool->num_servers + 1));
    pool->servers[pool->num_servers] = svr;

    /* store the smallest timeout for any server */
/*
    if (!pool->num_servers || timeval_to_double(bsc->timeout) < timeval_to_double(pool->timeout)) {
        pool->timeout = bsc->timeout;
    }
*/

    pool->num_servers++;
}
/* }}} */

void bsc_pool_close(bsc_pool_t *pool TSRMLS_DC) /*
    disconnects and removes all servers in the pool {{{ */
{
    if (pool->num_servers) {
        int i;

        for (i=0; i<pool->num_servers; i++) {
            bsc_server_free(pool->servers[i] TSRMLS_CC);
        }

        efree(pool->servers);
        pool->servers = NULL;
        pool->num_servers = 0;

        /* reallocate the hash strategy state */
        pool->hash->free_state(pool->hash_state);
        bsc_pool_init_hash(pool TSRMLS_CC);
    }
}
/* }}} */

bsc *bsc_pool_find(bsc_pool_t *pool, const char *key, unsigned int key_len TSRMLS_DC) /*
    maps a key to a non-failed server {{{ */
{
    bsc *svr = pool->hash->find_server(pool->hash_state, key, key_len TSRMLS_CC);

    return svr;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
