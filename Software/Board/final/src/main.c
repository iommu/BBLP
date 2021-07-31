/**
 * Copyright (c) 2017-2018 Tara Keeling
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "ssd1306.h"
#include "ssd1306_default_if.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const int I2CDisplayAddress = 0x3C;
static const int I2CDisplayWidth = 128;
static const int I2CDisplayHeight = 32;
static const int I2CResetPin = -1;

struct SSD1306_Device I2CDisplay;

void SetupDemo(struct SSD1306_Device *DisplayHandle,
               const struct SSD1306_FontDef *Font);
void SayHello(struct SSD1306_Device *DisplayHandle, const char *HelloText);

bool DefaultBusInit(void) {
  assert(SSD1306_I2CMasterInitDefault() == true);
  assert(SSD1306_I2CMasterAttachDisplayDefault(
             &I2CDisplay, I2CDisplayWidth, I2CDisplayHeight, I2CDisplayAddress,
             I2CResetPin) == true);

  return true;
}

void SetupDemo(struct SSD1306_Device *DisplayHandle,
               const struct SSD1306_FontDef *Font) {
  SSD1306_Clear(DisplayHandle, SSD_COLOR_BLACK);
  SSD1306_SetFont(DisplayHandle, Font);
}

void SayHello(struct SSD1306_Device *DisplayHandle, const char *HelloText) {
  SSD1306_FontDrawAnchoredString(DisplayHandle, TextAnchor_Center, HelloText,
                                 SSD_COLOR_WHITE);
  SSD1306_Update(DisplayHandle);
}

void app_main(void) {
  printf("Ready...\n");

  if (DefaultBusInit() == true) {
    printf("BUS Init lookin good...\n");
    printf("Drawing.\n");

    SetupDemo(&I2CDisplay, &Font_droid_sans_fallback_24x28);
    SayHello(&I2CDisplay, "Hello i2c!");

    printf("Done!\n");
  }
}