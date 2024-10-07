#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include <string.h>
#include "freertos/event_groups.h"
#include "esp_wifi.h"

#define HIGH 1
#define LOW 0
#define LED_GREEN GPIO_NUM_2
#define LED_YELLOW GPIO_NUM_4
#define LED_RED GPIO_NUM_5


unsigned char previousred = LOW;
unsigned char previousyellow = LOW;
unsigned char previousgreen = LOW;

static esp_mqtt_client_handle_t client;

static const char *TAG = "mqtt_example";

// دالة لنشر حالة الـ LED إلى الـ Broker في topic مخصص
void publish_led_status(const char* led, const char* status) {
    esp_mqtt_client_publish(client, led, status, 0, 1, 0);
    printf("Published %s status: %s\n", led, status);
}

// دالة للتحكم في تشغيل أو إطفاء LED معين
void control_led(const char* led, int level) {
    if (strcmp(led, "RED") == 0) {
        gpio_set_level(LED_RED, level);  // تشغيل أو إطفاء LED الأحمر
        publish_led_status("RED", level ? "ON" : "OFF");     
        
    } else if (strcmp(led, "YELLOW") == 0) {
        gpio_set_level(LED_YELLOW, level);  // تشغيل أو إطفاء LED الأصفر
        publish_led_status("YELLOW", level ? "ON" : "OFF");
        
        
    } else if (strcmp(led, "GREEN") == 0) {

        gpio_set_level(LED_GREEN, level);  // تشغيل أو إطفاء LED الأخضر
        publish_led_status("GREEN", level ? "ON" : "OFF");     
        
        
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
// معالج أحداث MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            // الاشتراك في المواضيع الخاصة بكل LED عند الاتصال بـ Broker
            esp_mqtt_client_subscribe(client, "RED", 0);
            esp_mqtt_client_subscribe(client, "YELLOW", 0);
            esp_mqtt_client_subscribe(client, "GREEN", 0);
            printf("Connected to MQTT broker and subscribed to LED topics\n");
            break;

        case MQTT_EVENT_DATA:
            // عند استقبال رسالة، تحديد الـ topic وتشغيل أو إطفاء الـ LED المناسب
            // التعامل مع مواضيع LED الأحمر
            if (strncmp(event->topic, "RED", event->topic_len) == 0) {
                if (strncmp(event->data, "ON", event->data_len) == 0 && previousred != HIGH) {
                    previousred = HIGH;
                    printf("Received message on topic: %.*s\n", event->topic_len, event->topic);
                    control_led("RED", HIGH);  // تشغيل LED الأحمر
                } else if (strncmp(event->data, "OFF", event->data_len) == 0 && previousred != LOW) {
                    previousred = LOW;
                    printf("Received message on topic: %.*s\n", event->topic_len, event->topic);
                    control_led("RED", LOW);  // إطفاء LED الأحمر
                }
            }

            // التعامل مع مواضيع LED الأصفر
            else if (strncmp(event->topic, "YELLOW", event->topic_len) == 0 ) {
                if (strncmp(event->data, "ON", event->data_len) == 0 && previousyellow != HIGH) {
                    previousyellow = HIGH;
                    printf("Received message on topic: %.*s\n", event->topic_len, event->topic);
                    control_led("YELLOW", HIGH);  // تشغيل LED الأصفر
                } else if (strncmp(event->data, "OFF", event->data_len) == 0 && previousyellow != LOW) {
                    previousyellow = LOW;
                    printf("Received message on topic: %.*s\n", event->topic_len, event->topic);
                    control_led("YELLOW", LOW);  // إطفاء LED الأصفر
                }
            }

            // التعامل مع مواضيع LED الأخضر
            else if (strncmp(event->topic, "GREEN", event->topic_len) == 0 ) {
                if (strncmp(event->data, "ON", event->data_len) == 0 && previousgreen != HIGH) {
                    previousgreen = HIGH;
                    printf("Received message on topic: %.*s\n", event->topic_len, event->topic);
                    control_led("GREEN", HIGH);  // تشغيل LED الأخضر
                } else if (strncmp(event->data, "OFF", event->data_len) == 0 && previousgreen != LOW) {
                    previousgreen = LOW;
                    printf("Received message on topic: %.*s\n", event->topic_len, event->topic);
                    control_led("GREEN", LOW);  // إطفاء LED الأخضر
                }
            }
            break;

        default:
            break;
    }
}

// إعداد الـ GPIO الخاصة بـ LEDs
void init_gpio(void) {
    gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_YELLOW, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);
}

void app_main(void)
{
    
     init_gpio();

      // إعداد الـ MQTT Client

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org/",  // استبدل this بـ URI الخاص بالـ broker
    };

    client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_mqtt_client_start(client);

     while (1) {
        vTaskDelay(5000);  
    }  
}
