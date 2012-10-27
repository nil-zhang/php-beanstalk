/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id: header,v 1.16.2.1.2.1 2007/01/01 19:32:09 iliaa Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "php_beanstalk.h"

#ifndef ZEND_ENGINE_2
#define OnUpdateLong OnUpdateInt
#endif

/* If you declare any globals in php_beanstalk.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(beanstalk)
*/

/* True global resources - no need for thread safety here */
//static int le_beanstalk;
static int le_beanstalk_pool;
static zend_class_entry *beanstalk_pool_ce;
static zend_class_entry *beanstalk_ce;

/* {{{ beanstalk_functions[]
 *
 * Every user visible function must have an entry in beanstalk_functions[].
 */
zend_function_entry beanstalk_functions[] = {
	PHP_FE(beanstalk_add_server,	NULL)
    PHP_FE(beanstalk_use,           NULL)
    PHP_FE(beanstalk_put,           NULL)
    PHP_FE(beanstalk_watch,         NULL)
    PHP_FE(beanstalk_reserve, 		NULL)
    PHP_FE(beanstalk_bury,          NULL)
    PHP_FE(beanstalk_ignore,        NULL)
    PHP_FE(beanstalk_delete,  		NULL)
    PHP_FE(beanstalk_kick,          NULL)
    PHP_FE(beanstalk_list_tubes,    NULL)
    /*PHP_FE(beanstalk_release,      	NULL)
    PHP_FE(beanstalk_touch,         NULL)
    PHP_FE(beanstalk_peek,          NULL)
    PHP_FE(beanstalk_quit,          NULL)*/
	{NULL, NULL, NULL}	/* Must be the last line in beanstalk_functions[] */
};

zend_function_entry php_beanstalk_pool_class_functions[] = {
    PHP_NAMED_FE(addserver,   zif_beanstalk_pool_addserver,    	NULL)
    //PHP_NAMED_FE(findserver,  zif_beanstalk_pool_findserver,    NULL)
    PHP_FALIAS(use,			beanstalk_use,           NULL)
    PHP_FALIAS(put,			beanstalk_put,           NULL)
    PHP_FALIAS(watch,		beanstalk_watch,         NULL)
    PHP_FALIAS(reserve,		beanstalk_reserve,       NULL)
    PHP_FALIAS(bury,		beanstalk_bury,          NULL)
    PHP_FALIAS(ignore,		beanstalk_ignore,        NULL)
    PHP_FALIAS(delete,		beanstalk_delete,        NULL)
    PHP_FALIAS(kick,		beanstalk_kick,          NULL)
    PHP_FALIAS(list_tubes,	beanstalk_list_tubes,    NULL)
    /*PHP_FALIAS(release,		beanstalk_release,       NULL)
    PHP_FALIAS(touch,		beanstalk_touch,         NULL)
    PHP_FALIAS(peek,		beanstalk_peek,          NULL)
    PHP_FALIAS(quit,		beanstalk_quit,          NULL)*/
    {NULL, NULL, NULL}
};  

zend_function_entry php_beanstalk_class_functions[] = {
    PHP_FALIAS(addserver,   beanstalk_add_server,    NULL)
    {NULL, NULL, NULL}  
};
/* }}} */

/* {{{ beanstalk_module_entry
 */
zend_module_entry beanstalk_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"beanstalk",
	beanstalk_functions,
	PHP_MINIT(beanstalk),
	PHP_MSHUTDOWN(beanstalk),
	NULL,		/* Replace with NULL if there's nothing to do at request start */
	NULL,	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(beanstalk),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_BEANSTALK_VERSION, /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_BEANSTALK
ZEND_GET_MODULE(beanstalk)
#endif

/*
static PHP_INI_MH(OnUpdateHashStrategy)
{
    if (!strcasecmp(new_value, "standard")) {
        BEANSTALK_G(hash_strategy) = BSC_STANDARD_HASH;
    }
    else if (!strcasecmp(new_value, "consistent")) {
        BEANSTALK_G(hash_strategy) = BSC_CONSISTENT_HASH;
    }
    else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "beanstalk.hash_strategy must be in set {standard, consistent} ('%s' given)", new_value);
        return FAILURE;
    }

    return SUCCESS;
}

static PHP_INI_MH(OnUpdateHashFunction)
{
    if (!strcasecmp(new_value, "crc32")) {
        BEANSTALK_G(hash_function) = BSC_HASH_CRC32;
    }
    else if (!strcasecmp(new_value, "fnv")) {
        BEANSTALK_G(hash_function) = BSC_HASH_FNV1A;
    }
    else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "beanstalk.hash_function must be in set {crc32, fnv} ('%s' given)", new_value);
        return FAILURE;
    }

    return SUCCESS;
}
*/

/* {{{ PHP_INI
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
	STD_PHP_INI_ENTRY("beanstalk.default_port",          DEFAULT_PORT,        PHP_INI_ALL, OnUpdateLong,          default_port,   zend_beanstalk_globals,  beanstalk_globals)
	STD_PHP_INI_ENTRY("beanstalk.hash_strategy",         "consistent",   PHP_INI_ALL, OnUpdateHashStrategy,  hash_strategy,  zend_beanstalk_globals,  beanstalk_globals)
    STD_PHP_INI_ENTRY("beanstalk.hash_function",         "crc32",        PHP_INI_ALL, OnUpdateHashFunction,  hash_function,  zend_beanstalk_globals,  beanstalk_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ internal function protos */
static void _bsc_pool_list_dtor(zend_rsrc_list_entry * TSRMLS_DC);
static void php_bsc_set_failure_callback(bsc_pool_t *, zval *, zval * TSRMLS_DC);
static void php_bsc_failure_callback(bsc_pool_t *, bsc *, void * TSRMLS_DC);
/* }}} */

/* {{{ php_beanstalk_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_beanstalk_init_globals(zend_beanstalk_globals *beanstalk_globals)
{
//	BEANSTALK_G(hash_strategy)     = BSC_STANDARD_HASH;
//    BEANSTALK_G(hash_function)     = BSC_HASH_CRC32;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(beanstalk)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "BeanstalkPool", php_beanstalk_pool_class_functions);
    beanstalk_pool_ce = zend_register_internal_class(&ce TSRMLS_CC);

    INIT_CLASS_ENTRY(ce, "Beanstalk", php_beanstalk_class_functions);
    beanstalk_ce = zend_register_internal_class_ex(&ce, beanstalk_pool_ce, NULL TSRMLS_CC);

    le_beanstalk_pool = zend_register_list_destructors_ex(_bsc_pool_list_dtor, NULL, "beanstalk connection", module_number);

#ifdef ZTS
    ts_allocate_id(&beanstalk_globals_id, sizeof(zend_beanstalk_globals), (ts_allocate_ctor) php_beanstalk_init_globals, NULL);
#else
    //php_beanstalk_init_globals(&beanstalk_globals TSRMLS_CC);
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(beanstalk)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
PHP_RINIT_FUNCTION(beanstalk)
{
	return SUCCESS;
}
 */
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
PHP_RSHUTDOWN_FUNCTION(beanstalk)
{
	return SUCCESS;
}
 */
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(beanstalk)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "beanstalk support", "enabled");
	php_info_print_table_row(2, "Version", PHP_BEANSTALK_VERSION);
    php_info_print_table_row(2, "Revision", "$Revision: 310129 $");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	*/
	DISPLAY_INI_ENTRIES();
}
/* }}} */


/* ------------------
   internal functions
   ------------------ */

static void _bsc_pool_list_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
    bsc_pool_t *pool = (bsc_pool_t *)rsrc->ptr;

    if (pool->failure_callback_param) {
        zval_ptr_dtor((zval **)&pool->failure_callback_param);
        pool->failure_callback_param = NULL;
    }

    bsc_pool_free(pool TSRMLS_CC);
}

static int bsc_get_pool(zval *id, bsc_pool_t **pool TSRMLS_DC) /* {{{ */
{
    zval **connection;
    int resource_type;

    if (Z_TYPE_P(id) != IS_OBJECT || zend_hash_find(Z_OBJPROP_P(id), "connection", sizeof("connection"), (void**)&connection) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "No servers added to beanstalk connection");
        return 0;
    }

    *pool = (bsc_pool_t *) zend_list_find(Z_LVAL_PP(connection), &resource_type);
    if (!*pool || resource_type != le_beanstalk_pool) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid BeanstalkPool->connection member variable");
        return 0;
    }

    return Z_LVAL_PP(connection);
}
/* }}} */

static bsc *php_bsc_pool_addserver(
    zval *bsc_object, const char *host, int host_len, long tcp_port, long weight,
    double timeout, long retry_interval, zend_bool status, bsc_pool_t **pool_result TSRMLS_DC) /* {{{ */ 
{
    zval **connection;
    bsc_pool_t *pool;
    bsc *svr;
    int list_id, resource_type;

    if (weight < 1) { 
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "weight must be a positive integer");
        return NULL;
    }    

    /* initialize pool if need be */
    if (zend_hash_find(Z_OBJPROP_P(bsc_object), "connection", sizeof("connection"), (void **)&connection) == FAILURE) {
        pool = bsc_pool_new(TSRMLS_C);
        pool->failure_callback = &php_bsc_failure_callback;
        list_id = zend_list_insert(pool, le_beanstalk_pool);
        add_property_resource(bsc_object, "connection", list_id);
    }    
    else {
        pool = (bsc_pool_t *)zend_list_find(Z_LVAL_PP(connection), &resource_type);
        if (!pool || resource_type != le_beanstalk_pool) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unknown connection identifier");
            return NULL;
        }    
    }    

	/* lazy initialization of server struct */
    //svr = bsc_find_persistent(host, host_len, tcp_port, timeout, retry_interval TSRMLS_CC);
    svr = bsc_server_new(host, host_len, tcp_port, timeout, retry_interval TSRMLS_CC);
	if(svr)
	{
    	bsc_pool_add(pool, svr, weight);
	}

    if (pool_result != NULL) {
        *pool_result = pool;
    }

    return svr;
}
/* }}} */

static void php_bsc_failure_callback(bsc_pool_t *pool, bsc *svr, void *param TSRMLS_DC) /* {{{ */ 
{
    zval **callback;

    /* check for userspace callback*/ 
    if (param != NULL && zend_hash_find(Z_OBJPROP_P((zval *)param), "_failureCallback", sizeof("_failureCallback"), (void **)&callback) == SUCCESS && Z_TYPE_PP(callback) != IS_NULL) {
        if (IS_CALLABLE(*callback, 0, NULL)) {
            zval *retval = NULL;
            zval *host, *tcp_port, *error, *errnum;
            zval **params[2];

            params[0] = &host;
            params[1] = &tcp_port;

            MAKE_STD_ZVAL(host);
            MAKE_STD_ZVAL(tcp_port);

            ZVAL_STRING(host, svr->host, 1);
            ZVAL_STRING(tcp_port, svr->host, 1);

            call_user_function_ex(EG(function_table), NULL, *callback, &retval, 5, params, 0, NULL TSRMLS_CC);

            zval_ptr_dtor(&host);
            zval_ptr_dtor(&tcp_port);

            if (retval != NULL) {
				zval_ptr_dtor(&retval);
            }
        }
        else {
            php_bsc_set_failure_callback(pool, (zval *)param, NULL TSRMLS_CC);
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid failure callback");
        }
    }
    else {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE, "Server %s (tcp ) failed.", svr->host, svr->port);
    }
}
/* }}} */

static void php_bsc_set_failure_callback(bsc_pool_t *pool, zval *bsc_object, zval *callback TSRMLS_DC)  /* {{{ 
*/
{
    // Decrease refcount of old bsc_object
    if (pool->failure_callback_param) {
        zval_ptr_dtor((zval **)&pool->failure_callback_param);
    }

    if (callback != NULL) {
        zval *callback_tmp;
        ALLOC_ZVAL(callback_tmp);

        *callback_tmp = *callback;
        zval_copy_ctor(callback_tmp);
        INIT_PZVAL(callback_tmp);

        add_property_zval(bsc_object, "_failureCallback", callback_tmp);
        pool->failure_callback_param = bsc_object;
        zval_add_ref(&bsc_object);

        INIT_PZVAL(callback_tmp);
    }
    else {
        add_property_null(bsc_object, "_failureCallback");
        pool->failure_callback_param = NULL;
    }
}
/* }}} */

/* ----------------
   module functions
   ---------------- */

/* {{{ proto bool BeanstalkPool::addServer(string host [, int tcp_port [, [, int weight [, double timeout [, int retry_interval [, bool status] ] ])
   Adds a server to the pool */
PHP_NAMED_FUNCTION(zif_beanstalk_pool_addserver)
{
    zval *bsc_object = getThis();
    bsc *svr;

    char *host;
    int host_len;
    long tcp_port = DEFAULT_PORT, weight = 1, retry_interval = BSC_DEFAULT_RETRY;
    double timeout = BSC_DEFAULT_TIMEOUT;
    zend_bool status = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lldlb",
        &host, &host_len, &tcp_port, &weight, &timeout, &retry_interval, &status) == FAILURE) {     
        return;
    }        

    svr = php_bsc_pool_addserver(bsc_object, host, host_len, tcp_port, weight, timeout, retry_interval, status, NULL TSRMLS_CC);
    if (svr == NULL) {
        RETURN_FALSE;
    }        

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool beanstalk_add_server(string host [, int port [, int weight [, double timeout [, int retry_interval [, bool status [, callback failure_callback ] ] ] ] ] ])
   Adds a server to the pool.*/
PHP_FUNCTION(beanstalk_add_server)
{
    zval *bsc_object = getThis(), *failure_callback = NULL;
    bsc_pool_t *pool;
	bsc	*svr;

    char *host;
    int host_len;
    int tcp_port = DEFAULT_PORT;
	int weight = 1; 
	int retry_interval = BSC_DEFAULT_RETRY;
    double timeout = BSC_DEFAULT_TIMEOUT;
	zend_bool status = 1;

    if (bsc_object) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|lldlbz",
            &host, &host_len, &tcp_port, &weight, &timeout, &retry_interval, &status, &failure_callback) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os|lldlbz", &bsc_object, beanstalk_ce,
            &host, &host_len, &tcp_port, &weight, &timeout, &retry_interval, &status, &failure_callback) == FAILURE) {
            return;
        }
    }

    if (failure_callback != NULL && Z_TYPE_P(failure_callback) != IS_NULL) {
        if (!IS_CALLABLE(failure_callback, 0, NULL)) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Invalid failure callback");
            RETURN_FALSE;
        }
    }

    svr = php_bsc_pool_addserver(bsc_object, host, host_len, tcp_port, weight, timeout, retry_interval, status, &pool TSRMLS_CC);
    if (svr == NULL) {
		RETURN_FALSE;
    }

    if (failure_callback != NULL && Z_TYPE_P(failure_callback) != IS_NULL) {
        php_bsc_set_failure_callback(pool, bsc_object, failure_callback TSRMLS_CC);
    }

    RETURN_TRUE;
}
/* }}} */

fd_set readset, writeset;
static bsc_error_t bsc_error;
int cmd_poll(bsc *svr, fd_set *readset, fd_set *writeset)
{
    FD_SET(svr->fd, readset);
    FD_SET(svr->fd, writeset);
    if (AQ_EMPTY(svr->outq)) {
        if ( select(svr->fd+1, readset, NULL, NULL, NULL) < 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: select()");
            return false;
        }
        if (FD_ISSET(svr->fd, readset))
            bsc_read(svr);
    }
    else {
        if ( select(svr->fd+1, readset, writeset, NULL, NULL) < 0) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: select()");
            return false;
        }
        if (FD_ISSET(svr->fd, readset))
            bsc_read(svr);
        if (FD_ISSET(svr->fd, writeset))
            bsc_write(svr);
    }
}

static int use_flag = false;
static struct bsc_use_info *g_use_info = NULL;
void use_callback(bsc *svr, struct bsc_use_info *info)
{
	g_use_info = info;
	use_flag = true;
}

bool cmd_use(bsc *svr, char *tube, int tube_len)
{

    bsc_error = bsc_use(svr, use_callback, NULL, tube);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
		return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!use_flag)
    {   
        cmd_poll(svr, &readset, &writeset);
    }

	return true;
}

/* {{{ proto bool beanstalk_use(object beanstalk, string tube)
   */
PHP_FUNCTION(beanstalk_use)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube;
    int tube_len;
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &bsc_object, beanstalk_pool_ce, &tube, &tube_len) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &tube, &tube_len) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

    svr = bsc_pool_find(pool, tube, tube_len);
    if(!svr){
        RETURN_FALSE;
    }

	if(cmd_use(svr, tube, tube_len) && g_use_info){
		if(BSC_USE_RES_USING == g_use_info->response.code){
			g_use_info = NULL;
			use_flag = false;
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}

static int put_flag = false;
static struct bsc_put_info *g_put_info = NULL;
void put_callback(bsc *svr, struct bsc_put_info *info)
{
	g_put_info = info;
    put_flag = true;
}

bool cmd_put(bsc *svr, char *value, int value_len)
{
    bsc_error = bsc_put(svr, put_callback, NULL, BSC_DEFAULT_PRORITY, 
					BSC_DEFAULT_DELAY, BSC_DEFAULT_TTR, value_len, value, false);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!put_flag)
	{
        cmd_poll(svr, &readset, &writeset);
	}

	return true;
}

/* {{{ proto bool beanstalk_put(object beanstalk, string tube [, mixed var [, int flag [, int exptime ] ] ])
   Sets the value of an item. Item may exist or not */
PHP_FUNCTION(beanstalk_put)
{
	bsc_pool_t *pool;
	bsc * svr;
	char *tube, *value;
	int tube_len, value_len;
    zval *bsc_object = getThis();
    long flags = 0, exptime = 0;

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Oss|ll", &bsc_object, beanstalk_pool_ce, &tube, &tube_len, &value, &value_len, &flags, &exptime) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss|ll", &tube, &tube_len, &value, &value_len, &flags, &exptime) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

	svr = bsc_pool_find(pool, tube, tube_len);
	if(!svr) {
		RETURN_FALSE;
	}

	if(cmd_put(svr, value, value_len) && g_put_info) {
		if(BSC_PUT_RES_INSERTED == g_put_info->response.code) {
			g_put_info = NULL;
			put_flag = false;
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}
/* }}} */


static int wtch_flag = false;
static struct bsc_watch_info *g_wtch_info = NULL;
void wtch_callback(bsc *svr, struct bsc_watch_info *info)
{   
    g_wtch_info = info;
    wtch_flag = true;
}

bool cmd_watch(bsc *svr, char *tube, int tube_len)
{
    bsc_error = bsc_watch(svr, wtch_callback, NULL, tube);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!wtch_flag)
    {   
        cmd_poll(svr, &readset, &writeset);
    }

	return true;
}

/* {{{ proto bool beanstalk_watch(object beanstalk, string tube)
   */
PHP_FUNCTION(beanstalk_watch)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube;
    int tube_len;
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &bsc_object, beanstalk_pool_ce, &tube, &tube_len) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &tube, &tube_len) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

    svr = bsc_pool_find(pool, tube, tube_len);
    if(!svr)
    {
        RETURN_FALSE;
    }

	if(cmd_watch(svr, tube, tube_len) && g_wtch_info) {
		if(BSC_RES_WATCHING == g_wtch_info->response.code)
		{
			g_wtch_info = NULL;
			wtch_flag = false;
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}

static int ign_flag = false;
static struct bsc_ignore_info *g_ign_info = NULL;
void ign_callback(bsc *svr, struct bsc_ignore_info *info)
{   
    g_ign_info = info;
    ign_flag = true;
}

bool cmd_ignore(bsc *svr, char *tube, int tube_len)
{
    bsc_error = bsc_ignore(svr, ign_callback, NULL, tube);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!ign_flag)
    {   
        cmd_poll(svr, &readset, &writeset);
    }

	return true;
}

/* {{{ proto bool beanstalk_ignore(object beanstalk, string tube)
   */
PHP_FUNCTION(beanstalk_ignore)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube;
    int tube_len;
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os", &bsc_object, beanstalk_pool_ce, &tube, &tube_len) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &tube, &tube_len) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

    svr = bsc_pool_find(pool, tube, tube_len);
    if(!svr)
    {
        RETURN_FALSE;
    }

	if(cmd_ignore(svr, tube, tube_len) && g_ign_info)
	{
		if(BSC_RES_WATCHING == g_ign_info->response.code)
		{
			g_ign_info = NULL;
			ign_flag = false;
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}

static int rsv_flag = false;
static struct bsc_reserve_info *g_rsv_info = NULL;
void rsv_callback(bsc *svr, struct bsc_reserve_info *info)
{
	g_rsv_info = info;
    rsv_flag = true;
}

bool cmd_reserve(bsc *svr, int timeout)
{
	bsc_error = bsc_reserve(svr, rsv_callback, NULL, timeout);
    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
	}

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!rsv_flag) {
        cmd_poll(svr, &readset, &writeset); 
    }

	return true;
}

/* {{{ proto bool beanstalk_reserve(object beanstalk, char *tube, int tube_len, int timeout ])
   Returns value of existing item or false */
PHP_FUNCTION(beanstalk_reserve)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube, *value;
    int tube_len, value_len;
	int timeout;
    zval *bsc_object = getThis();
    long flags = 0, exptime = 0;

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Os|l", &bsc_object, beanstalk_pool_ce, &tube, &tube_len, &timeout) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &tube, &tube_len, &timeout) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

	svr = bsc_pool_find(pool, tube, tube_len);
	if(!svr)
	{
		RETURN_FALSE;
	}

	array_init(return_value);

	if(cmd_reserve(svr, timeout) && g_rsv_info)
	{
		if(BSC_RESERVE_RES_RESERVED == g_rsv_info->response.code)
		{
			add_assoc_long(return_value, "id", g_rsv_info->response.id);
			add_assoc_stringl(return_value, "data", g_rsv_info->response.data, g_rsv_info->response.bytes, 1);
		}
	}
	g_rsv_info = NULL;
	rsv_flag = false;
}

static int bury_flag = false;
static struct bsc_bury_info *g_bury_info = NULL;
void bury_callback(bsc *svr, struct bsc_bury_info *info)
{   
    g_bury_info = info;
    bury_flag = true;
}

bool cmd_bury(bsc *svr, int job_id)
{
    bsc_error = bsc_bury(svr, bury_callback, NULL, job_id, BSC_DEFAULT_PRORITY);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!bury_flag)
    {   
        cmd_poll(svr, &readset, &writeset);
    }

	return true;
}

/* {{{ proto bool beanstalk_bury(object beanstalk, string tube, int job_id)
   */
PHP_FUNCTION(beanstalk_bury)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube;
    int tube_len;
	int job_id;
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ols", &bsc_object, beanstalk_pool_ce, &job_id, &tube, &tube_len) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &job_id, &tube, &tube_len) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

    svr = bsc_pool_find(pool, tube, tube_len);
    if(!svr)
    {
        RETURN_FALSE;
    }

	if(cmd_bury(svr, job_id) && g_bury_info) {
		if(BSC_RES_BURIED == g_bury_info->response.code){
			g_bury_info = NULL;
			bury_flag = false;
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}

static int kick_flag = false;
static struct bsc_kick_info *g_kick_info = NULL;
void kick_callback(bsc *svr, struct bsc_kick_info *info)
{   
    g_kick_info = info;
    kick_flag = true;
}

bool cmd_kick(bsc *svr, int bound)
{
    bsc_error = bsc_kick(svr, kick_callback, NULL, bound);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!kick_flag)
    {   
        cmd_poll(svr, &readset, &writeset);
    }

	return true;
}

/* {{{ proto bool beanstalk_kick(object beanstalk, string tube, int bound)
   */
PHP_FUNCTION(beanstalk_kick)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube;
    int tube_len;
	int bound;
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ols", &bsc_object, beanstalk_pool_ce, &bound, &tube, &tube_len) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &bound, &tube, &tube_len) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

    svr = bsc_pool_find(pool, tube, tube_len);
    if(!svr)
    {
        RETURN_FALSE;
    }

	if(cmd_kick(svr, bound) && g_kick_info) {
		if(BSC_KICK_RES_KICKED == g_kick_info->response.code){
			g_kick_info = NULL;
			kick_flag = false;
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}

static int dlt_flag = false;
static struct bsc_delete_info *g_dlt_info = NULL;
void dlt_callback(bsc *svr, struct bsc_delete_info *info)
{   
    g_dlt_info = info;
    dlt_flag = true;
}

bool cmd_delete(bsc *svr, int job_id)
{
    bsc_error = bsc_delete(svr, dlt_callback, NULL, job_id);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!dlt_flag)
    {   
        cmd_poll(svr, &readset, &writeset);
    }

	return true;
}

/* {{{ proto bool beanstalk_delete(object beanstalk, string tube, int job_id)
   */
PHP_FUNCTION(beanstalk_delete)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube;
    int tube_len;
	int job_id;
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Ols", &bsc_object, beanstalk_pool_ce, &job_id, &tube, &tube_len) == FAILURE) {
            return;
        }
    }
    else {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &job_id, &tube, &tube_len) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

    svr = bsc_pool_find(pool, tube, tube_len);
    if(!svr)
    {
        RETURN_FALSE;
    }

	if(cmd_delete(svr, job_id) && g_dlt_info){
		if(BSC_DELETE_RES_DELETED == g_dlt_info->response.code){
			g_dlt_info = NULL;
			dlt_flag = false;
			RETURN_TRUE;
		}
	}

	RETURN_FALSE;
}

static int lst_flag = false;
static struct bsc_list_tubes_info *g_lst_info = NULL;
static char *lst_callback(bsc *svr, struct bsc_list_tubes_info *info)
{
    g_lst_info = info;
    lst_flag = true;
}

bool cmd_list_tubes(bsc *svr)
{
    bsc_error = bsc_list_tubes(svr, lst_callback, NULL);

    if (bsc_error != BSC_ERROR_NONE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "critical error: got unknown error (%d)\n", bsc_error);
        return false;
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!lst_flag)
    {   
        cmd_poll(svr, &readset, &writeset);
    }

	return true;
}

/* {{{ proto bool beanstalk_list_tubes(object beanstalk)
   */
PHP_FUNCTION(beanstalk_list_tubes)
{
    bsc_pool_t *pool;
    bsc * svr;
    char *tube;
    int tube_len;
	int i;
	int j = 0;
	char key[BSC_MAX_TUBE_NAME + 56];
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &bsc_object, beanstalk_pool_ce) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC) || !pool->num_servers) {
        RETURN_FALSE;
    }

    RETVAL_NULL();

	array_init(return_value);

	for (i = 0; i < pool->num_servers; i++)
	{
		if(cmd_list_tubes(pool->servers[i]) && g_lst_info){
            while(g_lst_info->response.tubes[j][0] != '\0')
            {
                sprintf(key, "srv%d_%d", i, j);
                add_assoc_string(return_value, key, g_lst_info->response.tubes[j], 1);
                j++;
            }   
            j = 0;
		}
		g_lst_info = NULL;
		lst_flag = false;
	}
}

/* {{{ proto bool beanstalk_quit( object beanstalk )
   Closes connection to beanstalkd */
PHP_FUNCTION(beanstalk_quit)
{
    bsc_pool_t *pool;
    zval *bsc_object = getThis();

    if (bsc_object == NULL) {
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &bsc_object, beanstalk_pool_ce) == FAILURE) {
            return;
        }
    }

    if (!bsc_get_pool(bsc_object, &pool TSRMLS_CC)) {
        RETURN_FALSE;
    }

    bsc_pool_close(pool TSRMLS_CC);
    RETURN_TRUE;
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

