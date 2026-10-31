#include "driver/gptimer.h"
#include "driver/gptimer_types.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig_arch.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/i2c_types.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "soc/clk_tree_defs.h"
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <characters.h>

static const char* TAG = "Shiny";

// Global handles for all functions
i2c_master_bus_handle_t masterhandle;
i2c_master_dev_handle_t Oleddev;
gptimer_handle_t shclock = NULL;
TaskHandle_t oled_task_handle = NULL;

static uint8_t display_buffer[257] = {0};

// function prototypes
void save_shiny_count(uint32_t Encounters);
void load_shiny_count(uint32_t* Encounters);
static void init_i2c(void);
static void init_nvs(void);
static void init_timer(void);
static void init_display(void);

static void Test_Display(void *pvParameters);

// Timer Callback
static bool example_timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx)
{
    /* General process for handling callbacks:
   * 1. Retrieve user context data from user_ctx (passed in from
   * gptimer_register_event_callbacks)
   * 2. Get alarm event data from edata, such as edata->count_value
   * 3. Perform user-defined operations
   * 4. Return whether a high-priority task was awakened during the above
   * operations to notify the scheduler to switch tasks */
    return false;
}

// main
void app_main(void)
{
    init_nvs();
    init_i2c();


    init_timer();
	init_display();

	// Tasks
	xTaskCreate(Test_Display, "Test Display", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
}

static void Test_Display(void *pvParameters){
	while(1){
		// Load the hex values for 4 into the display buffer, leaving room for the 0x40 pixel write command
		// at index 0
		for(int i = 0; i < 5; i++){
			display_buffer[i + 11] = disp4[(i)];
		}
		// Prepend the pixel command to display_buffer, and then send it over i2c
		display_buffer[0] = 0x40;
		ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, display_buffer, sizeof(display_buffer), -1));
		vTaskDelay(pdMS_TO_TICKS(50));
	}
	// If task ever breaks out of loop, delete it
	vTaskDelete(NULL);
}

static void init_display(void){

	// Multiplex Ration:
	// 0x00 = command mode
	// 0xA8 = set multiplex ratio 
	// 0x1F = 31 rows
	uint8_t mux_val[] = { 0x00, 0xA8, 0x1F };
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, mux_val, sizeof(mux_val), -1));

	// Com Mode Setup:
	// 0x00 = command
	// 0xDA = Com Pings Hardware Configuration Command
	// 0x02 = Sequential mode for our 64x32 screen
	uint8_t com_pin_command[] = { 0x00, 0xDA, 0x02 };
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, com_pin_command, sizeof(com_pin_command), -1));

	// Memory Mode:
	// 0x00 = command
	// 0x20 = Memory Addressing Mode, page 34
	// 0x00 = Horizontal addressing mode, automatically go down once end of column is reached
	uint8_t mem_mode[] = { 0x00, 0x20, 0x00 };
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, mem_mode, sizeof(mem_mode), -1));

	// Column Limits:
	// 0x00 = command
	// 0x21 = Set Column Address, page 34. It specifies the start and end mem address in RAM
	// 0x00 = 0
	// 0x3F = 63
	uint8_t column_addr[] = { 0x00, 0x21, 0x20, 0x5F };
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, column_addr, sizeof(column_addr), -1));

	// Page Address Limits:
	// 0x00 = command
	// 0x22 = Set page address, page 35. It specifies the start and end pages.
	// 0x00 = 0
	// 0x03 = 3 
	uint8_t pages_addr[] = { 0x00, 0x22, 0x00, 0x03 };
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, pages_addr, sizeof(pages_addr), -1));

	// Power on:
	// Continuation bit 0, Data/Command selection bit 0, + 6 more zeros (0x00)
	// 0xAF is the value to turn on the OLED
	uint8_t write_buf[2] = { 0x00, 0xAF };
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, write_buf, sizeof(write_buf), -1));

	// Charge Pump:
	// 0x00 = command
	// 0x8D = Charge pump setting
	// 0x14 = Enable Charge Pump
	uint8_t charge_pump[] = { 0x00, 0x8D, 0x14 };
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, charge_pump, sizeof(charge_pump), -1));
	
	// This block clears the screen, since display_buffer is initialized with all zeros
	// Prepend the code 0x40 to the display buffer, which says that "I'm sending raw pixel data"
	display_buffer[0] = 0x40;
	ESP_ERROR_CHECK(i2c_master_transmit(Oleddev, display_buffer, sizeof(display_buffer), -1));
}

void save_shiny_count(uint32_t Encounters)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {

        // Set key-pair "Encounters" with value Encounters, inside
        // namespace storage
        nvs_set_u32(my_handle, "Encounters", Encounters);
        // Commit the changes to my_handle
        nvs_commit(my_handle);

        // Close the handle to free up memory
        nvs_close(my_handle);
    }
}

void load_shiny_count(uint32_t* Encounters)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        // get the unsigned 32-bit int from Encounters inside our handle, which is tied to namespace storage
        nvs_get_u32(my_handle, "Encounters", Encounters);
        nvs_close(my_handle);
    }
}

static void init_i2c(void)
{

    // i2c setup
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = 10,
        .sda_io_num = 9,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
		.flags.enable_internal_pullup = true,
    };

    i2c_device_config_t Oled = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3C,
        .scl_speed_hz = 400000,
        .scl_wait_us = 0,
    };
    // I2C Setup Section
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &masterhandle)); // assign mstconfig to masterhandle
    ESP_ERROR_CHECK(i2c_master_bus_add_device(masterhandle, &Oled, &Oleddev));
}

static void init_timer(void)
{

    // gptimer setup
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1 * 1000 * 1000,
        .intr_priority = 0,
    };

    // alarm configuration
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0, // When the alarm event occurs, the timer will automatically reload to 0
        .alarm_count = 1 * 1000 * 1000, // set the alarm period. since the requency is 1000000, alarm count at 1000000 is 1s
        .flags.auto_reload_on_alarm = true, // enable auto reload function
    };

    /* Create a new clock, named shclock with the timer config we set up
   * using the struct above. Enable the clock by passing the variable (not
   * the address) to gptimer_enable, and start it with gptimer_start*/
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &shclock));

    // assign the callback function above to the on alarm event
    gptimer_event_callbacks_t cbs = {
        .on_alarm = example_timer_on_alarm_cb,
    };

    // Assign alarm configuration to our gptimer.
    ESP_ERROR_CHECK(gptimer_set_alarm_action(shclock, &alarm_config));

    // Register timer event callback functions, allowing user context to be
    // carried (our NULL value), and assign it to our gptimer.
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(shclock, &cbs, NULL));

    // enable and start our gptimer.
    ESP_ERROR_CHECK(gptimer_enable(shclock));
    ESP_ERROR_CHECK(gptimer_start(shclock));
}

static void init_nvs(void)
{

    // initialize non-volatile flash, and this function returns a value of
    // esp_err_t, which allows us to error check below
    esp_err_t err = nvs_flash_init();

    // if there are no free pages, or there is a new version that doesn't
    // match our partition table, we erase the flash
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

static void oled_display_task(void* pvParameters){
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		ESP_LOGI(TAG, "OLED updated with new count1");
	}
}
