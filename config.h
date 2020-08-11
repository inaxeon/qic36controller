/* 
 * File:   config.h
 * Author: Matt
 *
 * Created on 13 July 2020, 06:17
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_MAGIC        0x5243
#define MAX_DESC            32

#define OPERATION_NONE          0
#define OPERATION_EXERCISE      1
#define OPERATION_WRITE_TEST    2
#define OPERATION_REWIND        3

typedef struct {
    uint16_t magic;
    uint8_t operation;
    uint8_t stopat_track;
} sys_config_t;

void configuration_bootprompt(sys_config_t *config);
void load_configuration(sys_config_t *config);

#endif /* __CONFIG_H__ */