/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2018 The PHP Group                                |
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

/* $Id$ */

#ifndef PHP_COUNTERLOCK_H
#define PHP_COUNTERLOCK_H

extern zend_module_entry counterlock_module_entry;
#define phpext_counterlock_ptr &counterlock_module_entry

#define PHP_COUNTERLOCK_VERSION "1.0.0" /* Replace with version number for your extension */
#define PHP_COUNTERLOCK_API

PHP_MINIT_FUNCTION(counterlock);
PHP_FUNCTION(counter_create);
PHP_FUNCTION(counter_increment);
PHP_FUNCTION(counter_remove);
PHP_FUNCTION(counter_decrement);
PHP_FUNCTION(counter_value);

typedef struct {
	int id;						/* For error reporting. */
	int key;					/* For error reporting. */
	int semid;					/* Returned by semget(). */
	int count;					/* Incrimnent count. */
} sysvcount_sem;

typedef struct {
	int le_sem;
} sysvcount_module;

extern sysvcount_module php_sysvcount_module;

#endif	/* PHP_COUNTERLOCK_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
