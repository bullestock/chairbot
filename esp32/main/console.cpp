#include "console.h"

#include "battery.h"
#include "config.h"
#include "motor.h"
#include "peripherals.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"

#include <driver/i2c_master.h>
#include <driver/uart.h>

#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"

int led_test(int argc, char** argv)
{
    int count = 10;
    if (argc > 1)
        count = atoi(argv[1]);

    printf("Running LED test (%d)\n", count);
    for (int j = 0; j < count; ++j)
    {
        gpio_set_level(GPIO_INTERNAL_LED, 1);
        vTaskDelay(500/portTICK_PERIOD_MS);
        gpio_set_level(GPIO_INTERNAL_LED, 0);
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
    return 0;
}

int motor_test(int argc, char** argv)
{
    int count = 1;
    if (argc > 1)
        count = atoi(argv[1]);

    printf("Running motor test (%d)\n", count);
    for (int j = 0; j < count; ++j)
    {
        set_motors(0, 0);
        for (int i = 10; i <= 100; ++i)
        {
            if (!(i % 10))
                printf("A %d\n", i);
            set_motors(i/100.0, 0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(50/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        for (int i = 100; i >= -100; --i)
        {
            if (!(i % 10))
                printf("A %d\n", i);
            set_motors(i/100.0, 0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(50/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        for (int i = -100; i <= 0; ++i)
        {
            if (!(i % 10))
                printf("A %d\n", i);
            set_motors(i/100.0, 0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(50/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        set_motors(0, 0);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        for (int i = 10; i <= 100; ++i)
        {
            if (!(i % 10))
                printf("B %d\n", i);
            set_motors(0, i/100.0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(50/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        for (int i = 100; i >= -100; --i)
        {
            if (!(i % 10))
                printf("B %d\n", i);
            set_motors(0, i/100.0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(50/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        for (int i = -100; i <= 0; ++i)
        {
            if (!(i % 10))
                printf("B %d\n", i);
            set_motors(0, i/100.0);
            gpio_set_level(GPIO_INTERNAL_LED, 1);
            vTaskDelay(50/portTICK_PERIOD_MS);
            gpio_set_level(GPIO_INTERNAL_LED, 0);
        }
        set_motors(0, 0);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    return 0;
}

int i2c_cmd(int argc, char** argv)
{
    static const char* i2c_usage =
       "Valid commands:\n"
       "scan [<count>]       Scan for I2C devices\n"
       "read <addr>          Read from device at specified address\n"
       "write <addr> <data>  Write to device at specified address\n";

    if (argc < 2)
    {
        printf("Error: Missing command\n");
        printf(i2c_usage);
        return -1;
    }
    if (!strcmp(argv[1], "scan"))
    {
        int count = 1;
        if (argc > 2)
            count = atoi(argv[2]);
        printf("Scanning for I2C devices (%d)\n", count);
        /*
        for (int j = 0; j < count; ++j)
        {
            printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
            printf("00:         ");
            for (int i = 3; i < 0x78; ++i)
            {
                i2c_cmd_handle_t cmd = i2c_cmd_link_create();
                i2c_master_start(cmd);
                i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1); // expect ack
                i2c_master_stop(cmd);

                const auto espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
                if (i%16 == 0)
                    printf("\n%.2x:", i);
                if (espRc == 0)
                    printf(" %.2x", i);
                else
                    printf(" --");
                //ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
                i2c_cmd_link_delete(cmd);
            }
            printf("\n");
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
*/
        return 0;
    }
    if (!strcmp(argv[1],"read"))
    {
        return 0;
    }
    if (!strcmp(argv[1], "write"))
    {
        if (argc < 3)
        {
            printf("Error: Missing address\n");
            printf(i2c_usage);
            return -1;
        }
        const int address = atoi(argv[2]);

        if (argc < 4)
        {
            printf("Error: Missing data\n");
            printf(i2c_usage);
            return -1;
        }
        const int data = atoi(argv[3]);

        /*
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, address << 1 | I2C_MASTER_WRITE, 1);
        i2c_master_write_byte(cmd, data, 1);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS);
        if (ret == ESP_OK)
            printf("I2C write %d to %d OK\n", data, address);
        else if (ret == ESP_ERR_TIMEOUT)
            printf("Error: Bus is busy\n");
        else
            printf("Error: Write failed: %d", ret);
        i2c_cmd_link_delete(cmd);
        */
        return 0;
    }
    printf("Error: Unknown command\n");
    printf(i2c_usage);
    return -1;
}

int read_battery(int argc, char** argv)
{
    Battery bat;
    for (int i = 0; i < 10; ++i)
    {
        printf("Battery voltage: %f V\n", bat.read_voltage());
        vTaskDelay(100/portTICK_PERIOD_MS);
    }

    return 0;
}

int control_peripherals(int argc, char** argv)
{
    static const char* peripherals_usage =
       "Valid commands:\n"
       "sound <number>     Play sound\n"
       "volume <number>    Set volume\n"
       "pwm <addr> <data>  Set PWM output\n";

    if (argc < 2)
    {
        printf("Error: Missing command\n");
        printf(peripherals_usage);
        return -1;
    }
    if (!strcmp(argv[1], "sound"))
    {
        if (argc < 3)
        {
            printf("Error: Missing sound number\n");
            return -1;
        }
        int sound = atoi(argv[2]);
        peripherals_play_sound(sound);
        return 0;
    }
    if (!strcmp(argv[1], "volume"))
    {
        if (argc < 3)
        {
            printf("Error: Missing volume level\n");
            return -1;
        }
        int volume = atoi(argv[2]);
        peripherals_set_volume(volume);
        return 0;
    }
    if (!strcmp(argv[1], "pwm"))
    {
        if (argc < 3)
        {
            printf("Error: Missing address\n");
            printf(peripherals_usage);
            return -1;
        }
        const int chan = atoi(argv[2]);

        if (argc < 4)
        {
            printf("Error: Missing data\n");
            printf(peripherals_usage);
            return -1;
        }
        const int value = atoi(argv[3]);

        peripherals_set_pwm(chan, value);
        return 0;
    }
    printf("Error: Unknown command\n");
    return -1;
}

void initialize_console()
{
    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    uart_config_t uart_config;
    memset(&uart_config, 0, sizeof(uart_config));
    uart_config.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.source_clk = UART_SCLK_REF_TICK;
    ESP_ERROR_CHECK(uart_param_config((uart_port_t) CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install((uart_port_t) CONFIG_ESP_CONSOLE_UART_NUM,
                                         256, 0, 0, NULL, 0));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

    /* Initialize the console */
    esp_console_config_t console_config;
    memset(&console_config, 0, sizeof(console_config));
    console_config.max_cmdline_args = 8;
    console_config.max_cmdline_length = 256;
#if CONFIG_LOG_COLORS
    console_config.hint_color = atoi(LOG_COLOR_CYAN);
#endif
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);
}

void run_console()
{
    initialize_console();

    /* Register commands */
    esp_console_register_help_command();

    const esp_console_cmd_t cmd1 = {
        .command = "motortest",
        .help = "Test the motors",
        .hint = NULL,
        .func = &motor_test,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd1));

    const esp_console_cmd_t cmd2 = {
        .command = "i2c",
        .help = "Manage I2C devices",
        .hint = NULL,
        .func = &i2c_cmd,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd2));

    const esp_console_cmd_t cmd3 = {
        .command = "readbattery",
        .help = "Read battery voltage",
        .hint = NULL,
        .func = &read_battery,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd3));

    const esp_console_cmd_t cmd4 = {
        .command = "peripherals",
        .help = "Control peripherals",
        .hint = NULL,
        .func = &control_peripherals,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd4));

    const esp_console_cmd_t cmd5 = {
        .command = "ledtest",
        .help = "Test the LED",
        .hint = NULL,
        .func = &led_test,
        .argtable = nullptr
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd5));

    /* Prompt to be printed before each line.
     * This can be customized, made dynamic, etc.
     */
    const char* prompt = LOG_COLOR_I "esp32> " LOG_RESET_COLOR;

    printf("\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n");

    /* Figure out if the terminal supports escape sequences */
    int probe_status = linenoiseProbe();
    if (probe_status)
    {
        printf("\n"
               "Your terminal application does not support escape sequences.\n"
               "Line editing and history features are disabled.\n"
               "On Windows, try using Putty instead.\n");
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        /* Since the terminal doesn't support escape sequences,
         * don't use color codes in the prompt.
         */
        prompt = "esp32> ";
#endif //CONFIG_LOG_COLORS
    }

    while (true)
    {
        char* line = linenoise(prompt);
        if (line == NULL)
            continue;

        linenoiseHistoryAdd(line);

        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND)
            printf("Unrecognized command\n");
        else if (err == ESP_ERR_INVALID_ARG)
            ; // command was empty
        else if (err == ESP_OK && ret != ESP_OK)
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(err));
        else if (err != ESP_OK)
            printf("Internal error: %s\n", esp_err_to_name(err));

        linenoiseFree(line);
    }
}

// Local Variables:
// compile-command: "(cd ..; idf.py build)"
// End:
