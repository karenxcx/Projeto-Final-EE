/* 
 * File:   
 * Author: Grupo 2
 * Comments:
 * Revision history: 
 */
/*/PINOS 
 
 RA0 - Lim - Limite da válvula;        RB0 - Echo - sensor ultrassom; 
 RA1 - SM1 - Status Motor;             RB1 - UART_Rx - Bluetooth;
 RA2 - SM2 - Status Motor;             RB2 - UART_Tx - Bluetooth;
 RA3 - SM3 - Status Motor;             RB3 - Vent - aciona a ventoinha;
 RA4 - SM4 - Status Motor;             RB4 - Temp - Sensor de temperatura;
 RA5 - MCLR - faz nada;                RB5 - vazio
 RA6 - Trigger - sensor ultrassom;     RB6 - CSPCLK - faz nada;
 RA7 - LED - apenas um led;            RB7 - ICSPDAT - faz nada; 
 /*/

/*/Dados analógicos
 Sensor óptico TCRT-5000 - 0V: valvula aberta, 5V: valvula fechada
 Sensor ultrassom HC-SR04 - altura da bola
 Sensor temperatura LM35 - compensação para altura da bola
 * Todos conectados em VDD = 5V; 
 * RA0: INPUT  - Lim - sensor óptico;
 * RA6: INPUT  - Echo - sensor ultrassom;
 * RB4: INPUT  - Temp  - sensor de temperatura; 
 * RB0: OUTPUT - Trigger - sensor ultrassom;
 * RA7: OUTPUT - LED;
 /*/

/*/Comunicação Bluetooth
 * UART
 * RB1 - UART_Rx: 7 bytes;
 * RB2 - UART_Tx: 15 bytes;
 /*/

/*/Controle da Ventoinha
* RB3: PWM - Controle da ventoinha;             
 /*/

/*/Controle Motor de Passo ULN3003
* RA1: OUTPUT - SM1 - Status Motor;             
* RA2: OUTPUT - SM2 - Status Motor;             
* RA3: OUTPUT - SM3 - Status Motor;             
* RA4: OUTPUT - SM4 - Status Motor;             
 /*/

// This is a guard condition so that contents of this file are not included
// more than once.  
#ifndef XC_HEADER_TEMPLATE_H
#define	XC_HEADER_TEMPLATE_H

#include <xc.h> // include processor files - each processor file is guarded.  
#include <math.h>

/* Medição da Altura da Bola*/

#define T0      273.15       // [K]   - Temperatura em Kelvin a 0C;
#define C0      331.3        // [m/s] - Velocidade do som a 0C; 
#define COEF    0.0488       // Coeficiente para conversão para 0C = 50/1023

/* Variáveis de controle PI*/

#define ganhoK  0.1          // Ganho proporcional
#define tempoK  1.5          // Tempo de interação / Tempo de ação integral
#define ganhoD  0.05         // Ganho derivativo

/* Modos de operação */

#define MANUAL 0
#define VENTOINHA 1
#define VALVULA 2
#define RESET_ 3

/* Sensor fim de curso e passos Max do motor */

#define TCRT_THRESHOLD 500
#define MAX_PASSOS 420


/* Macro para limtar o valor das variáveis de pwm e valvula
A macro é utilizada para limitar um valor dentro de um intervalo mínimo e máximo.
Ela impede que um valor ultrapasse um limite superior ou fique abaixo de um limite inferior.

Se valor for menor que min, retorna min.
Se valor for maior que max, retorna max.
Se valor estiver dentro do intervalo, retorna valor normalmente.
 */

#define Limites(valor, min, max) ((valor) < (min) ? (min) : ((valor) > (max) ? (max) : (valor)))

float temperature = 0, temperature_kelvin = 0, time_of_flight = 0, soundspeed = 0, valvula =0;
uint8_t temperature_index = 0, rxbuffer[7];
uint16_t soundspeed_t;

float  valvula_desejada = 0, erroAcumulado = 0, erroAnterior = 0;
//bool Alterar_PWM, enviado = false;
//uint8_t Modo_Operacao;      // Recebe o modo de operação pela comunicação
uint16_t pwm;               // Ciclo útil do motor (entre 0 e 1023)

uint16_t passo_atual = 0;
bool valvula_aberta = false;
bool calibracao = false;
bool novosDadosCadastrados = false;
uint16_t posicao = 0;

typedef union {
    uint16_t total;
    uint8_t bytes[2];
} U16_Union;

// Estrutura para armazenar os dados recebidos e enviados via UART
typedef struct {
    uint8_t Modo_Operacao;
    U16_Union height_desejada;
    U16_Union height;
    U16_Union time_of_flight;
    uint16_t temp_t;
    U16_Union valvula_desejada;
    U16_Union valvula;
    U16_Union pwm;
} DADOS;

// Declaração da variável global de dados
DADOS dados;

// Inicialização das funções
void triggerUltrasonic();
float calculateHeight();
void calculateToF();
int ControleAjuste();
void Controle();
void ajustaPwm(int ajuste);
void ajustaValvula(int ajuste);
void enviarDados();
void atualizaDadosRecebidos();
void processaRecepcaoUSART();
bool LeituraValvula(bool forcar_calibracao);
void MoveValvula(bool direcao);
void AbreValvula();
void FechaValvula();


/*/Velocidade do som em [m/s] para temperaturas de 0ºC a 50ºC */
const float LUTsoundspeed[51] = {
    331.3,331.9,332.5,333.1,333.7,334.3,334.9,335.5,336.1,336.7,
    337.3,337.9,338.5,339.1,339.7,340.3,340.9,341.5,342.0,342.6,
    343.2,343.8,344.4,345.0,345.5,346.1,346.7,347.3,347.9,348.4,
    349.0,349.6,350.2,350.7,351.3,351.9,352.5,353.0,353.6,354.2,
    354.7,355.3,355.9,356.4,357.0,357.6,358.1,358.7,359.2,359.8,
    360.3};







#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

    // TODO If C++ is being used, regular C code needs function names to have C 
    // linkage so the functions can be used by the c code. 

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif	/* XC_HEADER_TEMPLATE_H */

