#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "pio_matrix.pio.h"

#define NUM_PIXELS 25
#define OUT_PIN 7
#define Led_pin_red 13
#define Botao_A 5
#define Botao_B 6

static volatile uint32_t ultimo_evento_A = 0;
static volatile uint32_t ultimo_evento_B = 0;
static volatile int letra = 0;

static void gpio_irq_handler(uint gpio, uint32_t events);
void animacao_1();

double letras[10][25] = {
    {1, 1, 1, 1, 1,  // 0
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    {0, 0, 1, 0, 0,
     0, 0, 1, 1, 0,
     1, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
    1, 1, 1, 1, 1},

    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     0, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

     {1,1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      1, 1, 1, 1, 1},

     {1, 0, 0, 0, 1,
      1, 0, 0, 0, 1,
      1, 1, 1, 1, 1,
      1, 0, 0, 0, 0,
      0, 0, 0, 0, 1},

    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0,0,
     1, 1, 1, 1, 1},

    {1, 1, 1, 1, 1,  
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    {1, 1, 1, 1, 1,  
     1, 0, 0, 0, 0,
     0, 0, 0, 1, 0,
     0, 0, 1, 0, 0,
     0, 1, 0, 0, 0},

    {1, 1, 1, 1, 1, 
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1},

    {1, 1, 1, 1, 1,  
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1}
};

uint32_t matrix_rgb(double b, double r, double g) {
    unsigned char R = r * 255, G = g * 255, B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}



void piscar_led() {
    for (int i = 0; i < 5; i++) {
        gpio_put(Led_pin_red, true);
        sleep_ms(100);
        gpio_put(Led_pin_red, false);
        sleep_ms(100);
    }
    sleep_ms(500);
}
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if(gpio == Botao_A){
        if ( current_time - ultimo_evento_A > 200000) {
            ultimo_evento_A = current_time;
            letra ++; 
            if (letra >9){
                letra =9;
            }
            animacao_1();
      }  
    }
    else if (gpio == Botao_B && current_time - ultimo_evento_B > 200000) {
        ultimo_evento_B = current_time;
        letra --;
        if (letra <0){
            letra =0;
        }
         animacao_1();
    }
}
PIO pio = pio0; 
uint offset;
uint sm;
uint32_t valor_led;

int main() {
    stdio_init_all();
    gpio_init(Led_pin_red);
    gpio_set_dir(Led_pin_red, GPIO_OUT);

    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);
    gpio_init(Botao_B);
    gpio_set_dir(Botao_B, GPIO_IN);
    gpio_pull_up(Botao_B);

    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    uint32_t valor_led;

    gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);


    while (true) {
        piscar_led();
    }
}

void animacao_1() {

    for (int i = 0; i < 25; i++) {
        valor_led = matrix_rgb(letras[letra][24-i], 0, 0.0);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}
