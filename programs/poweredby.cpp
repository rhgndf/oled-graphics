#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"


// Instantiate the display
ArduiPi_OLED display;


int main(int argc, char **argv)
{
								int i;

								display.init(OLED_I2C_RESET, 3); //128x64


								display.begin();

								display.clearDisplay(); // clears the screen  buffer
								display.display(); // display it (clear display)


								display.setTextSize(2);
								display.setTextColor(WHITE);
								display.setCursor(0,0);
								display.print("Powered By\nRaspberry\nPi\n3\n");
								display.display();
								return 0;
}
