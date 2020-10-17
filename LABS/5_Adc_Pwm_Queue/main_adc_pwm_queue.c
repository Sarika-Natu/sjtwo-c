#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "adc.h"
#include "lpc40xx.h"
#include "pwm1.h"
#include "queue.h"

// This is the queue handle we will need for the xQueue Send/Receive API
static QueueHandle_t adc_to_pwm_task_queue;

#define ADC_MAX_VALUE 4095.0
#define VREF 3.3
#define PWM_FREQ 1
#define DUTY_CYCLE_50 50
#define PERCENTILE 100
#define RGB_MAX 255
#define ANGLE_MAX 360

#define PART3_1

typedef struct {
    gpio__port_e port;
    uint8_t pin;
} port_pin_s;

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_value;

const uint8_t HSVlights[61] = {0,   4,   8,   13,  17,  21,  25,  30,  34,  38,  42,  47,  51,  55,  59,  64,
                               68,  72,  76,  81,  85,  89,  93,  98,  102, 106, 110, 115, 119, 123, 127, 132,
                               136, 140, 144, 149, 153, 157, 161, 166, 170, 174, 178, 183, 187, 191, 195, 200,
                               204, 208, 212, 217, 221, 225, 229, 234, 238, 242, 246, 251, 255};

static void pin_configure_pwm_channel_as_io_pin(void);
void pwm_task(void *p);
static rgb_value trueHSV(uint8_t angle);
static void pin_configure_adc_channel_as_io_pin(void);
void adc_task(void *p);

static void pin_configure_pwm_channel_as_io_pin(void) {
    const uint32_t func_mask = 0x07;
    const uint32_t enable_pwm_func = 0x01;
    LPC_IOCON->P2_0 &= ~func_mask;
    LPC_IOCON->P2_0 |= enable_pwm_func;

    LPC_IOCON->P2_1 &= ~func_mask;
    LPC_IOCON->P2_1 |= enable_pwm_func;

    LPC_IOCON->P2_2 &= ~func_mask;
    LPC_IOCON->P2_2 |= enable_pwm_func;

    LPC_IOCON->P2_4 &= ~func_mask;
    LPC_IOCON->P2_4 |= enable_pwm_func;
}
#ifdef PART3_1
/*Source: https://www.instructables.com/id/How-to-Make-Proper-Rainbow-and-Random-Colors-With-/*/
static rgb_value trueHSV(uint8_t angle) {
    rgb_value value;
    uint8_t red, green, blue;

    // Calculate the RGB colors.
    if (angle < 60) {
        red = 255;
        green = HSVlights[angle];
        blue = 0;

    } else if (angle < 120) {
        red = HSVlights[120 - angle];
        green = 255;
        blue = 0;

    } else if (angle < 180) {
        red = 0;
        green = 255;
        blue = HSVlights[angle - 120];

    } else if (angle < 240) {
        red = 0;
        green = HSVlights[240 - angle];
        blue = 255;

    } else if (angle < 300) {
        red = HSVlights[angle - 240];
        green = 0;
        blue = 255;

    } else {
        red = 255;
        green = 0;
        blue = HSVlights[360 - angle];
    }
    value.red = red;
    value.green = green;
    value.blue = blue;
    return value;
}
#endif
void pwm_task(void *p) {
    int adc_reading;
    uint32_t MR_Readch1, MR_Readch2, MR_Readch3, MR_Readch4;
    uint32_t MR_Read0;
    // Locate a GPIO pin that a PWM channel will control
    // NOTE You can use gpio__construct_with_function() API from gpio.h
    // TODO Write this function yourself

    pin_configure_pwm_channel_as_io_pin();
    pwm1__init_single_edge(PWM_FREQ);
    // We only need to set PWM configuration once, and the HW will drive
    // the GPIO at 1000Hz, and control set its duty cycle to 50%
#ifdef Part3_0
    pwm1__set_duty_cycle(PWM1__2_0, DUTY_CYCLE_50);
  pwm1__set_duty_cycle(PWM1__2_1, DUTY_CYCLE_50);
  pwm1__set_duty_cycle(PWM1__2_2, DUTY_CYCLE_50);
  pwm1__set_duty_cycle(PWM1__2_4, DUTY_CYCLE_50);
#endif

    // Continue to vary the duty cycle in the loop

    while (1) {
        if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading, 100)) {

#ifdef PART3_1
            /****
             * PWM1__2_1 - brown wire -Red color
             * PWM1__2_2 - blue wire - Blue color
             * PWM1__2_4 - green wire -Green color
             * Truth table
             * Red  Green   Blue    Merge col
             * ON    ON      OFF     YELLOW
             * OFF   ON      ON      CYAN
             * ON    OFF     ON      MAGENTA
             * ON    ON      ON      WHITE
             */

            pin_configure_pwm_channel_as_io_pin();

            pwm1__set_duty_cycle(PWM1__2_1, DUTY_CYCLE_50);
            pwm1__set_duty_cycle(PWM1__2_2, DUTY_CYCLE_50);
            pwm1__set_duty_cycle(PWM1__2_4, DUTY_CYCLE_50);

            uint8_t rgb_angle = 0;
            rgb_value rgb_color = {0, 0, 0};

            // calculate angle to get respective rgb color value
            rgb_angle = (adc_reading * ANGLE_MAX) / ADC_MAX_VALUE;
            rgb_color = trueHSV(rgb_angle);

            // calculate rgb percentage for duty cycle
            rgb_color.red = (rgb_color.red / RGB_MAX) * PERCENTILE;
            rgb_color.green = (rgb_color.green / RGB_MAX) * PERCENTILE;
            rgb_color.blue = (rgb_color.blue / RGB_MAX) * PERCENTILE;

            pwm1__set_duty_cycle(PWM1__2_1, rgb_color.red);
            pwm1__set_duty_cycle(PWM1__2_4, rgb_color.green);
            pwm1__set_duty_cycle(PWM1__2_2, rgb_color.blue);

#endif

#ifdef Part3_0
            uint8_t adc_percentage = (u_int8_t)(((double)adc_reading / ADC_MAX_VALUE) * PERCENTILE);
      fprintf(stderr, "The value adc_percentage is %d\n\n", adc_percentage);
      pwm1__set_duty_cycle(PWM1__2_0, adc_percentage);
      pwm1__set_duty_cycle(PWM1__2_1, adc_percentage);
      pwm1__set_duty_cycle(PWM1__2_2, adc_percentage);
      pwm1__set_duty_cycle(PWM1__2_4, adc_percentage);
#endif
            MR_Read0 = LPC_PWM1->MR0;
            fprintf(stderr, "The value at MR0 is %ld\n", MR_Read0);
            MR_Readch1 = LPC_PWM1->MR1;
            fprintf(stderr, "The value at MR1 is %ld\n", MR_Readch1);
            MR_Readch2 = LPC_PWM1->MR2;
            fprintf(stderr, "The value at MR2 is %ld\n", MR_Readch2);
            MR_Readch3 = LPC_PWM1->MR3;
            fprintf(stderr, "The value at MR3 is %ld\n", MR_Readch3);
            MR_Readch4 = LPC_PWM1->MR5;
            fprintf(stderr, "The value at MR5 is %ld\n\n", MR_Readch4);
        }
#ifdef PART0
        uint8_t percent = 0;
    pwm1__set_duty_cycle(PWM1__2_1, percent);
    // fprintf(stderr, "%d\n", percent);
    if (++percent > PERCENTILE) {
      percent = 0;
    }
    vTaskDelay(100);
#endif
    }
}

static void pin_configure_adc_channel_as_io_pin(void) {
    const uint32_t func_mask = 0x07;
    const uint32_t enable_adc_func = 0x01;
    const uint32_t enable_analog_mode = (1u << 7);
    // const uint32_t enable_pull_mode = (0b01 << 3);
    // const uint32_t enable_pull_mask = (0b11 << 3);

    LPC_IOCON->P0_25 &= ~func_mask;
    LPC_IOCON->P0_25 |= enable_adc_func;     // setting FUNC bits of IOCON
    LPC_IOCON->P0_25 &= ~enable_analog_mode; // setting pin to analog mode
    // LPC_IOCON->P0_25 &= ~enable_pull_mask;
    // LPC_IOCON->P0_25 |= enable_pull_mode;
}
void adc_task(void *p) {
    int adc_reading;
    double adc_voltage = 0;
    adc__initialize();
    int64_t average = 0;
    int i = 0;

    // TODO This is the function you need to add to adc.h
    // You can configure burst mode for just the channel you are using
    adc__enable_burst_mode(ADC__CHANNEL_2);

    // Configure a pin, such as P1.31 with FUNC 011 to route this pin as ADC channel 5
    // You can use gpio__construct_with_function() API from gpio.h
    pin_configure_adc_channel_as_io_pin(); // TODO You need to write this function

    while (1) {
        // Get the ADC reading using a new routine you created to read an ADC burst reading
        // TODO: You need to write the implementation of this function
        while (i < 30) {
            const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_2);
            // const uint16_t adc_value = adc__get_adc_value(ADC__CHANNEL_5);
            average += adc_value;
            i++;
        }

        adc_reading = (int)((double)average / 30.0);
        average = 0;
        i = 0;
        adc_voltage = ((double)adc_reading / ADC_MAX_VALUE) * VREF;
        fprintf(stderr, "The voltage read using the potentiometer is : %f volts\n", adc_voltage);
        // fprintf(stderr, "the ADC reading is: %d\n", adc_reading);

        xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);

        vTaskDelay(150);
    }
}

int main(void) {
    static port_pin_s flashy_led[4] = {{GPIO__PORT_2, 3}, {GPIO__PORT_1, 26}, {GPIO__PORT_1, 24}, {GPIO__PORT_1, 18}};

    for (u_int8_t index = 0; index < 4; index++) {
        gpio_s port_pin = gpio__construct(flashy_led[index].port, flashy_led[index].pin);
        gpio__set_as_output(port_pin);
        gpio__set(port_pin);
    }
    // Queue will only hold 1 integer
    adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int));
    xTaskCreate(pwm_task, "pwm_task", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
    xTaskCreate(adc_task, "adc_task", (512U * 4) / sizeof(void *), (void *)NULL, PRIORITY_LOW, NULL);
    puts("Starting RTOS");
    vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

    return 0;
}

