<?php

// Start of counterlock.

/**
 * Get or create a semaphore counter id
 *
 * @param int $key
 * @param int $perm [optional] <p>
 * The semaphore permissions. Actually this value is
 * set only if the process finds it is the only process currently
 * attached to the semaphore.
 * </p>
 *
 * @return resource a positive counter identifier on success, or <b>FALSE</b> on
 * error.
 */
function counter_create ($key, $perm = 0666) {}

/**
 * Increment a counter
 * @param resource $resource <p>
 * <i>resource</i> is a resource, obtained from <b>counter_create</b>.
 * </p>
 * @return bool <b>TRUE</b> on success or <b>FALSE</b> on failure.
 */
function counter_increment ($resource) {}

/**
 * Increment a counter
 * @param resource $resource <p>
 * A resource handle as returned by
 * <b>counter_create</b>.
 * </p>
 * @return bool <b>TRUE</b> on success or <b>FALSE</b> on failure.
 */
function counter_decrement ($resource) {}

/**
 * Get counter value.
 *
 * @param resource $resource <p>
 * A resource handle as returned by
 * <b>counter_create</b>.
 * </p>
 * @return integer|false <b>FALSE</b> on failure.
 */
function counter_value ($resource) {}

/**
 * Remove a System V semaphore for counter
 * @param resource $resource <p>
 * A semaphore resource identifier as returned
 * by <b>sem_get</b>.
 * </p>
 * @return bool <b>TRUE</b> on success or <b>FALSE</b> on failure.
 */
function counter_remove ($resource) {}

// End of counterlock.

