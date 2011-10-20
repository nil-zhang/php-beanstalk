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

#ifndef PHP_BEANSTALK_H
#define PHP_BEANSTALK_H

extern zend_module_entry beanstalk_module_entry;
#define phpext_beanstalk_ptr &beanstalk_module_entry

#ifdef PHP_WIN32
#define PHP_BEANSTALK_API __declspec(dllexport)
#else
#define PHP_BEANSTALK_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include "beanstalk_pool.h"

PHP_MINIT_FUNCTION(beanstalk);
PHP_MSHUTDOWN_FUNCTION(beanstalk);
//PHP_RINIT_FUNCTION(beanstalk);
//PHP_RSHUTDOWN_FUNCTION(beanstalk);
PHP_MINFO_FUNCTION(beanstalk);

PHP_NAMED_FUNCTION(zif_beanstalk_pool_addserver);
PHP_NAMED_FUNCTION(zif_beanstalk_pool_findserver);

PHP_FUNCTION(beanstalk_add_server);
PHP_FUNCTION(beanstalk_use);
PHP_FUNCTION(beanstalk_put);
PHP_FUNCTION(beanstalk_reserve);
PHP_FUNCTION(beanstalk_delete);
PHP_FUNCTION(beanstalk_release);
PHP_FUNCTION(beanstalk_bury);
PHP_FUNCTION(beanstalk_touch);
PHP_FUNCTION(beanstalk_watch);
PHP_FUNCTION(beanstalk_ignore);
PHP_FUNCTION(beanstalk_peek);
PHP_FUNCTION(beanstalk_kick);
PHP_FUNCTION(beanstalk_list_tubes);
PHP_FUNCTION(beanstalk_quit);

#define PHP_BEANSTALK_VERSION "0.0.1"

#define BSC_DEFAULT_TIMEOUT 0               /* seconds */
#define BSC_DEFAULT_RETRY 15                /* retry failed server after x seconds */

#define BSC_DEFAULT_PRORITY	1024
#define BSC_DEFAULT_DELAY	0
#define BSC_DEFAULT_TTR		60
#define DEFAULT_PORT		60

#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION >= 3)
#   define IS_CALLABLE(cb_zv, flags, cb_sp) zend_is_callable((cb_zv), (flags), (cb_sp) TSRMLS_CC)
#else
#   define IS_CALLABLE(cb_zv, flags, cb_sp) zend_is_callable((cb_zv), (flags), (cb_sp))
#endif

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     
ZEND_BEGIN_MODULE_GLOBALS(beanstalk)
	long default_port;
    long hash_strategy;
    long hash_function;
ZEND_END_MODULE_GLOBALS(beanstalk)
*/

/* In every utility function you add that needs to use variables 
   in php_beanstalk_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as BEANSTALK_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

/*
*/
#ifdef ZTS
#define BEANSTALK_G(v) TSRMG(beanstalk_globals_id, zend_beanstalk_globals *, v)
#else
#define BEANSTALK_G(v) (beanstalk_globals.v)
#endif

#endif	/* PHP_BEANSTALK_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */


