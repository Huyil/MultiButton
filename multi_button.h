/*
 * Copyright (c) 2016 Zibin Zheng <znbin@qq.com>
 * All rights reserved
 */

#ifndef _MULTI_BUTTON_H_
#define _MULTI_BUTTON_H_

#include <stdint.h>
#include <string.h>

//According to your need to modify the constants.
#define TICKS_INTERVAL    10	//ms
#define DEBOUNCE_TICKS    3	//MAX 7 (0 ~ 7)
#define SHORT_TICKS       (150  / TICKS_INTERVAL) //default shortTick is 0 (only SINGLE_CLICK)
#define LONG_TICKS        (1000 / TICKS_INTERVAL) //default longTick  is LONG_TICKS

#define EVENT_ENABLE      1     //Enable EVENT GET
#define DOUBLE_ENABLE     1     //Enable Double Click

typedef void (*BtnCallback)(void*);

#if EVENT_ENABLE
#if DOUBLE_ENABLE
  typedef enum {
    PRESS_DOWN = 0,
    PRESS_UP,
    PRESS_REPEAT,
    SINGLE_CLICK,
    DOUBLE_CLICK,
    LONG_PRESS_START,
    LONG_PRESS_HOLD,
    number_of_event,
    NONE_PRESS
  }PressEvent;
  typedef struct Button {
    uint16_t ticks;
    uint8_t  repeat : 4;
    uint8_t  event : 4;
    uint8_t  state : 3;
    uint8_t  debounce_cnt : 3;
    uint8_t  active_level : 1;
    uint8_t  button_level : 1;
    uint8_t  button_id;
    uint8_t  (*hal_button_Level)(uint8_t button_id_);
    uint8_t  shortTick;
    uint16_t  longTick;
    BtnCallback  cb[number_of_event];
    struct Button* next;
  }Button;
#else
  typedef enum {
    PRESS_DOWN = 0,
    PRESS_UP,
    PRESS_REPEAT,
    SINGLE_CLICK,
    LONG_PRESS_START,
    LONG_PRESS_HOLD,
    number_of_event,
    NONE_PRESS
  }PressEvent;
  typedef struct Button {
    uint16_t ticks;
    uint8_t  repeat : 4;
    uint8_t  event : 4;
    uint8_t  state : 3;
    uint8_t  debounce_cnt : 3;
    uint8_t  active_level : 1;
    uint8_t  button_level : 1;
    uint8_t  button_id;
    uint8_t  (*hal_button_Level)(uint8_t button_id_);
    uint16_t  longTick;
    BtnCallback  cb[number_of_event];
    struct Button* next;
  }Button;
#endif
#else
  #if DOUBLE_ENABLE
    typedef enum {
      PRESS_DOWN = 0,
      PRESS_UP,
      PRESS_REPEAT,
      SINGLE_CLICK,
      DOUBLE_CLICK,
      LONG_PRESS_START,
      LONG_PRESS_HOLD,
      number_of_event,
      NONE_PRESS
    }PressEvent;

    typedef struct Button {
      uint16_t ticks;
      uint8_t  repeat;
      uint8_t  state : 3;
      uint8_t  debounce_cnt : 3;
      uint8_t  active_level : 1;
      uint8_t  button_level : 1;
      uint8_t  button_id;
      uint8_t  (*hal_button_Level)(uint8_t button_id_);
      BtnCallback  cb[number_of_event];
      struct Button* next;
    }Button;
  #else
    typedef enum {
      PRESS_DOWN = 0,
      PRESS_UP,
      SINGLE_CLICK,
      LONG_PRESS_START,
      LONG_PRESS_HOLD,
      number_of_event,
      NONE_PRESS
    }PressEvent;
    typedef struct Button {
      uint16_t ticks;
      uint8_t  state : 3;
      uint8_t  debounce_cnt : 3;
      uint8_t  active_level : 1;
      uint8_t  button_level : 1;
      uint8_t  button_id;
      uint8_t  (*hal_button_Level)(uint8_t button_id_);
      BtnCallback  cb[number_of_event];
      struct Button* next;
    }Button;
  #endif
#endif
#ifdef __cplusplus
extern "C" {
#endif

void button_init(struct Button* handle, uint8_t(*pin_level)(uint8_t), uint8_t active_level, uint8_t button_id);
void button_attach(struct Button* handle, PressEvent event, BtnCallback cb);
#if EVENT_ENABLE
PressEvent get_button_event(struct Button* handle);
#endif
int  button_start(struct Button* handle);
void button_stop(struct Button* handle);
void button_ticks(void);

#ifdef __cplusplus
}
#endif

#endif
