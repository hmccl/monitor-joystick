#include "hardware/adc.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include <hardware/gpio.h>
#include <lwip/tcpbase.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define JOYSTICK_VRX 27 // GPIO conectado ao eixo horizontal do Joystick
#define JOYSTICK_VRY 26 // GPIO conectado ao eixo vertical do Joystick

#define JOYSTICK_MAX_VALUE 80
#define JOYSTICK_MIN_VALUE 20

// Variáveis para o estado dos Botões
static volatile uint8_t vrx = 0;
static volatile uint8_t vry = 0;

// Buffer para resposta HTTP
char http_response[4096];

// Função que lê a posição do Joystick no eixo x
uint8_t read_joystick_x() {
  adc_select_input(1);
  return adc_read() * 100 / 4095;
}

// Função que lê a posição do Joystick no eixo y
uint8_t read_joystick_y() {
  adc_select_input(0);
  return adc_read() * 100 / 4095;
}

// Função que cria a resposta HTTP
void create_html_response() {
  // Corpo da página HTML
  char html_body[2048];
  snprintf(
      html_body, sizeof(html_body),
      "<!DOCTYPE html>"
      "<html lang=\"pt\">"
      "  <head>"
      "    <meta charset=\"UTF-8\">"
      "    <title>Joystick</title>"
      "    <style>"
      "      body { font-family: sans-serif; text-align: center; margin-top: "
      "2rem; }"
      "      .joy-st { display: inline-block; margin: 1rem; padding: 1rem; "
      "border-radius: 0.5rem; "
      "                width: 20rem; }"
      "      h2 { margin-top: 0; }"
      "    </style>"
      "  </head>"
      "  <body>"
      "    <div class=\"joy-st\" style=\"background-color: lavender; "
      "color: black;\">"
      "      <h2>Eixo X</h2>"
      "      <p id=\"pos-x\">0</p>"
      "    </div>"
      "    <div class=\"joy-st\" style=\"background-color: lavender; "
      "color: black;\">"
      "      <h2>Eixo Y</h2>"
      "      <p id=\"pos-y\">0</p>"
      "    </div>"
      "    <div class=\"joy-st\" style=\"background-color: lavender; "
      "color: black;\">"
      "      <h2 id=\"dir\">Centro</h2>"
      "      <p>Direção</p>"
      "    </div>"
      ""
      "    <script>"
      "      function direction(x, y) {"
      "        const center = {x: 50, y: 50};"
      "        const lim = 15;"
      "        const dx = x - center.x;"
      "        const dy = y - center.y;"
      "        if (Math.abs(dx) < lim && Math.abs(dy) < lim) {"
      "          return 'Centro';"
      "        }"
      "        if (Math.abs(Math.abs(dx) - Math.abs(dy)) < lim) {"
      "          if (dx > 0 && dy > 0) return 'Nordeste';"
      "          if (dx < 0 && dy > 0) return 'Noroeste';"
      "          if (dx > 0 && dy < 0) return 'Sudeste';"
      "          if (dx < 0 && dy < 0) return 'Sudoeste';"
      "        } else {"
      "          if (Math.abs(dx) > Math.abs(dy)) {"
      "            return dx > 0 ? 'Leste' : 'Oeste';"
      "          } else {"
      "            return dy > 0 ? 'Norte' : 'Sul';"
      "          }"
      "        }"
      "      }"
      "      function updatePosition() {"
      "        fetch('/api/joystick')"
      "          .then(response => response.json())"
      "          .then(data => {"
      "            document.getElementById('pos-x').textContent = "
      "data.axiX.pos;"
      "            document.getElementById('pos-y').textContent = "
      "data.axiY.pos;"
      "            document.getElementById('dir').textContent = "
      "direction(data.axiX.pos, data.axiY.pos);"
      "          });"
      "      }"
      "      updatePosition();"
      "      setInterval(updatePosition, 1000);"
      "    </script>"
      "  </body>"
      "</html>");

  // Tamanho da página HTML
  int content_length = strlen(html_body);

  // Cabeçalho HTTP + Corpo HTML
  snprintf(http_response, sizeof(http_response),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: %d\r\n"
           "\r\n"
           "%s",
           content_length, html_body);
}

// Função que cria a resposta JSON para a API
void create_json_response() {
  // Cria o JSON
  char json_body[512];
  snprintf(json_body, sizeof(json_body),
           "{"
           "  \"axiX\": {"
           "    \"pos\": %d"
           "  },"
           "  \"axiY\": {"
           "    \"pos\": %d"
           "  }"
           "}",
           vrx, vry);

  // Tamanho do JSON
  int content_length = strlen(json_body);

  // Cabeçalho HTTP + Corpo JSON
  snprintf(http_response, sizeof(http_response),
           "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/json\r\n"
           "Content-Length: %d\r\n"
           "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
           "Pragma: no-cache\r\n"
           "\r\n"
           "%s",
           content_length, json_body);
}

// Callback que processa requisições HTTP
static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p,
                       err_t err) {
  if (p == NULL) {
    // Fecha a conexão
    tcp_close(tpcb);
    return ERR_OK;
  }

  char *request;
  // Requisição do HTTP
  request = p->payload;
  printf("Requisição recebida:\n%s", request);

  // Analisa a requisição para determinar o endpoint
  if (strstr(request, "GET /api/joystick") != NULL) {
    // Endpoint API - retorna JSON com estado dos botões
    create_json_response();
  } else {
    // Qualquer outra rota - retorna a página HTML principal
    create_html_response();
  }

  // Envia a resposta HTTP
  tcp_write(tpcb, http_response, strlen(http_response), TCP_WRITE_FLAG_COPY);

  // Libera o buffer recebido
  pbuf_free(p);

  return ERR_OK;
}

// Callback de conexão
static err_t http_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
  // Associa o callback HTTP
  tcp_recv(newpcb, http_recv);
  return ERR_OK;
}

// Função que inicia o servidor TCP
static void http_init(void) {
  // Protocol Control Block
  struct tcp_pcb *pcb = tcp_new();
  if (!pcb) {
    printf("Erro ao criar PCB\n");
    return;
  }

  // Liga o servidor na porta 80
  if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK) {
    printf("Erro ao ligar o servidor na porta 80\n");
    return;
  }

  // Coloca o PCB em modo de escuta
  pcb = tcp_listen(pcb);

  // Associa o callback de conexão
  tcp_accept(pcb, http_accept);

  printf("Servidor HTTP ligado na porta 80...\n");
}

int main() {
  // Funções de init
  stdio_init_all();
  adc_init();
  adc_gpio_init(JOYSTICK_VRX);
  adc_gpio_init(JOYSTICK_VRY);

  // Inicia o chip Wi-Fi
  if (cyw43_arch_init()) {
    printf("Inicialização do Wi-Fi falhou\n");
    return -1;
  }

  // Inicia a estação Wi-Fi
  cyw43_arch_enable_sta_mode();

  printf("Conectando o Wi-Fi...\n");
  if (cyw43_arch_wifi_connect_timeout_ms("INTELBRAS", "12345678",
                                         CYW43_AUTH_WPA2_AES_PSK, 30000)) {
    printf("Falha na conexão\n");
    return 1;
  } else {
    printf("Conectado\n");
    // Endereço de IP
    uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    printf("Endereço IP %d.%d.%d.%d\n", ip_address[0], ip_address[1],
           ip_address[2], ip_address[3]);
  }

  // Inicia o servidor HTTP
  http_init();

  // Lógica da aplicação
  while (true) {
    vrx = read_joystick_x();
    vry = read_joystick_y();

    // Debounce
    sleep_ms(50);
  }
}
