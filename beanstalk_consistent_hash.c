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

/* $Id: beanstalk_consistent_hash.c 310129 2011-04-11 04:44:27Z hradtke $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "php.h"
#include "beanstalk_pool.h"

//ZEND_EXTERN_MODULE_GLOBALS(beanstalk)

typedef struct bsc_consistent_point {
	bsc					*server;
	unsigned int			point;
} bsc_consistent_point_t;

typedef struct bsc_consistent_state {
	int						num_servers;
	bsc_consistent_point_t	*points;
	int						num_points;
	bsc					*buckets[BSC_CONSISTENT_BUCKETS];
	int						buckets_populated;
	bsc_hash_function_t		*hash;
} bsc_consistent_state_t;

void *bsc_consistent_create_state(bsc_hash_function_t *hash) /* {{{ */
{
	bsc_consistent_state_t *state = emalloc(sizeof(bsc_consistent_state_t));
	memset(state, 0, sizeof(bsc_consistent_state_t));
	state->hash = hash;
	return state;
}
/* }}} */

void bsc_consistent_free_state(void *s) /* {{{ */
{
	bsc_consistent_state_t *state = s;
	if (state != NULL) {
		if (state->points != NULL) {
			efree(state->points);
		}
		efree(state);
	}
}
/* }}} */

static int bsc_consistent_compare(const void *a, const void *b) /* {{{ */
{
	if (((bsc_consistent_point_t *)a)->point < ((bsc_consistent_point_t *)b)->point) {
		return -1;
	}
	if (((bsc_consistent_point_t *)a)->point > ((bsc_consistent_point_t *)b)->point) {
		return 1;
	}
	return 0;
}
/* }}} */

static bsc *bsc_consistent_find(bsc_consistent_state_t *state, unsigned int point) /* {{{ */
{
	int lo = 0, hi = state->num_points - 1, mid;

	while (1) {
		/* point is outside interval or lo >= hi, wrap-around */
		if (point <= state->points[lo].point || point > state->points[hi].point) {
			return state->points[lo].server;
		}

		/* test middle point */
		mid = lo + (hi - lo) / 2;

		/* perfect match */
		if (point <= state->points[mid].point && point > (mid ? state->points[mid-1].point : 0)) {
			return state->points[mid].server;
		}

		/* too low, go up */
		if (state->points[mid].point < point) {
			lo = mid + 1;
		}
		else {
			hi = mid - 1;
		}
	}
}
/* }}} */

static void bsc_consistent_populate_buckets(bsc_consistent_state_t *state) /* {{{ */
{
	unsigned int i, step = 0xffffffff / BSC_CONSISTENT_BUCKETS;

	qsort((void *)state->points, state->num_points, sizeof(bsc_consistent_point_t), bsc_consistent_compare);
	for (i=0; i<BSC_CONSISTENT_BUCKETS; i++) {
		state->buckets[i] = bsc_consistent_find(state, step * i);
	}

	state->buckets_populated = 1;
}
/* }}} */

bsc *bsc_consistent_find_server(void *s, const char *key, unsigned int key_len TSRMLS_DC) /* {{{ */
{
	bsc_consistent_state_t *state = s;

	if (state->num_servers > 1) {
		unsigned int hash;

		if (!state->buckets_populated) {
			bsc_consistent_populate_buckets(state);
		}
		
		hash = bsc_hash(state->hash, key, key_len);
		return state->buckets[hash % BSC_CONSISTENT_BUCKETS];
	}

	return state->points[0].server;
}
/* }}} */

void bsc_consistent_add_server(void *s, bsc *svr, unsigned int weight) /* {{{ */
{
	bsc_consistent_state_t *state = s;
	int i, key_len, points = weight * BSC_CONSISTENT_POINTS;
	unsigned int seed = state->hash->init(), hash;
	
	/* buffer for "host:port-i\0" */
	char *key = emalloc(strlen(svr->host) + MAX_LENGTH_OF_LONG * 2 + 3);
	key_len = sprintf(key, "%s:%d-", svr->host, svr->port);
	seed = state->hash->combine(seed, key, key_len);

	/* add weight * BSC_CONSISTENT_POINTS number of points for this server */
	state->points = erealloc(state->points, sizeof(*state->points) * (state->num_points + points));

	for (i=0; i<points; i++) {
		key_len = sprintf(key, "%d", i);
		hash = state->hash->finish(state->hash->combine(seed, key, key_len));
		state->points[state->num_points + i].server = svr;
		state->points[state->num_points + i].point = hash;
	}

	state->num_points += points;
	state->num_servers++;
	state->buckets_populated = 0;

	efree(key);
}
/* }}} */

bsc_hash_strategy_t bsc_consistent_hash = {
	bsc_consistent_create_state,
	bsc_consistent_free_state,
	bsc_consistent_find_server,
	bsc_consistent_add_server
};

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
