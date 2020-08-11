/* 
 * File:   config.c
 * Author: Matt
 *
 * Created on 13 July 2020, 06:17
 */


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"
#include "config.h"
#include "util.h"
#include "usart.h"
#include "iopins.h"

#define CMD_NONE              0x00
#define CMD_READLINE          0x01
#define CMD_COMPLETE          0x02
#define CMD_ESCAPE            0x03
#define CMD_AWAIT_NAV         0x04
#define CMD_PREVCOMMAND       0x05
#define CMD_NEXTCOMMAND       0x06
#define CMD_DEL               0x07
#define CMD_DROP_NAV          0x08
#define CMD_CANCEL            0x10

#define CTL_CANCEL            0x03
#define CTL_XOFF              0x13
#define CTL_U                 0x15

#define SEQ_ESCAPE_CHAR       0x1B
#define SEQ_CTRL_CHAR1        0x5B
#define SEQ_ARROW_UP          0x41
#define SEQ_ARROW_DOWN        0x42
#define SEQ_HOME              0x31
#define SEQ_INS               0x32
#define SEQ_DEL               0x33
#define SEQ_END               0x34
#define SEQ_PGUP              0x35
#define SEQ_PGDN              0x36
#define SEQ_NAV_END           0x7E

#define CMD_MAX_LINE          64
#define CMD_MAX_HISTORY       4

#define PARAM_U16             1
#define PARAM_U8              2
#define PARAM_DESC            3

static inline int8_t configuration_prompt_handler(char *message, sys_config_t *config);
static int8_t get_line(char *str, int8_t max, uint8_t *ignore_lf);
static int get_string(char *str, int8_t max, uint8_t *ignore_lf);
static uint8_t parse_param(void *param, uint8_t type, char *arg);
static void save_configuration(sys_config_t *config);
static void default_configuration(sys_config_t *config);
static int8_t parse_operation_arg(const char *arg);
static uint8_t do_select_drive(char *arg);
static uint8_t do_reset_drive(char *arg);
static uint8_t do_select_track(char *arg);
static uint8_t do_go_drive(char *arg);
static uint8_t do_state(char *arg);

uint8_t _g_max_history;
uint8_t _g_show_history;
uint8_t _g_next_history;
char _g_cmd_history[CMD_MAX_HISTORY][CMD_MAX_LINE];

static void do_help(void)
{
    printf(
        "\r\nCommands:\r\n\r\n"
        "\toperation none|exercise|rewind|writetest\r\n"
        "\t\tIn the case of 'writetest' Ensure 150.15 KHz test signal input\r\n"
        "\tstopat 0-8\r\n"
        "\t\tThe index of the last track to record when writing a test tape\r\n"
        "\tdriveselect|s 0|1\r\n"
        "\tdrivereset|r\r\n"
        "\tdrivego|g f|fwd r|rev s|stop\r\n"
        "\tdrivetrack|r 0-8\r\n"
        "\t\tOnly observed by drive at EOT/BOT and only before motor start\r\n"
        "\tdrivestate|t\r\n"
        "\trun [none|exercise|rewind|writetesttape]\r\n"
        "\r\n"
    );
}

void configuration_bootprompt(sys_config_t *config)
{
    char cmdbuf[64];
    uint8_t i;
    int8_t enter_bootpromt = 0;
    uint8_t ignore_lf = 0;
    
    if (config->operation != OPERATION_NONE)
    {
        printf("<Press Ctrl+C to enter configuration prompt>\r\n");

        for (i = 0; i < 100; i++)
        {
            if (usart1_data_ready())
            {
                char c = usart1_get();
                if (c == 3) /* Ctrl + C */
                {
                    enter_bootpromt = 1;
                    break;
                }
            }
            __delay_ms(10);
        }
    }
    else
    {
        enter_bootpromt = 1;
    }

    if (!enter_bootpromt)
        return;

    printf("\r\n");
    
    for (;;)
    {
        int8_t ret;

        printf("config>");
        ret = get_line(cmdbuf, sizeof(cmdbuf), &ignore_lf);

        if (ret == 0 || ret == -1) {
            printf("\r\n");
            continue;
        }

        ret = configuration_prompt_handler(cmdbuf, config);

        if (ret > 0)
            printf("Error: command failed\r\n");

        if (ret == -1) {
            return;
        }
    }
}

static void do_show(sys_config_t *config)
{
    printf(
            "\r\nCurrent configuration:\r\n\r\n"
        );

    printf("\r\n");
}

static inline int8_t configuration_prompt_handler(char *text, sys_config_t *config)
{
    char *command;
    char *arg;

    command = strtok(text, " ");
    arg = strtok(NULL, "");
        
    if (!stricmp(command, "operation")) {
        int8_t operation = parse_operation_arg(arg);
        if (operation < 0)
        {
            printf("Error: Missing or invalid parameter\r\n");
            return 1;
        }
        config->operation = operation;
    }
    if (!stricmp(command, "stopat")) {
        return parse_param(&config->stopat_track, PARAM_U8, arg);
    }
    else if (!stricmp(command, "driveselect") || !stricmp(command, "s")) {
        return do_select_drive(arg);
    }
    else if (!stricmp(command, "drivereset") || !stricmp(command, "r")) {
        return do_reset_drive(arg);
    }
    else if (!stricmp(command, "drivego") || !stricmp(command, "g")) {
        return do_go_drive(arg);
    }
    else if (!stricmp(command, "drivetrack") || !stricmp(command, "k")) {
        return do_select_track(arg);
    }
    else if (!stricmp(command, "drivestate") || !stricmp(command, "t")) {
        return do_state(arg);
    }
    else if (!stricmp(command, "save")) {
        save_configuration(config);
        printf("\r\nConfiguration saved.\r\n\r\n");
        return 0;
    }
    else if (!stricmp(command, "default")) {
        default_configuration(config);
        printf("\r\nDefault configuration loaded.\r\n\r\n");
        return 0;
    }
    else if (!stricmp(command, "run")) {
        printf("\r\nStarting...\r\n");
        
        if (arg && *arg)
        {
            int8_t operation = parse_operation_arg(arg);
            
            if (operation < 0)
                return 1;
            
            config->operation = operation;
            return -1;
        }
        
        return -1;
    }
    else if (!stricmp(command, "show")) {
        do_show(config);
    }
    else if ((!stricmp(command, "help") || !stricmp(command, "?"))) {
        do_help();
        return 0;
    }
    else
    {
        printf("Error: no such command (%s)\r\n", command);
        return 1;
    }

    return 0;
}

static int8_t parse_operation_arg(const char *arg)
{        
    if (!arg || !*arg)
        return -1;
    
    if (!stricmp(arg, "none"))
    {
        return OPERATION_NONE;
    }
    else if (!stricmp(arg, "exercise"))
    {
        return OPERATION_EXERCISE;
    }
    else if (!stricmp(arg, "writetest"))
    {
        return OPERATION_WRITE_TEST;
    }
    else if (!stricmp(arg, "rewind"))
    {
        return OPERATION_REWIND;
    }
    else
    {
        printf("Error: Invalid operation\r\n");
        return -1;
    }
}

static uint8_t do_select_drive(char *arg)
{
    uint8_t res;
    uint8_t selected;

    res = parse_param(&selected, PARAM_U8, arg);
    
    if (res)
        return res;

    if (!drive_select(selected ? true : false))
        return 1;
    
    return 0;
}

static uint8_t do_go_drive(char *arg)
{
    bool go;
    bool rev;
    
    if (!arg || !*arg)
    {
        printf("Error: Missing parameter\r\n");
        return 1;
    }

    if (!stricmp(arg, "f") || !stricmp(arg, "fwd"))
    {
        go = true;
        rev = false;
    }
    else if (!stricmp(arg, "r") || !stricmp(arg, "rev"))
    {
        go = true;
        rev = false;
    }
    else if (!stricmp(arg, "s") || !stricmp(arg, "stop"))
    {
        go = false;
    }
    else
    {
        printf("Error: Invalid parameter\r\n");
        return 1;
    }
    
    if (!drive_go(go, rev))
        return 1;
    
    return 0;
}

static uint8_t do_select_track(char *arg)
{
    uint8_t res;
    uint8_t track = 0;

    res = parse_param(&track, PARAM_U8, arg);
    
    if (res)
        return res;
    
    if (track > 8)
    {
        printf("Error: Invalid parameter\r\n");
        return 1;
    }
    
    drive_select_track(track);

    return 0;
}

static uint8_t do_reset_drive(char *arg)
{
    drive_reset();
    return 0;
}

static uint8_t do_state(char *arg)
{
    if (!LTHbit && !UTHbit)
        printf("TAPE_ZONE_BOT\r\n");
    if (UTHbit && !LTHbit)
        printf("TAPE_ZONE_EOT\r\n");
    if (!UTHbit && LTHbit)
        printf("TAPE_ZONE_EW\r\n");
    if (UTHbit && LTHbit)
        printf("TAPE_ZONE_DATA\r\n");
    
    return 0;
}

static uint8_t parse_param(void *param, uint8_t type, char *arg)
{
    uint16_t u16param;
    uint8_t u8param;
    char *s;
    char *sparam;

    if (!arg || !*arg)
    {
        /* Avoid stack overflow */
        printf("Error: Missing parameter\r\n");
        return 1;
    }

    switch (type)
    {
        case PARAM_U8:
            u8param = (uint8_t)atoi(arg);
            *(uint8_t *)param = u8param;
            break;
        case PARAM_U16:
            s = strtok(arg, ".");
            u16param = atoi(s);
            *(int16_t *)param = u16param;
            break;
        case PARAM_DESC:
            sparam = (char *)param;
            strncpy(sparam, arg, MAX_DESC);
            sparam[MAX_DESC - 1] = 0;
            break;
    }
    return 0;
}

static void cmd_erase_line(uint8_t count)
{
    printf("%c[%dD%c[K", SEQ_ESCAPE_CHAR, count, SEQ_ESCAPE_CHAR);
}

static void config_next_command(char *cmdbuf, int8_t *count)
{
    uint8_t previdx;

    if (!_g_max_history)
        return;

    if (*count)
        cmd_erase_line(*count);

    previdx = ++_g_show_history;

    if (_g_show_history == CMD_MAX_HISTORY)
    {
        _g_show_history = 0;
        previdx = 0;
    }

    strcpy(cmdbuf, _g_cmd_history[previdx]);
    *count = strlen(cmdbuf);
    printf("%s", cmdbuf);
}

static void config_prev_command(char *cmdbuf, int8_t *count)
{
    uint8_t previdx;

    if (!_g_max_history)
        return;

    if (*count)
        cmd_erase_line(*count);

    if (_g_show_history == 0)
        _g_show_history = CMD_MAX_HISTORY;

    previdx = --_g_show_history;

    strcpy(cmdbuf, _g_cmd_history[previdx]);
    *count = strlen(cmdbuf);
    printf("%s", cmdbuf);
}

static int get_string(char *str, int8_t max, uint8_t *ignore_lf)
{
    unsigned char c;
    uint8_t state = CMD_READLINE;
    int8_t count;

    count = 0;
    do {
        c = wdt_getch();

        if (state == CMD_ESCAPE) {
            if (c == SEQ_CTRL_CHAR1) {
                state = CMD_AWAIT_NAV;
                continue;
            }
            else {
                state = CMD_READLINE;
                continue;
            }
        }
        else if (state == CMD_AWAIT_NAV)
        {
            if (c == SEQ_ARROW_UP) {
                config_prev_command(str, &count);
                state = CMD_READLINE;
                continue;
            }
            else if (c == SEQ_ARROW_DOWN) {
                config_next_command(str, &count);
                state = CMD_READLINE;
                continue;
            }
            else if (c == SEQ_DEL) {
                state = CMD_DEL;
                continue;
            }
            else if (c == SEQ_HOME || c == SEQ_END || c == SEQ_INS || c == SEQ_PGUP || c == SEQ_PGDN) {
                state = CMD_DROP_NAV;
                continue;
            }
            else {
                state = CMD_READLINE;
                continue;
            }
        }
        else if (state == CMD_DEL) {
            if (c == SEQ_NAV_END && count) {
                putch('\b');
                putch(' ');
                putch('\b');
                count--;
            }

            state = CMD_READLINE;
            continue;
        }
        else if (state == CMD_DROP_NAV) {
            state = CMD_READLINE;
            continue;
        }
        else
        {
            if (count >= max) {
                count--;
                break;
            }

            if (c == 19) /* Swallow XOFF */
                continue;

            if (c == CTL_U) {
                if (count) {
                    cmd_erase_line(count);
                    *(str) = 0;
                    count = 0;
                }
                continue;
            }

            if (c == SEQ_ESCAPE_CHAR) {
                state = CMD_ESCAPE;
                continue;
            }

            /* Unix telnet sends:    <CR> <NUL>
            * Windows telnet sends: <CR> <LF>
            */
            if (*ignore_lf && (c == '\n' || c == 0x00)) {
                *ignore_lf = 0;
                continue;
            }

            if (c == 3) { /* Ctrl+C */
                return -1;
            }

            if (c == '\b' || c == 0x7F) {
                if (!count)
                    continue;

                putch('\b');
                putch(' ');
                putch('\b');
                count--;
                continue;
            }
            if (c != '\n' && c != '\r') {
                putch(c);
            }
            else {
                if (c == '\r') {
                    *ignore_lf = 1;
                    break;
                }

                if (c == '\n')
                    break;
            }
            str[count] = c;
            count++;
        }
    } while (1);

    str[count] = 0;
    return count;
}

static int8_t get_line(char *str, int8_t max, uint8_t *ignore_lf)
{
    uint8_t i;
    int8_t ret;
    int8_t tostore = -1;

    ret = get_string(str, max, ignore_lf);

    if (ret <= 0) {
        return ret;
    }
    
    if (_g_next_history >= CMD_MAX_HISTORY)
        _g_next_history = 0;
    else
        _g_max_history++;

    for (i = 0; i < CMD_MAX_HISTORY; i++)
    {
        if (!stricmp(_g_cmd_history[i], str))
        {
            tostore = i;
            break;
        }
    }

    if (tostore < 0)
    {
        // Don't have this command in history. Store it
        strcpy(_g_cmd_history[_g_next_history], str);
        _g_next_history++;
        _g_show_history = _g_next_history;
    }
    else
    {
        // Already have this command in history, set the 'up' arrow to retrieve it.
        tostore++;

        if (tostore == CMD_MAX_HISTORY)
            tostore = 0;

        _g_show_history = tostore;
    }

    printf("\r\n");

    return ret;
}

void load_configuration(sys_config_t *config)
{
    uint16_t config_size = sizeof(sys_config_t);
    if (config_size > 0x100)
    {
        printf("\r\nConfiguration size is too large. Currently %u bytes.", config_size);
        reset();
    }
    
    eeprom_read_data(0, (uint8_t *)config, sizeof(sys_config_t));

    if (config->magic != CONFIG_MAGIC)
    {
        printf("\r\nNo configuration found. Setting defaults\r\n");
        default_configuration(config);
        save_configuration(config);
    }
}

static void default_configuration(sys_config_t *config)
{
    config->magic = CONFIG_MAGIC;
    config->operation = OPERATION_NONE;
    config->stopat_track = 1;
}

static void save_configuration(sys_config_t *config)
{
    eeprom_write_data(0, (uint8_t *)config, sizeof(sys_config_t));
}
