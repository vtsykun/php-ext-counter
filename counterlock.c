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

/* @see http://man7.org/linux/man-pages/man2/semop.2.html */
/* @see http://man7.org/linux/man-pages/man2/semget.2.html */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_counterlock.h"
#include "Zend/zend_gc.h"
#include "Zend/zend_builtin_functions.h"
#include "Zend/zend_extensions.h" /* for ZEND_EXTENSION_API_NO */
#include "ext/standard/php_array.h"
#include "Zend/zend_interfaces.h"
#include "Zend/zend_alloc.h"
#include "SAPI.h"

#if !HAVE_SEMUN

union semun {
	int val;                    /* value for SETVAL */
	struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
	unsigned short int *array;  /* array for GETALL, SETALL */
	struct seminfo *__buf;      /* buffer for IPC_INFO */
};

#undef HAVE_SEMUN
#define HAVE_SEMUN 1

#endif

/* {{{ arginfo */
ZEND_BEGIN_ARG_INFO_EX(arginfo_counter_create, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, perm)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_counter_increment, 0, 0, 1)
	ZEND_ARG_INFO(0, sem_identifier)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_counter_decrement, 0, 0, 1)
	ZEND_ARG_INFO(0, sem_identifier)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_counter_remove, 0, 0, 1)
	ZEND_ARG_INFO(0, sem_identifier)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_counter_value, 0, 0, 1)
	ZEND_ARG_INFO(0, sem_identifier)
ZEND_END_ARG_INFO()
/* }}} */

ZEND_GET_MODULE(counterlock);
THREAD_LS sysvcount_module php_sysvcount_module;

zend_function_entry counterlock_functions[] = {
        PHP_FE(counter_create, arginfo_counter_create)
		PHP_FE(counter_increment, arginfo_counter_increment)
		PHP_FE(counter_decrement, arginfo_counter_decrement)
		PHP_FE(counter_remove, arginfo_counter_remove)
		PHP_FE(counter_value, arginfo_counter_value)
        PHP_FE_END
};

zend_module_entry counterlock_module_entry = {
        STANDARD_MODULE_HEADER,
        "Cuantic SysSem counter",
        counterlock_functions,
        PHP_MINIT(counterlock),
        NULL,
        NULL,
        NULL,
        NULL,
        PHP_COUNTERLOCK_VERSION,
        STANDARD_MODULE_PROPERTIES,
};

static void release_sysvcount_sem(zend_resource *rsrc)
{
	sysvcount_sem *sem_ptr = (sysvcount_sem *)rsrc->ptr;
	struct sembuf sop[1];
	int opcount = 1;

	efree(sem_ptr);
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(counterlock)
{
	// Declare resurece ID
	// To prevent PHP Warning:  Unknown list entry type (0) in Unknown on line 0
	php_sysvcount_module.le_sem = zend_register_list_destructors_ex(release_sysvcount_sem, NULL, "counterlock", module_number);
	return SUCCESS;
}

PHP_FUNCTION(counter_create)
{
	zend_long key, perm = 0666;
	struct sembuf sop[1];
	int semid;
	int count;
	sysvcount_sem *sem_ptr;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "l|l", &key, &perm)) {
		RETURN_FALSE;
	}

	/* Get/create the semaphore.  Note that we rely on the semaphores
	 * being zeroed when they are created.  Despite the fact that
	 * the(?)  Linux semget() man page says they are not initialized,
	 * the kernel versions 2.0.x and 2.1.z do in fact zero them.
	 */
	semid = semget(key, 1, perm|IPC_CREAT);
	if (semid == -1) {
		php_error_docref(NULL, E_WARNING, "failed for key 0x" ZEND_XLONG_FMT ": %s", key, strerror(errno));
		RETURN_FALSE;
	}

	sem_ptr = (sysvcount_sem *) emalloc(sizeof(sysvcount_sem));
	sem_ptr->key   = key;
	sem_ptr->semid = semid;
	sem_ptr->count = 0;
	RETVAL_RES(zend_register_resource(sem_ptr, php_sysvcount_module.le_sem));
	sem_ptr->id = Z_RES_HANDLE_P(return_value);
}

PHP_FUNCTION(counter_increment)
{
	zval *arg_id;
	zend_bool nowait = 0;
	sysvcount_sem *sem_ptr;
	struct sembuf sop;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "r", &arg_id)) {
		RETURN_FALSE;
	}

	if ((sem_ptr = (sysvcount_sem *)zend_fetch_resource(Z_RES_P(arg_id), "SysV counter", php_sysvcount_module.le_sem)) == NULL) {
		RETURN_FALSE;
	}

	sop.sem_num = 0;
	sop.sem_op  = 1;
	sop.sem_flg = SEM_UNDO;

	while (semop(sem_ptr->semid, &sop, 1) == -1) {
		if (errno != EINTR) {
			if (errno != EAGAIN) {
				php_error_docref(NULL, E_WARNING, "failed to %s key 0x%x: %s", "increment", sem_ptr->key, strerror(errno));
			}
			RETURN_FALSE;
		}
	}

	sem_ptr->count += 1;
	RETURN_TRUE;
}

PHP_FUNCTION(counter_decrement)
{
	zval *arg_id;
	zend_bool nowait = 0;
	sysvcount_sem *sem_ptr;
	struct sembuf sop;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "r", &arg_id)) {
		RETURN_FALSE;
	}

	if ((sem_ptr = (sysvcount_sem *)zend_fetch_resource(Z_RES_P(arg_id), "SysV counter", php_sysvcount_module.le_sem)) == NULL) {
		RETURN_FALSE;
	}

	if (sem_ptr->count == 0) {
		php_error_docref(NULL, E_WARNING, "SysV counter " ZEND_LONG_FMT " (key 0x%x) is not increment yet", Z_LVAL_P(arg_id), sem_ptr->key);
		RETURN_FALSE;
	}

	sop.sem_num = 0;
	sop.sem_op  = -1;
	sop.sem_flg = SEM_UNDO | IPC_NOWAIT;

	while (semop(sem_ptr->semid, &sop, 1) == -1) {
		if (errno != EINTR) {
			if (errno != EAGAIN) {
				php_error_docref(NULL, E_WARNING, "failed to %s key 0x%x: %s", "decrement", sem_ptr->key, strerror(errno));
			}
			RETURN_FALSE;
		}
	}

	sem_ptr->count -= 1;
	RETURN_TRUE;
}

PHP_FUNCTION(counter_remove)
{
	zval *arg_id;
	sysvcount_sem *sem_ptr;
	union semun un;
	struct semid_ds buf;

	if (zend_parse_parameters(ZEND_NUM_ARGS(), "r", &arg_id) == FAILURE) {
		return;
	}

	if ((sem_ptr = (sysvcount_sem *)zend_fetch_resource(Z_RES_P(arg_id), "SysV counter", php_sysvcount_module.le_sem)) == NULL) {
		RETURN_FALSE;
	}

	un.buf = &buf;
	if (semctl(sem_ptr->semid, 0, IPC_STAT, un) < 0) {
		php_error_docref(NULL, E_WARNING, "SysV counter " ZEND_LONG_FMT " does not (any longer) exist", Z_LVAL_P(arg_id));
		RETURN_FALSE;
	}

	if (semctl(sem_ptr->semid, 0, IPC_RMID, un) < 0) {
		php_error_docref(NULL, E_WARNING, "failed for SysV counter " ZEND_LONG_FMT ": %s", Z_LVAL_P(arg_id), strerror(errno));
		RETURN_FALSE;
	}

	/* let release_sysvsem_sem know we have removed
	 * the semaphore to avoid issues with releasing.
	 */

	sem_ptr->count = -1;
	RETURN_TRUE;
}

PHP_FUNCTION(counter_value)
{
	zval *arg_id;
	zend_bool nowait = 0;
	sysvcount_sem *sem_ptr;
	struct sembuf sop;
	int retval;

	if (FAILURE == zend_parse_parameters(ZEND_NUM_ARGS(), "r", &arg_id)) {
		RETURN_FALSE;
	}

	if ((sem_ptr = (sysvcount_sem *)zend_fetch_resource(Z_RES_P(arg_id), "SysV counter", php_sysvcount_module.le_sem)) == NULL) {
		RETURN_FALSE;
	}

    retval = semctl(sem_ptr->semid, 0, GETVAL, NULL);
	if (retval == -1) {
		php_error_docref(NULL, E_WARNING, "failed for key 0x" ZEND_XLONG_FMT ": %s", sem_ptr->key, strerror(errno));
		RETURN_FALSE;
	}

	RETURN_LONG(retval);
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
