# Monitor de Joystick

Este projeto implementa um servidor web no Raspberry Pi Pico W que monitora a posição de um joystick em tempo real. A posição do joystick é exibida em uma página web que atualiza a cada segundo, mostrando os valores dos eixos X e Y, bem como a direção atual.

## Como Funciona

1. O Pico W se conecta à rede Wi-Fi definida no código
2. Inicia um servidor web na porta 80
3. Serve uma página HTML estática quando acessado por um navegador
4. A página HTML contém JavaScript que faz requisições a cada segundo para o endpoint `/api/joystick`
5. O servidor retorna dados JSON contendo os valores atuais dos eixos X e Y do joystick
6. O JavaScript calcula a direção (Norte, Sul, Leste, Oeste, Nordeste, etc.) com base nos valores recebidos e atualiza a interface visual

## Estrutura do Código

- **Inicialização**: Configuração do Wi-Fi, ADCs para o joystick e servidor TCP
- **Rotas HTTP**:
  - `/` (raiz): Retorna a página HTML principal
  - `/api/joystick`: Retorna os valores dos eixos do joystick em formato JSON
- **JavaScript do Cliente**: 
  - Realiza requisições periódicas usando a Fetch API
  - Determina a direção com base nos valores X e Y
  - Atualiza a interface gráfica com as informações

## Hardware Necessário

- Raspberry Pi Pico W
- Joystick analógico de 2 eixos
- Conexões:
  - Eixo X do joystick ao GPIO27 (ADC1)
  - Eixo Y do joystick ao GPIO26 (ADC0)