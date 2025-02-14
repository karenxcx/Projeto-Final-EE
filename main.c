/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name: Projeto Final Grupo 2 
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC16F1827
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "main.h"
#include <stdbool.h>

void main(void)
{
    // initialize the device
    SYSTEM_Initialize();
    LeituraValvula(true);  // Realiza calibra��o no in�cio

    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    while (1)
    {
        calculateHeight();
        calculateToF();
        __delay_ms(100); //pq tem esse delay?
        
        if (EUSART_is_rx_ready()) {
            processaRecepcaoUSART();
        } else if (novosDadosCadastrados) {
            atualizaDadosRecebidos();
        }
        
        if (rxbuffer[0] == VENTOINHA || rxbuffer[0] == VALVULA) { // Se o modo de opera��o for ventoinha ou valvula, chama a fun��o de controle PID
        
            Controle();
        }
        else if (rxbuffer[0] == MANUAL) {
            //QUE QUE ELE FAZ?
        }
        
        else if (rxbuffer[0] == RESET_) {
            asm("reset"); // Reseta o Pic
        }
       
      
        ajustaPwm(128);
        __delay_ms(500);

        AbreValvula();
        __delay_ms(1000);

        FechaValvula();
        __delay_ms(1000);
        
    }
}

void processaRecepcaoUSART() {
    for (int i = 0; i < 7; i++) {
        rxbuffer[i] = EUSART_Read();
    }

    dados.Modo_Operacao = rxbuffer[0];
    dados.height_desejada.bytes[1] = rxbuffer[1];
    dados.height_desejada.bytes[0] = rxbuffer[2];
    dados.valvula_desejada.bytes[1] = rxbuffer[3];
    dados.valvula_desejada.bytes[0] = rxbuffer[4];
    dados.pwm.bytes[1] = rxbuffer[5];
    dados.pwm.bytes[0] = rxbuffer[6];

    novosDadosCadastrados = true;
}

void atualizaDadosRecebidos() {
    EPWM1_LoadDutyValue(dados.pwm.total);
    novosDadosCadastrados = false;
}

void enviarDados() {
    uint8_t pacote[12];
    
    pacote[0] = dados.Modo_Operacao;
    pacote[1] = dados.height_desejada.bytes[1];
    pacote[2] = dados.height_desejada.bytes[0];
    pacote[3] = dados.height.bytes[1];
    pacote[4] = dados.height.bytes[0];
    pacote[5] = dados.time_of_flight.bytes[1];
    pacote[6] = dados.time_of_flight.bytes[0];
    pacote[7] = dados.temp_t;
    pacote[8] = dados.valvula_desejada.bytes[1];
    pacote[9] = dados.valvula_desejada.bytes[0];
    pacote[10] = dados.valvula.bytes[1];
    pacote[11] = dados.valvula.bytes[0];

    for (size_t i = 0; i < sizeof(pacote); i++) {
        EUSART_Write(pacote[i]);
    }
}

void triggerUltrasonic(){
//    Trigger_SetHigh();
//    __delay_us(10);
//    Trigger_SetLow();
//    TMR1_Initialize();
}

void calculateToF(){
    Trigger_SetHigh();
    __delay_us(10);
    Trigger_SetLow();
    
    while(!Echo_PORT);  // Aguarda borda de subida (in�cio do ECHO)
    TMR1_WriteTimer(0);
    TMR1_StartTimer();
    while(Echo_PORT);   // Aguarda borda de descida (fim do ECHO)
    TMR1_StopTimer();
    dados.time_of_flight.total = TMR1_ReadTimer();
    // Converte de contagens de Timer1 para ms
    dados.time_of_flight.total = dados.time_of_flight.total * 0.00025;
    
}

float calculateHeight(){
    temperature = ADC_GetConversion(Temp);
    temperature = (int)( COEF *(float) temperature);    // Converte valor para [�C] inteiro
    if (temperature < 0) temperature = 0;               // Temperatura m�nima em 0�C
    if (temperature > 50) temperature = 50;             // Temperatura m�xima em 50�C
    temperature_index = temperature; 
    soundspeed_t = LUTsoundspeed[temperature_index];            //Calcula velocidade do som [m/s]
    dados.height.total = (dados.time_of_flight.total*soundspeed/2);             //Altura em [mm]
    

}

//Fun��o para calcular o erro e retornar o ajuste que deve ser feito na ventoinha ou na v�lvula
int ControleAjuste() {
    float erroAtual = (float)(dados.height_desejada.total - dados.height.total); //Mede a diferen�a entre a altura desejada e a altura lida
    erroAcumulado += erroAtual; //Soma o erro ao longo do tempo para eliminar o erro estacion�rio.
    float deltaErro = erroAtual - erroAnterior; // Mede a taxa de varia��o do erro, ajudando a evitar oscila��es excessivas.
    erroAnterior = erroAtual;
    
    return (int)(ganhoK * erroAtual + tempoK * erroAcumulado + ganhoD * deltaErro); //C�lculo da sa�da do controlador PI
}

void Controle() {
    //Alterar_PWM = enviado = false;
    int ajuste = ControleAjuste();        // Obt�m o valor de ajuste necess�rio para reduzir o erro entre o valor medido e o valor desejado.
    if (dados.Modo_Operacao == VENTOINHA) {     // Se o sistema est� no modo VENTOINHA, chama ajustaPwm
        ajustaPwm(ajuste);
    } else {                              // Se est� no modo VALVULA, chama ajustaValvula
        ajustaValvula(ajuste);
    }
}

// Ajusta o valor do PWM da ventoinha com base no ajuste calculado pelo controle PI.
void ajustaPwm(int ajuste) {
    int sinalPWM = dados.pwm.total + ajuste;          // Soma o ajuste ao PWM atual
    sinalPWM = Limites(sinalPWM, 0, 1023); // Garante que o PWM fique dentro dos limites
    //pwm = sinalPWM;
    EPWM1_LoadDutyValue(sinalPWM); //O novo valor do PWM � carregado para controlar a velocidade do motor da ventoinha
}

// Ajusta a posi��o da v�lvula com base no ajuste calculado pelo controle PI.
void ajustaValvula(int ajuste) {
    //int novaPosicao = valvula_desejada + ajuste; // Soma o ajuste � posi��o da v�lvula, Se o erro for positivo, a v�lvula abre mais. Se for negativo, fecha.
    int novaPosicao = dados.valvula.total + ajuste;
    //valvula_desejada = Limites(novaPosicao, 0, alturaMaximaMM); // Garante os limtes: O valor da v�lvula n�o pode ser menor que 0 (totalmente fechada) nem maior que alturaMaximaMM (totalmente aberta).
    novaPosicao = Limites(novaPosicao, 0, 470);
    //MANDAR MEXER A VALVULA COM A NOVA POSI��O
}

// Calibra��o e leitura da valvula
bool LeituraValvula(bool forcar_calibracao) {
    uint16_t valor_sensor = ADC_GetConversion(Lim);

    if (forcar_calibracao || (!calibracao&& valor_sensor < TCRT_THRESHOLD)) {
        //printf("Iniciando calibra��o da v�lvula...\n");

        while (valor_sensor < TCRT_THRESHOLD && passo_atual < MAX_PASSOS) {
            MoveValvula(true);
            __delay_ms(6);
            valor_sensor = ADC_GetConversion(Lim);
        }

        passo_atual = 0;
        calibracao= true;
    }

    return passo_atual;  // Retorna a posi��o atual da v�lvula
}

// controle da valvula
void MoveValvula(bool direcao) {
    static uint8_t passos = 0;

    if ((direcao && passo_atual >= MAX_PASSOS) || (!direcao && passo_atual == 0)) {
        return;
    }

    if (direcao) {
        passo_atual++;
        passos++;
        if (passos > 3) passos = 0;
    } else {
        passo_atual--;
        if (passos == 0) passos = 3;
        else passos--;
    }

    switch (passos) {
        case 0: SM1_SetHigh(); SM2_SetHigh(); SM3_SetLow(); SM4_SetLow(); break;
        case 1: SM1_SetLow(); SM2_SetHigh(); SM3_SetHigh(); SM4_SetLow(); break;
        case 2: SM1_SetLow(); SM2_SetLow(); SM3_SetHigh(); SM4_SetHigh(); break;
        case 3: SM1_SetHigh(); SM2_SetLow(); SM3_SetLow(); SM4_SetHigh(); break;
    }

    __delay_ms(3);
}

void AbreValvula() {
    while (passo_atual < MAX_PASSOS && !LeituraValvula(false)) {
        MoveValvula(true);  // Move no sentido hor�rio
    }
    valvula_aberta = true;
    //printf("V�lvula totalmente aberta!\n");
}

void FechaValvula() {
    while (passo_atual > 0) {
        MoveValvula(false);
    }
    valvula_aberta = false;
    //printf("V�lvula totalmente fechada!\n");
}
