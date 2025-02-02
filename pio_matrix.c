#include <stdio.h>       // Biblioteca padrão para entrada e saída
#include <stdlib.h>      // Biblioteca padrão para alocação de memória, conversões e utilidades
#include <math.h>        // Biblioteca matemática para cálculos numéricos
#include "pico/stdlib.h" // Biblioteca padrão do Raspberry Pi Pico
#include "hardware/pio.h" // Biblioteca para manipulação do PIO (Programável I/O)
#include "hardware/clocks.h" // Biblioteca para manipulação de clocks no Pico
#include "hardware/adc.h" // Biblioteca para controle do ADC (Conversor Analógico-Digital)
#include "pico/bootrom.h" // Biblioteca para acesso a funções de boot
#include "pio_matrix.pio.h" // Arquivo de programa PIO externo para controle da matriz de LEDs

// Definição do número de LEDs na matriz e do pino de saída
#define NUM_PIXELS 25
#define OUT_PIN 7

// Definição dos pinos para LED e botões
#define Led_pin_red 13
#define Botao_A 5
#define Botao_B 6

// Variáveis voláteis para armazenar o tempo do último evento dos botões
static volatile uint32_t ultimo_evento_A = 0;
static volatile uint32_t ultimo_evento_B = 0;
static volatile int letra = 0; // Índice da letra exibida na matriz de LEDs

// Protótipo da função de interrupção do GPIO
static void gpio_irq_handler(uint gpio, uint32_t events);

// Protótipo da função de animação
void animacao_1();

// Matriz que representa 10 caracteres (0 a 9) em uma matriz 5x5
double letras[10][25] = {
    {1, 1, 1, 1, 1,  
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // Representação do número 0

    {0, 0, 1, 0, 0,
     0, 0, 1, 1, 0,
     1, 0, 1, 0, 0,
     0, 0, 1, 0, 0,
     1, 1, 1, 1, 1}, // Representação do número 1

    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     0, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // Representação do número 2

    {1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1}, // Representação do número 3

    {1, 0, 0, 0, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     0, 0, 0, 0, 1}, // Representação do número 4

    {1, 1, 1, 1, 1,
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1}, // Representação do número 5

    {1, 1, 1, 1, 1,  
     0, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // Representação do número 6

    {1, 1, 1, 1, 1,  
     1, 0, 0, 0, 0,
     0, 0, 0, 1, 0,
     0, 0, 1, 0, 0,
     0, 1, 0, 0, 0}, // Representação do número 7

    {1, 1, 1, 1, 1, 
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1}, // Representação do número 8

    {1, 1, 1, 1, 1,  
     1, 0, 0, 0, 1,
     1, 1, 1, 1, 1,
     1, 0, 0, 0, 0,
     1, 1, 1, 1, 1}  // Representação do número 9
};

// Função para converter valores de cor em um formato compatível com a matriz de LEDs
uint32_t matrix_rgb(double b, double r, double g) {
    unsigned char R = r * 255, G = g * 255, B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// Função que pisca o LED vermelho 5 vezes
void piscar_led() {
    for (int i = 0; i < 5; i++) {
        gpio_put(Led_pin_red, true);
        sleep_ms(100);
        gpio_put(Led_pin_red, false);
        sleep_ms(100);
    }
    sleep_ms(500);
}

// Função de interrupção acionada pelo pressionamento dos botões
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (gpio == Botao_A) {
        if (current_time - ultimo_evento_A > 200000) {
            ultimo_evento_A = current_time;
            letra++; 
            if (letra > 9) letra = 9;
            animacao_1();
        }  
    }
    else if (gpio == Botao_B && current_time - ultimo_evento_B > 200000) {
        ultimo_evento_B = current_time;
        letra--;
        if (letra < 0) letra = 0;
        animacao_1();
    }
}

// Variáveis globais para controle do PIO
PIO pio = pio0; 
uint offset;
uint sm;
uint32_t valor_led;

int main() {
    stdio_init_all(); // Inicializa entrada e saída padrão
    gpio_init(Led_pin_red);
    gpio_set_dir(Led_pin_red, GPIO_OUT);

    // Configuração dos botões como entrada com pull-up
    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);
    gpio_init(Botao_B);
    gpio_set_dir(Botao_B, GPIO_IN);
    gpio_pull_up(Botao_B);

    // Inicialização do PIO para a matriz de LEDs
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    // Configuração das interrupções dos botões
    gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        piscar_led(); // Pisca o LED em loop
    }
}

// Função que envia os valores da matriz para os LEDs
void animacao_1() {
    for (int i = 0; i < 25; i++) {
        valor_led = matrix_rgb(letras[letra][24-i], 0, 0.0);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}
