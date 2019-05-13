/* CalorieLifeCam Code

   This code forms part of an Embedded Systems Project for the class ESE350
   at the University of Pennsylvania, School of Engineering & Applied Science
   by Kyler Mintah & Ransford Antwi


*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <time.h>
#include <sys/time.h>
#include "OV7670.h"
#include <sstream>
#include <WiFi.h>
#include <nvs_flash.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include "BMP.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "soc/uart_struct.h"
#include "string.h"
#include <string>
#include <iostream>
#include <inttypes.h>
#include <curl/curl.h>
#include <RESTClient.h>
#include "esp_sleep.h"
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"

// RESTClient client;
// RESTTimings *timings;
const int SIOD = 21; //SDA
const int SIOC = 22; //SCL

const int VSYNC = 34;
const int HREF = 35;

const int XCLK = 32;
const int PCLK = 33;

const int D0 = 27;
const int D1 = 17;
const int D2 = 16;
const int D3 = 19; // was 15
const int D4 = 14;
const int D5 = 13;
const int D6 = 18; // was 12
const int D7 = 4;

//Orientation Counter
int fCount = 0;

//Indicator Lights
#define RED_LED (GPIO_NUM_23)
#define GREEN_LED (GPIO_NUM_5)
#define BLUE_LED (GPIO_NUM_15)

//DIN <- MOSI 23
//CLK <- SCK 18

#define ssid1        "LifeCam"
#define password1    "12345678"
//#define ssid2        ""
//#define password2    ""``

OV7670 *camera;

WiFiMulti wifiMulti;
WiFiServer server(80);

unsigned char bmpHeader[BMP::headerSize];

static void led_state(int state1,int state2,int state3){
  gpio_pad_select_gpio(RED_LED);
  gpio_set_direction(RED_LED, GPIO_MODE_OUTPUT);
  gpio_set_level(RED_LED, state1);

  gpio_pad_select_gpio(GREEN_LED);
  gpio_set_direction(GREEN_LED, GPIO_MODE_OUTPUT);
  gpio_set_level(GREEN_LED, state2);

  gpio_pad_select_gpio(BLUE_LED);
  gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT);
  gpio_set_level(BLUE_LED, state3);
}

void serve()
{
  WiFiClient client = server.available();
  if (client)
  {
    String currentLine = "";
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        if (c == '\n')
        {
          if (currentLine.length() == 0)
          {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(
              "<style>body{margin: 0}\nimg{height: 100%; width: auto}</style>"
              "<img id='a' src='/camera' onload='this.style.display=\"initial\"; var b = document.getElementById(\"b\"); b.style.display=\"none\"; b.src=\"camera?\"+Date.now(); '>"
              "<img id='b' style='display: none' src='/camera' onload='this.style.display=\"initial\"; var a = document.getElementById(\"a\"); a.style.display=\"none\"; a.src=\"camera?\"+Date.now(); '>");
            client.println();
            client.println("<body><h1>Calorie Life Cam</h1>");
            break;
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }

        if(currentLine.endsWith("GET /camera"))
        {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:image/bmp");
            client.println();

            client.write(bmpHeader, BMP::headerSize);
            client.write(camera->frame, camera->xres * camera->yres * 2);
            led_state(0,0,0);
            led_state(0,1,0);
            led_state(0,0,0);
        }
      }
    }
    // close the connection:
    client.stop();
  }
}


void displayY8(unsigned char * frame, int xres, int yres)
{
  //tft.setAddrWindow(0, 0, yres - 1, xres - 1);
  int i = 0;
  for(int x = 0; x < xres; x++)
    for(int y = 0; y < yres; y++)
    {
      i = y * xres + x;
      unsigned char c = frame[i];
      unsigned short r = c >> 3;
      unsigned short g = c >> 2;
      unsigned short b = c >> 3;
      //tft.pushColor(r << 11 | g << 5 | b);
    }
}

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_2)
#define RXD_PIN (GPIO_NUM_25)

void init() {
    Serial.begin(115200);
//------------------UART Init----------------------->
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);

//------------------Camera Init----------------------->
    wifiMulti.addAP(ssid1, password1);
    led_state(0,0,1);
    Serial.println("Connecting Wifi...");
    if(wifiMulti.run() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }
    camera = new OV7670(OV7670::Mode::QQVGA_RGB565, SIOD, SIOC, VSYNC, HREF, XCLK, PCLK, D0, D1, D2, D3, D4, D5, D6, D7);
    BMP::construct16BitHeader(bmpHeader, camera->xres, camera->yres);
    server.begin();
    led_state(0,1,0);
    Serial.println("Server began");
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}



static void tx_task(void *k)
{
    //Do Nothing
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "PING");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *k)
{
    Serial.println("Running Rx Task");
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);

    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 50 / portTICK_RATE_MS);
            data[rxBytes] = 0;
            std::string someString();



           const char *ConstString = (char*)data;
           const char& ConstRef = *ConstString;

          if ((int)ConstRef == 'F')
          {
              fCount = fCount + 1;
          } else{
              if((int)ConstRef == 'A'){
              fCount=0;
            }
          }
          if (fCount > 120){
            led_state(0,0,1);
            const int wakeup_time_sec = 30;
            printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
            esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);
            Serial.println("Timer triggered wakeup... leaving ULP ");
            fCount = 0;
          } else{
            led_state(0,0,0);
            //Serial.println(fCount);
          }

    }
    free(data);
}
static void camera_task()
{
  led_state(0,0,0);
  while(1){
    camera->oneFrame();
    serve();
    delay(5000);
  }
}

static void demo_visual()
{

//   /* <DESC>
// * simple HTTP POST using the easy interface
// * </DESC>
// */
// CURL *easy = curl_easy_init();
// curl_mime *mime;
// curl_mimepart *part;
//
// /* Build an HTTP form with a single field named "data", */
// //mime = curl_easy_init(easy);
// // part = curl_mime_addpart(mime);
// // curl_mime_data(part, "images_file=@ccke.jpg", CURL_ZERO_TERMINATED);
// // curl_mime_data(part, "threshold=0.6", CURL_ZERO_TERMINATED);
// //  curl_mime_data(part, "classifier_ids=food", CURL_ZERO_TERMINATED);
//
// curl_easy_setopt(easy,CURLOPT_POSTFIELDS, "images_file=@ccke.jpg&threshold=0.6&classifier_ids=food");
// /* Post and send it. */
// Serial.println("Postfields in");
// //curl_easy_setopt(easy, CURLOPT_MIMEPOST, mime);
// curl_easy_setopt(easy, CURLOPT_HEADER, "apikey:GNkhFfnReDsG_PlxHNvzG8snF8Ai-f2BFAg3xbtDveKp");
// Serial.println("API Key In");
// curl_easy_setopt(easy, CURLOPT_URL, "https://gateway.watsonplatform.net/visual-recognition/api/v3/classify?version=2018-03-19");
// Serial.println("URL Reached");
// curl_easy_perform(easy);
//
// /* Clean-up. */
// curl_easy_cleanup(easy);
// Serial.println(curl_easy_getinfo(easy,CURLINFO_HTTP_CONNECTCODE));
//
// // curl_mime_free(mime);
// Serial.println("Reached");
//static const char *WATSON_TAG = "WATSON_TAG";

//
// client.setURL("https://gateway.watsonplatform.net/visual-recognition/api/v3/classify?version=2018-03-19");
// client.addHeader("apikey","GNkhFfnReDsG_PlxHNvzG8snF8Ai-f2BFAg3xbtDveKp");
// // client.addHeader("images_file","@random.jpg");
// // client.addHeader("threshold","0.7");
// // client.addHeader("classifier_ids","food");
// client.addHeader("Content-Type","application/json");
// client.post("images_file=@random.jpg&threshold=0.7&classifier_ids=food");
// ESP_LOGD(WATSON_TAG, "Result: %s", client.getResponse().c_str());
// timings->refresh();
// ESP_LOGD(WATSON_TAG, "timings: %s", timings->toString().c_str());
// Serial.println("Test Complete");


  while(1){

}
}

extern "C" void app_main()
{
    nvs_flash_init();
    init();
    initArduino();
    Serial.println("before task");
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    camera_task();
//    xTaskCreate(camera_task, "camera_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    // xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    // demo_visual();
}
