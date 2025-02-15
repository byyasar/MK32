/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Copyright 2018 Gal Zaidenstein.
 */

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include "oled_tasks.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"

#include "u8g2_esp32_hal.h"
#include "keyboard_config.h"
#include "battery_monitor.h"
#include "nvs_keymaps.h"

static const char *TAG = "	OLED";

u8g2_t u8g2; // a structure which will contain all the data for one display
uint8_t prev_led = 0;

QueueHandle_t layer_recieve_q;
QueueHandle_t led_recieve_q;
QueueHandle_t cur_opsystem_q;
QueueHandle_t mouseMode_q;
QueueHandle_t fusion360Mode_q;

uint32_t battery_percent = 0;
uint32_t prev_battery_percent = 0;

uint8_t curr_layout = 0;
uint8_t curr_operation_system = 0;
uint8_t current_led = 0;
uint8_t current_mouse_mod = 1;
uint8_t mouseMode = 1;	   // 1-SCROLL 2-LEFT-RİGHT 3- UP-DOWN
uint8_t fusion360Mode = 1; // 1-zoom 2-pan

int BATT_FLAG = 0;
int DROP_H = 0;

int offset_x_batt = 0;
int offset_y_batt = 0;

#define BT_ICON 0x5e
#define BATT_ICON 0x5b
#define LOCK_ICON 0xca

// Erasing area from oled
void erase_area(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	u8g2_SetDrawColor(&u8g2, 0);
	u8g2_DrawBox(&u8g2, x, y, w, h);
	u8g2_SetDrawColor(&u8g2, 1);
}

// Function for updating the OLED
void update_oled(void)
{
#ifdef BATT_STAT
	battery_percent = get_battery_level();
#endif

	/*if (xQueueReceive(mouseMode_q, &current_mouse_mod, (TickType_t)0) && curr_layout == 1)
	{
		erase_area(0, 0, 128, 32);
		u8g2_DrawRFrame(&u8g2, 0, 0, 128, 8, 3);
		current_mouse_mod == 1 ? u8g2_DrawStr(&u8g2, 0, 7, " Scroll ") : u8g2_DrawStr(&u8g2, 0, 7, " Left ");
	}*/

	if (xQueueReceive(layer_recieve_q, &curr_layout, (TickType_t)0) || xQueueReceive(cur_opsystem_q, &curr_operation_system, (TickType_t)0)||xQueueReceive(mouseMode_q, &current_mouse_mod, (TickType_t)0))
	{
		erase_area(0, 0, 128, 32);
		u8g2_DrawRFrame(&u8g2, 0, 0, 128, 8, 3);
		curr_operation_system == 0 ? u8g2_DrawStr(&u8g2, 0, 7, " Win ") : u8g2_DrawStr(&u8g2, 0, 7, " Mac ");
		current_mouse_mod == 0 ? u8g2_DrawStr(&u8g2, 0, 14, " Scr ") : u8g2_DrawStr(&u8g2, 0,14, " L-R ");
		if (curr_layout == 0)
		{

			u8g2_DrawStr(&u8g2, 50, 7, " Ana Menu ");
			u8g2_DrawStr(&u8g2, 0, 15, " Ent | Esc |-> V- x V+ ");
			u8g2_DrawStr(&u8g2, 0, 22, "  1  |  2  |  3  | WxM |");
			u8g2_DrawStr(&u8g2, 0, 29, " Tab |  6  |  7  | Slp |");
		}
		else if (curr_layout == 1) /*|| xQueueReceive(mouseMode_q, &current_mouse_mod, (TickType_t)0)*/
		{
			u8g2_DrawStr(&u8g2, 50, 7, " Fusion 360 ");
			current_mouse_mod == 1 ? u8g2_DrawStr(&u8g2, 0, 15, " Mod | Esc | -> Scroll") : (current_mouse_mod == 2 ? u8g2_DrawStr(&u8g2, 0, 15, " Mod | Esc | ->Sol xSag") : u8g2_DrawStr(&u8g2, 0, 15, " Mod | Esc |->Asg xYuk"));
			u8g2_DrawStr(&u8g2, 0, 15, " Mod | Esc | -> Scroll");
			u8g2_DrawStr(&u8g2, 0, 22, " Zom | Rot | Pan | Mov |");
			u8g2_DrawStr(&u8g2, 0, 29, " Tab |  6  |  7  |  8  |");
		}
		else if (curr_layout == 2)
		{
			u8g2_DrawStr(&u8g2, 50, 7, " Youtube ");
			u8g2_DrawStr(&u8g2, 0, 15, " Ply | Esc |->Ileri-Geri ");
			u8g2_DrawStr(&u8g2, 0, 22, " FlS |  2  |  3  | V+  |");
			u8g2_DrawStr(&u8g2, 0, 29, " Tab |  6  |  7  | V-  |");
		}
		else
		{
			u8g2_DrawStr(&u8g2, 50, 7, " OBS ");
			u8g2_DrawStr(&u8g2, 0, 15, " Ply | Esc |-> V- x V+ ");
			u8g2_DrawStr(&u8g2, 0, 22, "  1  | Rec | Pau | Stp |");
			u8g2_DrawStr(&u8g2, 0, 29, " Tab | Mic | Cam | Scn |");
		}

		u8g2_SendBuffer(&u8g2);
	}
}

// oled on connection
void ble_connected_oled(void)
{

	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
	u8g2_DrawRFrame(&u8g2, 0, 0, 128, 8, 3);
	curr_operation_system == 0 ? u8g2_DrawStr(&u8g2, 0, 7, " Win ") : u8g2_DrawStr(&u8g2, 0, 7, " Mac ");
	u8g2_DrawStr(&u8g2, 50, 7, " Ana Menu ");
	u8g2_DrawStr(&u8g2, 0, 15, " Ent | Esc |-> V- x V+ ");
	u8g2_DrawStr(&u8g2, 0, 22, "  1  |  2  |  3  | WxM |");
	u8g2_DrawStr(&u8g2, 0, 29, " Tab |  6  |  7  | Slp |");

	// u8g2_DrawStr(&u8g2, 0, 16, layer_names_arr[current_layout]);

	// u8g2_SetFont(&u8g2, u8g2_font_open_iconic_all_1x_t);
	// u8g2_DrawGlyph(&u8g2, 110 + offset_x_batt , 8 + offset_y_batt, BATT_ICON);
	// u8g2_DrawGlyph(&u8g2, 120 + offset_x_batt, 8 + offset_y_batt , BT_ICON);

	//	if(CHECK_BIT(curr_led,0)!=0){
	//		u8g2_SetFont(&u8g2, u8g2_font_5x7_tf );
	//		u8g2_DrawStr(&u8g2, 0,31,"NUM");
	//		u8g2_SetFont(&u8g2, u8g2_font_open_iconic_all_1x_t );
	//		u8g2_DrawGlyph(&u8g2, 16,32,LOCK_ICON);
	//	}
	//
	//	if(CHECK_BIT(curr_led,1)!=0){
	//		u8g2_SetFont(&u8g2, u8g2_font_5x7_tf );
	//		u8g2_DrawStr(&u8g2, 27,31,"CAPS");
	//		u8g2_SetFont(&u8g2, u8g2_font_open_iconic_all_1x_t );
	//		u8g2_DrawGlyph(&u8g2,48,32,LOCK_ICON);
	//	}
	//	if(CHECK_BIT(curr_led,2)!=0){
	//		u8g2_SetFont(&u8g2, u8g2_font_5x7_tf );
	//		u8g2_DrawStr(&u8g2, 57,31,"SCROLL");
	//		u8g2_SetFont(&u8g2, u8g2_font_open_iconic_all_1x_t );
	//		u8g2_DrawGlyph(&u8g2,88,32,LOCK_ICON);
	//	}

	/*u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
	char buf[sizeof(uint32_t)];
	snprintf(buf, sizeof(uint32_t), "%d", battery_percent);
	u8g2_DrawStr(&u8g2, + offset_x_batt, + offset_y_batt, "%");
	if (battery_percent < 100) {
		u8g2_DrawStr(&u8g2, + offset_x_batt, 7 + offset_y_batt , buf);
	} else {
		u8g2_DrawStr(&u8g2, 85 + offset_x_batt, 7 + offset_y_batt, "100");
	}*/
	u8g2_SendBuffer(&u8g2);
}

// Slave oled display
void ble_slave_oled(void)
{
	battery_percent = get_battery_level();

	if (battery_percent != prev_battery_percent)
	{
		u8g2_ClearBuffer(&u8g2);
		u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
		u8g2_DrawStr(&u8g2, 0, 6, GATTS_TAG);
		u8g2_DrawStr(&u8g2, 0, 14, "Slave pad 1");
		u8g2_SetFont(&u8g2, u8g2_font_open_iconic_all_1x_t);
		u8g2_DrawGlyph(&u8g2, 110 + offset_x_batt, 8 + offset_y_batt, BATT_ICON);
		u8g2_DrawGlyph(&u8g2, 120 + offset_x_batt, 8 + offset_y_batt, BT_ICON);

		u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
		char buf[sizeof(uint32_t)];
		snprintf(buf, sizeof(uint32_t), "%d", battery_percent);
		u8g2_DrawStr(&u8g2, 103 + offset_x_batt, 7 + offset_y_batt, "%");
		if ((battery_percent < 100) && (battery_percent - prev_battery_percent >= 2))
		{
			u8g2_SetDrawColor(&u8g2, 0);
			u8g2_DrawBox(&u8g2, 85 + offset_x_batt, 90 + offset_y_batt, 0, 7);
			u8g2_SetDrawColor(&u8g2, 1);
			u8g2_DrawStr(&u8g2, 90 + offset_x_batt, 7 + offset_y_batt, buf);
			u8g2_SendBuffer(&u8g2);
		}
		if ((battery_percent < 100) && (battery_percent - prev_battery_percent >= 2))
		{
			erase_area(85 + offset_x_batt, 0 + offset_y_batt, 15, 7);
			u8g2_DrawStr(&u8g2, 90, 7, buf);
			u8g2_SendBuffer(&u8g2);
		}
		if ((battery_percent > 100) && (BATT_FLAG = 0))
		{
			erase_area(85 + offset_x_batt, 0 + offset_y_batt, 15, 7);
			u8g2_DrawStr(&u8g2, 85 + offset_x_batt, 7 + offset_y_batt, "100");
			BATT_FLAG = 1;
			u8g2_SendBuffer(&u8g2);
		}
		if (battery_percent == 100)
		{
			erase_area(85 + offset_x_batt, 0 + offset_y_batt, 15, 7);
			u8g2_DrawStr(&u8g2, 85 + offset_x_batt, 7 + offset_y_batt, "100");
			u8g2_SendBuffer(&u8g2);
		}
		prev_battery_percent = battery_percent;
	}
}

// Waiting for connecting animation
void waiting_oled(void)
{
	char waiting[] = "Baglanti \nBekleniyor ";
	char conn[] = "Baglaniyor";

#ifdef BATT_STAT
	battery_percent = get_battery_level();
#endif

	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_open_iconic_all_1x_t);
	u8g2_DrawGlyph(&u8g2, 110 + offset_x_batt, 8 + offset_y_batt, BATT_ICON);
	u8g2_DrawGlyph(&u8g2, 120 + offset_x_batt, 8 + offset_y_batt, BT_ICON);
	u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
	u8g2_DrawStr(&u8g2, 0, 6, GATTS_TAG);

	char buf[sizeof(uint32_t)];
	snprintf(buf, sizeof(uint32_t), "%d", battery_percent);
	u8g2_DrawStr(&u8g2, 103 + offset_x_batt, 7 + offset_y_batt, "%");
	if ((battery_percent < 100) && (battery_percent - prev_battery_percent >= 2))
	{
		u8g2_DrawStr(&u8g2, 90 + offset_x_batt, 7 + offset_y_batt, buf);
	}
	if (battery_percent < 100)
	{
		u8g2_DrawStr(&u8g2, 90 + offset_x_batt, 7 + offset_y_batt, buf);
	}
	else
	{
		u8g2_DrawStr(&u8g2, 85 + offset_x_batt, 7 + offset_y_batt, "100");
	}

	for (int i = 0; i < 3; i++)
	{
		u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);
		u8g2_DrawStr(&u8g2, 0, 15, waiting);
		u8g2_DrawStr(&u8g2, 0, 30, conn);
		u8g2_SendBuffer(&u8g2);
		vTaskDelay(100 / portTICK_PERIOD_MS);
		strcat(conn, ".");
	}
}

// shut down OLED
void deinit_oled(void)
{

	u8g2_ClearBuffer(&u8g2);
	u8g2_SetFont(&u8g2, u8g2_font_5x7_tf);
	u8g2_DrawStr(&u8g2, 0, 6, GATTS_TAG);
	u8g2_DrawStr(&u8g2, 0, 26, "Going to sleep!");
	u8g2_SendBuffer(&u8g2);
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	u8g2_ClearDisplay(&u8g2);
	u8g2_SetPowerSave(&u8g2, 1);
	i2c_driver_delete(I2C_NUM_0);
}

// initialize oled
void init_oled(const u8g2_cb_t *rotation)
{

	layer_recieve_q = xQueueCreate(32, sizeof(uint8_t));
	led_recieve_q = xQueueCreate(32, sizeof(uint8_t));
	cur_opsystem_q = xQueueCreate(32, sizeof(uint8_t)); 
	mouseMode_q = xQueueCreate(32, sizeof(uint8_t)); //mouseMode_q

	ESP_LOGI(TAG, "Setting up oled");

	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	u8g2_esp32_hal.sda = OLED_SDA_PIN;
	u8g2_esp32_hal.scl = OLED_SCL_PIN;
	u8g2_esp32_hal_init(u8g2_esp32_hal);

	if ((rotation == DEG90) || rotation == DEG270)
	{

		offset_x_batt = -85;
		offset_y_batt = 120;
	}

	/* burayı değiştirdim
	u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, rotation,
			u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure*/

	u8g2_Setup_ssd1306_i2c_128x32_univision_f(&u8g2, rotation,
											  u8g2_esp32_i2c_byte_cb, u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure

	u8x8_SetI2CAddress(&u8g2.u8x8, 0x78);
	u8g2_InitDisplay(&u8g2);	 // send init sequence to the display, display is in sleep mode after this,
	u8g2_SetPowerSave(&u8g2, 0); // wake up display

	u8g2_ClearBuffer(&u8g2);
}
