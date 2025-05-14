/*
 * Copyright (c) 2016 Zibin Zheng <znbin@qq.com>
 * All rights reserved
 */

#include "multi_button.h"

#define EVENT_CB(ev)   if(handle->cb[ev])handle->cb[ev]((void*)handle)
#if EVENT_ENABLE
#define EVENT_SET(ev)  handle->event=(uint8_t)(ev)
#else
#define EVENT_SET(ev)
#endif

#if DOUBLE_ENABLE
#define PRESS_REPEAT_MAX_NUM  15 /*!< The maximum value of the repeat counter */
#else
#define PRESS_REPEAT_MAX_NUM  255 /*!< The maximum value of the repeat counter */
#endif

//button handle list head.
static struct Button* head_handle = NULL;

static void button_handler(struct Button* handle);

/**
  * @brief  Initializes the button struct handle.
  * @param  handle: the button handle struct.
  * @param  pin_level: read the HAL GPIO of the connected button level.
  * @param  active_level: pressed GPIO level.
  * @param  button_id: the button id.
  * @retval None
  */
void button_init(struct Button* handle, uint8_t(*pin_level)(uint8_t), uint8_t active_level, uint8_t button_id)
{
	memset(handle, 0, sizeof(struct Button));
	EVENT_SET(NONE_PRESS);
	handle->hal_button_Level = pin_level;
	handle->button_level = !active_level;
	handle->active_level = active_level;
	handle->button_id = button_id;
}

/**
  * @brief  Attach the button event callback function.
  * @param  handle: the button handle struct.
  * @param  event: trigger event type.
  * @param  cb: callback function.
  * @retval None
  */
void button_attach(struct Button* handle, PressEvent event, BtnCallback cb)
{
	handle->cb[event] = cb;
}

/**
  * @brief  Inquire the button event happen.
  * @param  handle: the button handle struct.
  * @retval button event.
  */
#if EVENT_ENABLE
PressEvent get_button_event(struct Button* handle)
{
	return (PressEvent)(handle->event);
}
#endif
/**
  * @brief  Button driver core function, driver state machine.
  * @param  handle: the button handle struct.
  * @retval None
  */
static void button_handler(struct Button* handle)
{
	uint8_t read_gpio_level = handle->hal_button_Level(handle->button_id);

	//ticks counter working..
	if((handle->state) > 0) handle->ticks++;

	/*------------button debounce handle---------------*/
#if DEBOUNCE_TICKS
	if(read_gpio_level != handle->button_level) { //not equal to prev one
		//continue read 3 times same new level change
		if(++(handle->debounce_cnt) >= DEBOUNCE_TICKS) {
			handle->button_level = read_gpio_level;
			handle->debounce_cnt = 0;
		}
	} else { //level not change ,counter reset.
		handle->debounce_cnt = 0;
	}
#endif
	/*-----------------State machine-------------------*/
	switch (handle->state) {
	case 0:
		if(handle->button_level == handle->active_level) {	//start press down
			EVENT_SET(PRESS_DOWN);
			EVENT_CB(PRESS_DOWN);
			handle->ticks = 0;
      #if DOUBLE_ENABLE
			handle->repeat = 1;
      #endif
			handle->state = 1;
		} else {
			EVENT_SET(NONE_PRESS);
		}
		break;

	case 1:
		if(handle->button_level != handle->active_level) { //released press up
			EVENT_SET(PRESS_UP);
			EVENT_CB(PRESS_UP);
			handle->ticks = 0;
	#if DOUBLE_ENABLE
			handle->state = 2;
	#else
			EVENT_SET(SINGLE_CLICK);
			EVENT_CB(SINGLE_CLICK);
			handle->state = 0;
	#endif
		} else if(handle->ticks > LONG_TICKS) {
			EVENT_SET(LONG_PRESS_START);
			EVENT_CB(LONG_PRESS_START);
			handle->state = 5;
		}
		break;
#if DOUBLE_ENABLE
	case 2:
		if(handle->button_level == handle->active_level) { //press down again
			EVENT_SET(PRESS_DOWN);
			EVENT_CB(PRESS_DOWN);
			if(handle->repeat != PRESS_REPEAT_MAX_NUM) {
				handle->repeat++;
			}
			EVENT_CB(PRESS_REPEAT); // repeat hit
			handle->ticks = 0;
			handle->state = 3;
		} else if(handle->ticks > SHORT_TICKS) { //released timeout
			if(handle->repeat == 1) {
				EVENT_SET(SINGLE_CLICK);
				EVENT_CB(SINGLE_CLICK);
			} else if(handle->repeat == 2) {
				EVENT_SET(DOUBLE_CLICK);
				EVENT_CB(DOUBLE_CLICK); // repeat hit
			}
			handle->state = 0;
		}
		break;

	case 3:
		if(handle->button_level != handle->active_level) { //released press up
			EVENT_SET(PRESS_UP);
			EVENT_CB(PRESS_UP);
			if(handle->ticks < SHORT_TICKS) {
				handle->ticks = 0;
				handle->state = 2; //repeat press
			} else {
				handle->state = 0;
			}
		} else if(handle->ticks > SHORT_TICKS) { // SHORT_TICKS < press down hold time < LONG_TICKS
			handle->state = 1;
		}
		break;
#endif
	case 5:
		if(handle->button_level == handle->active_level) {
			//continue hold trigger
			EVENT_SET(LONG_PRESS_HOLD);
			EVENT_CB(LONG_PRESS_HOLD);
		} else { //released
			EVENT_SET(PRESS_UP);
			EVENT_CB(PRESS_UP);
			handle->state = 0; //reset
		}
		break;
	default:
		handle->state = 0; //reset
		break;
	}
}

/**
  * @brief  Start the button work, add the handle into work list.
  * @param  handle: target handle struct.
  * @retval 0: succeed. -1: already exist.
  */
int button_start(struct Button* handle)
{
	struct Button* target = head_handle;
	while(target) {
		if(target == handle) return -1;	//already exist.
		target = target->next;
	}
	handle->next = head_handle;
	head_handle = handle;
	return 0;
}

/**
  * @brief  Stop the button work, remove the handle off work list.
  * @param  handle: target handle struct.
  * @retval None
  */
void button_stop(struct Button* handle)
{
	struct Button** curr;
	for(curr = &head_handle; *curr; ) {
		struct Button* entry = *curr;
		if(entry == handle) {
			*curr = entry->next;
//			free(entry);
			return;//glacier add 2021-8-18
		} else {
			curr = &entry->next;
		}
	}
}

/**
  * @brief  background ticks, timer repeat invoking interval 5ms.
  * @param  None.
  * @retval None
  */
void button_ticks(void)
{
	struct Button* target;
	for(target=head_handle; target; target=target->next) {
		button_handler(target);
	}
}
