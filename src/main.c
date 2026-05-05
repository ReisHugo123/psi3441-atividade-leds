#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

/* Aliases no zephyr.dts da FRDM-KL25Z (Zephyr 4.2) */
#define LED_GREEN_NODE  DT_ALIAS(led0)  /* Verde     - PTB19 */
#define LED_BLUE_NODE   DT_ALIAS(led1)  /* Azul      - PTD1  */
#define LED_RED_NODE    DT_ALIAS(led2)  /* Vermelho  - PTB18 */

/* Especificações dos GPIOs (porta + pino + flags) extraídas do DT */
static const struct gpio_dt_spec led_red   = GPIO_DT_SPEC_GET(LED_RED_NODE,   gpios);
static const struct gpio_dt_spec led_green = GPIO_DT_SPEC_GET(LED_GREEN_NODE, gpios);
static const struct gpio_dt_spec led_blue  = GPIO_DT_SPEC_GET(LED_BLUE_NODE,  gpios);

/* Estados do semáforo */
typedef enum {
    ESTADO_VERMELHO,
    ESTADO_VERDE,
    ESTADO_AMARELO,
} estado_t;

/* Tempos de cada fase em ms */
#define TEMPO_VERMELHO_MS  5000
#define TEMPO_VERDE_MS     5000
#define TEMPO_AMARELO_MS   2000

/* Aplica o estado nos LEDs (apaga tudo e acende o que precisa) */
static void aplicar_estado(estado_t estado)
{
    /* Usa gpio_pin_set_dt no lugar do toggle, como a atividade pede */
    gpio_pin_set_dt(&led_red,   0);
    gpio_pin_set_dt(&led_green, 0);
    gpio_pin_set_dt(&led_blue,  0);

    switch (estado) {
        case ESTADO_VERMELHO:
            gpio_pin_set_dt(&led_red, 1);
            break;
        case ESTADO_VERDE:
            gpio_pin_set_dt(&led_green, 1);
            break;
        case ESTADO_AMARELO:
            /* Amarelo = vermelho + verde (mistura aditiva) */
            gpio_pin_set_dt(&led_red,   1);
            gpio_pin_set_dt(&led_green, 1);
            break;
    }
}

/* Retorna o tempo de espera de cada estado */
static uint32_t tempo_estado(estado_t estado)
{
    switch (estado) {
        case ESTADO_VERMELHO: return TEMPO_VERMELHO_MS;
        case ESTADO_VERDE:    return TEMPO_VERDE_MS;
        case ESTADO_AMARELO:  return TEMPO_AMARELO_MS;
        default:              return 1000;
    }
}

/* Calcula o próximo estado: VERMELHO -> VERDE -> AMARELO -> VERMELHO ... */
static estado_t proximo_estado(estado_t atual)
{
    switch (atual) {
        case ESTADO_VERMELHO: return ESTADO_VERDE;
        case ESTADO_VERDE:    return ESTADO_AMARELO;
        case ESTADO_AMARELO:  return ESTADO_VERMELHO;
        default:              return ESTADO_VERMELHO;
    }
}

void main(void)
{
    /* Confere se os 3 dispositivos GPIO estão prontos */
    if (!gpio_is_ready_dt(&led_red) ||
        !gpio_is_ready_dt(&led_green) ||
        !gpio_is_ready_dt(&led_blue)) {
        printk("Erro: GPIO nao pronto\n");
        return;
    }

    /* Configura os 3 pinos como saída, iniciando apagados */
    gpio_pin_configure_dt(&led_red,   GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_green, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_blue,  GPIO_OUTPUT_INACTIVE);

    printk("Semaforo iniciado\n");

    estado_t estado = ESTADO_VERMELHO;

    while (1) {
        aplicar_estado(estado);
        k_msleep(tempo_estado(estado));
        estado = proximo_estado(estado);
    }
}