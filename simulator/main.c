/*
#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>
#include <hardware/gpio.h>
#include <math.h>
*/
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

/*
float read_adc(uint8_t addr, uint i2c_num, uint pin, uint pga_num, uint sps){
    // /*
        PGA Selection :-
            0 : FSR = ±6.144 V(1)
            1 : FSR = ±4.096 V(1)
            2 : FSR = ±2.048 V (default)
            3 : FSR = ±1.024 V
            4 : FSR = ±0.512 V
            5 : FSR = ±0.256 V
            6 : FSR = ±0.256 V
            7 : FSR = ±0.256 V
        
        SPS Selection :-
            0 : 8 SPS
            1 : 16 SPS
            2 : 32 SPS
            3 : 64 SPS
            4 : 128 SPS (default)
            5 : 250 SPS
            6 : 475 SPS
            7 : 860 SPS
    // '*'/''

   
    uint8_t config_reg = 1;
    uint8_t conversion_reg = 0;

    uint8_t analog_pin[] = {4, 5, 6, 7};
    float voltage[] = {0.0001875, 0.000125, 0.0000625, 0.00003125, 0.000015625, 0.0000078125};

    uint8_t os = 1;
    int array[][2] = {{0b11, 2}, {0, 1}, {0, 1}, {0, 1}, {sps, 3}, {1, 1}, {pga_num, 3}, {analog_pin[pin], 3}, {os, 1}};

    uint16_t data;
    int current_bit_place = 0;
    for (int x = 0; x < 9; x++){
        data = (array[x][0] << current_bit_place) | data;
        current_bit_place += array[x][1];
    }

    uint8_t send_data[] = {config_reg, data >> 8, data & ((int) pow(2, 8) - 1)};
    uint8_t read_data[2];
    if (i2c_num == 0){
        i2c_write_blocking(i2c0, addr, send_data, 3, false);
        i2c_write_blocking(i2c0, addr, &conversion_reg, 1, false);
        i2c_read_blocking(i2c0, addr, read_data, 2 , false);
    }
    else if (i2c_num == 1){
        i2c_write_blocking(i2c1, addr, send_data, 3, false);
        i2c_write_blocking(i2c1, addr, &conversion_reg, 1, false);
        i2c_read_blocking(i2c1, addr, read_data, 2 , false);
    }


    uint16_t out_data = (read_data[0] << 8) | (read_data[1]);
    float return_data = ((int) out_data) * voltage[pga_num];

    return return_data;
}

void init_adc(){
    i2c_init(i2c1, 100000);

    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
}

int main(){
    stdio_init_all();
    float value = 0;
    init_adc();
    while(true){
        value = read_adc(72, 1, 0, 0, 7);
        float resistance_of_thermistor = (value * 10000) / (4.992 - value);
        float cal_temp = ((3950 * 298.15) / (3950 + (298.15 * log(resistance_of_thermistor / 100000)))) - 273.15;
        printf("%f\n", cal_temp);
    }
}
*/

// Define the Pin you want to use
#define OUTPUT_PIN 0

int main() {
    // Initialize standard I/O (optional, useful for debugging)
    stdio_init_all();

    // 1. Set the GPIO function to PWM
    // This connects the GPIO pin to the internal PWM hardware slice
    gpio_set_function(OUTPUT_PIN, GPIO_FUNC_PWM);

    // 2. Find out which PWM slice is connected to this GPIO
    uint slice_num = pwm_gpio_to_slice_num(OUTPUT_PIN);

    // 3. Calculate configuration for 1 MHz
    // The default system clock on Pico is 125 MHz.
    // We need to divide 125 MHz down to 1 MHz.
    // Formula: SysClock / (MsgDiv * (Wrap + 1)) = TargetFreq
    // 125,000,000 / (1.0 * (124 + 1)) = 1,000,000 Hz
    
    float clock_div = 1.0f;    // Keep divider at 1
    uint16_t wrap_val = 2499;   // 0 to 124 is 125 cycles total

    // Get default config
    pwm_config config = pwm_get_default_config();
    
    // Set the divider and the wrap value (period)
    pwm_config_set_clkdiv(&config, clock_div);
    pwm_config_set_wrap(&config, wrap_val);

    // 4. Initialize the PWM slice with this config
    pwm_init(slice_num, &config, true);

    // 5. Set the duty cycle to 50%
    // The counter goes from 0 to 124 (wrap_val).
    // Midpoint is approx 62. 
    // This sets the level at which the signal switches from High to Low.
    pwm_set_gpio_level(OUTPUT_PIN, 62);

    // The signal is now running in the background. 
    // The CPU is free to do anything else here.
    while (true) {
        tight_loop_contents();
    }
}


