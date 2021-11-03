/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2020-2021, STMicroelectronics - All Rights Reserved
 */

#ifndef COUNTER_H
#define COUNTER_H

#include <libfdt.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/queue.h>
#include <tee_api_types.h>

enum counter_event_type {
	/* Count value increased past ceiling */
	COUNTER_EVENT_OVERFLOW,
	/* Count value reached threshold */
	COUNTER_EVENT_THRESHOLD,
	COUNTER_NB_EVENT,
};

/**
 * counter_event_cb_t - Typedef for counter event callback handler functions
 * @priv: Event consumer private data
 * @event: Id of the event reported
 */
typedef void (*counter_event_cb_t)(void *priv, enum counter_event_type event);

/**
 * @brief event structure.
 *
 * @param is_enabled True if event is enabled, false otherwise.
 * @param callback Function called on event (cannot be NULL).
 * @param priv Private data passed to the callback function.
 */
struct counter_event {
	bool is_enabled;
	counter_event_cb_t callback;
	void *priv;
};

struct counter_device;

struct counter_param {
	uint32_t *params;
	size_t len;
};

/**
 * struct counter_ops - Counter device operations
 *
 * @start: function to start the counter
 * @stop: function to stop the counter
 * @get_value: function to get the counter value
 * @set_threshold: function to set the counter threshold
 * @set_ceiling: function to set the counter ceiling
 * @enable_event:function to enable an event on counter value
 * @disable_event: function to clear all events
 * @set_config: function to set a config in the counter
 * @release_config: function to release a config from the counter
 */
struct counter_ops {
	TEE_Result (*start)(struct counter_device *counter, void *config);
	TEE_Result (*stop)(struct counter_device *counter);
	TEE_Result (*get_value)(struct counter_device *counter,
				unsigned int *ticks);
	TEE_Result (*set_threshold)(struct counter_device *counter,
				    unsigned int ticks);
	TEE_Result (*set_ceiling)(struct counter_device *counter,
				  unsigned int ticks);
	TEE_Result (*enable_event)(struct counter_device *counter,
				   enum counter_event_type event_type);
	TEE_Result (*disable_event)(struct counter_device *counter,
				    enum counter_event_type event_type);
	TEE_Result (*set_config)(struct counter_device *counter,
				 const void *param,
				 int len, void **config);
	void (*release_config)(void *config);
};

/**
 * @brief Start counter device in free running mode.
 *
 * @param name name of device.
 * @param phandle dt phandle.
 * @param dev_list list on counter device.
 * @param is_used True if counter is used (exclusive consumer).
 * @param ops Operation table of the counter.
 * @param events List of event of the counter.
 * @param max_ticks Tick max value supported by the counter.
 * @param threshold Value reached on threshold event (COUNTER_EVENT_THRESHOLD).
 * @param ceiling Value reached on overflow event (COUNTER_EVENT_OVERFLOW).
 * @param priv Optional private data supplied by driver.
 */
struct counter_device {
	const char *name;
	int phandle;
	LIST_ENTRY(counter_device) dev_list;
	bool is_used;
	const struct counter_ops *ops;
	struct counter_event events[COUNTER_NB_EVENT];
	unsigned int max_ticks;
	unsigned int threshold;
	unsigned int ceiling;
	void *priv;
};

/**
 * @brief Start counter device in free running mode.
 */
TEE_Result counter_start(struct counter_device *counter, void *config);

/**
 * @brief Stop counter device.
 */
TEE_Result counter_stop(struct counter_device *counter);

/**
 * @brief Get current counter value.
 */
TEE_Result counter_get_value(struct counter_device *counter,
			     unsigned int *ticks);

/**
 * @brief Set counter threshold.
 */
TEE_Result counter_set_threshold(struct counter_device *counter,
				 unsigned int ticks);

/**
 * @brief Set counter ceiling.
 */
TEE_Result counter_set_ceiling(struct counter_device *counter,
			       unsigned int ceiling);

/**
 * @brief Call of the registered callback function of the event.
 */
TEE_Result counter_call_callback(struct counter_device *counter,
				 enum counter_event_type event_type);

/**
 * @brief Set an event and associate a callback.
 */
TEE_Result counter_enable_event(struct counter_device *counter,
				enum counter_event_type event_type,
				counter_event_cb_t callback,
				void *priv);

/**
 * @brief Disable an event.
 */
TEE_Result counter_disable_event(struct counter_device *counter,
				 enum counter_event_type event_type);

/**
 * @brief Disable all events.
 */
TEE_Result counter_disable_all_events(struct counter_device *counter);

/**
 * @brief Release the counter configuration.
 */
void counter_release_config(struct counter_device *counter, void *config);

#ifdef CFG_DT
/**
 * @brief Parse and lookup a counter referenced by a device node.
 * Retrieve an associated configuration.
 *
 * @retval counter device if successful, else 0 on error.
 */
struct counter_device *fdt_counter_get(const void *fdt,
				       int offs, void **config);
#else
/**
 * @brief Give the counter associated configuration link to given
 * parameters.
 *
 * @retval TEE_SUCCESS if config is returned, error value otherwise.
 */
TEE_Result counter_get_config(struct counter_device *cnt_dev,
			      const struct counter_param *param,
			      void **config);
#endif

/**
 * @brief get a reference on counter device.
 *
 * @retval counter device if successful, else 0 on error.
 */
struct counter_device *counter_get_by_name(const char *name);

/* API for provider */
static inline void *counter_priv(struct counter_device *counter)
{
	return (void *)counter->priv;
}

struct counter_device *counter_dev_alloc(void);
void counter_dev_free(struct counter_device *counter);
TEE_Result counter_dev_register(struct counter_device *counter);

#endif /* COUNTER_H */
