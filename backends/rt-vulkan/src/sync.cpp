#include "sync.h"

#include <condition_variable>
#include <mutex>
#include <new>

struct rtvk_mutex : std::mutex mutex {};

struct rtvk_condition {
	std::condition_variable condition;
};

extern "C" {

struct rtvk_mutex* rtvk_mutex_create(void) {
	return new (std::nothrow) rtvk_mutex;
}

usize rtvk_mutex_allocation_size(void) {
	return sizeof(rtvk_mutex);
}

void rtvk_mutex_destroy(struct rtvk_mutex* mutex) {
	delete mutex;
}

void rtvk_mutex_lock(struct rtvk_mutex* mutex) {
	mutex->mutex.lock();
}

void rtvk_mutex_unlock(struct rtvk_mutex* mutex) {
	mutex->mutex.unlock();
}

struct rtvk_condition* rtvk_condition_create(void) {
	return new (std::nothrow) rtvk_condition;
}

void rtvk_condition_destroy(struct rtvk_condition* condition) {
	delete condition;
}

void rtvk_condition_wait(struct rtvk_condition* condition, struct rtvk_mutex* mutex) {
	std::unique_lock lock(mutex->mutex, std::adopt_lock);
	condition->condition.wait(lock);
	lock.release();
}

void rtvk_condition_broadcast(struct rtvk_condition* condition) {
	condition->condition.notify_all();
}

}
