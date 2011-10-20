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

/* $Id: beanstalk_pool.h 310129 2011-04-11 04:44:27Z hradtke $ */

#ifndef BEANSTALK_POOL_H
#define BEANSTALK_POOL_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdint.h>
#include <string.h>

#include "php.h"
#include "ext/standard/php_smart_str_public.h"
#include "beanstalkclient.h"

#ifndef ZSTR
#define ZSTR
#define ZSTR_VAL(v) v
#define zstr char *
#else
#define ZSTR_VAL(v) (v).s
#endif

/*
 * Mac OS X has no MSG_NOSIGNAL but >= 10.2 comes with SO_NOSIGPIPE which is a setsockopt() option
 * and not a send() parameter as MSG_NOSIGNAL. OpenBSD has none of the options so we need to ignore 
 * SIGPIPE events
 */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif /*MSG_NOSIGNAL*/

#define BSC_MAX_KEY_LEN         250
#define BSC_PORT_LEN         	10

#define BSC_OK                  0

#define BSC_STANDARD_HASH       1
#define BSC_CONSISTENT_HASH     2
#define BSC_HASH_CRC32          1           /* CRC32 hash function */
#define BSC_HASH_FNV1A          2           /* FNV-1a hash function */

#define BSC_CONSISTENT_POINTS   160         /* points per server */
#define BSC_CONSISTENT_BUCKETS  1024        /* number of precomputed buckets, should be power of 2 */

typedef struct bsc_pool bsc_pool_t;

/* hashing strategy */
typedef unsigned int (*bsc_hash_function_init)();
typedef unsigned int (*bsc_hash_function_combine)(unsigned int seed, const void *key, unsigned int key_len);
typedef unsigned int (*bsc_hash_function_finish)(unsigned int seed);

typedef struct bsc_hash_function {
    bsc_hash_function_init      init;
    bsc_hash_function_combine   combine;
    bsc_hash_function_finish    finish;
} bsc_hash_function_t;

extern bsc_hash_function_t bsc_hash_crc32;
extern bsc_hash_function_t bsc_hash_fnv1a;

#define bsc_hash(hash, key, key_len) ((hash)->finish((hash)->combine((hash)->init(), (key), (key_len))))

typedef void * (*bsc_hash_create_state)(bsc_hash_function_t *);
typedef void (*bsc_hash_free_state)(void *state);
typedef bsc * (*bsc_hash_find_server)(void *state, const char *key, unsigned int key_len TSRMLS_DC);
typedef void (*bsc_hash_add_server)(void *state, bsc *mmc, unsigned int weight);

typedef struct bsc_hash_strategy {
    bsc_hash_create_state   create_state;
    bsc_hash_free_state     free_state;
    bsc_hash_find_server    find_server;
    bsc_hash_add_server     add_server;
} bsc_hash_strategy_t;

extern bsc_hash_strategy_t bsc_standard_hash;
extern bsc_hash_strategy_t bsc_consistent_hash;

/* 32 bit magic FNV-1a prime and init */
#define FNV_32_PRIME    0x01000193
#define FNV_32_INIT     0x811c9dc5

/* failure callback prototype */
typedef void (*bsc_failure_callback)(bsc_pool_t *pool, bsc *svr, void *param TSRMLS_DC);

/* server pool */
struct bsc_pool {
    bsc                   **servers;
    int                     num_servers;
    bsc_hash_strategy_t     *hash;                      /* hash strategy */
    void                    *hash_state;                /* strategy specific state */
    struct timeval          timeout;                    /* smallest timeout for any of the servers */
    bsc_failure_callback    failure_callback;           /* receives notification when a server fails */
    void                    *failure_callback_param;
};

/* pool functions */
bsc_pool_t *bsc_pool_new(TSRMLS_D);
void bsc_pool_free(bsc_pool_t * TSRMLS_DC);
void bsc_pool_add(bsc_pool_t *, bsc *, unsigned int);
bsc *bsc_server_new(const char *host, int host_len, unsigned short tcp_port, double timeout, int retry_interval TSRMLS_DC);
void bsc_server_free(bsc * TSRMLS_DC);
void bsc_pool_close(bsc_pool_t * TSRMLS_DC);
bsc *bsc_pool_find(bsc_pool_t *, const char *, unsigned int TSRMLS_DC);

/* globals */
/*
ZEND_BEGIN_MODULE_GLOBALS(beanstalk)
    long default_port;
    long hash_strategy;
    long hash_function;
ZEND_END_MODULE_GLOBALS(beanstalk)
*/
/*
#ifdef ZTS
#define BEANSTALK_G(v) TSRMG(beanstalk_globals_id, zend_beanstalk_globals *, v)
#else
#define BEANSTALK_G(v) (beanstalk_globals.v)
#endif
*/

#endif  /* BEANSTALK_POOL_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
