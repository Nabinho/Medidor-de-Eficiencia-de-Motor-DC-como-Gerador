/*******************************************************************************
* Calculadora de Eficiencia de Geracao de Motor DC
*
* Escrito por Giovanni de Castro (02/02/2021).
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version (<https://www.gnu.org/licenses/>).
*******************************************************************************/

//Adiciona a biblioteca de controle do LCD
#include <Wire.h>
#include <rgb_lcd.h>

//Cria o objeto lcd
rgb_lcd lcd;  
const int colorR = 0;
const int colorG = 0;
const int colorB = 255;

// Declaracao dos pinos de controle do motor DC da Julieta
const int PINO_INB = 8;
const int PINO_ENB = 6;

// Declaracao dos pinos conectados aos canais de saida dos encoders
const int PINO_CH2_E = 2;
const int PINO_CH1_E = 4;
const int PINO_CH2_D = 3;
const int PINO_CH1_D = 5;

// Declaracao do pino conectado ao botao da Julieta
const int PINO_BOTAO = A0;

// Declaracao do pino conectado ao potenciometro de controle da velocidade do motor
const int PINO_POT = A1;

// Declaracao do pino que le a tensao da bateria
const int PINO_VOLTMETER = A2;

// Declaracao do pino que le a tensao do motor
const int PINO_MOTOR = A3;

// Declaracao dos pinos conectados aos divisores de tensao do gerador
const int PINO_GERADOR_HORARIO = A6;
const int PINO_GERADOR_ANTIHORARIO = A7;

// Declaracao das variaveis auxiliares para a medicao da tensao da bateria
int leitura = 0;
float tensao_mapeada = 0.00;
float tensao_calculada = 0.00;
float resistor_1 = 51000.00;
float resistor_2 = 75000.00;

// Declaracao das variaveis auxiliares para a medicao da tensao do motor de controle
int leitura_motor = 0;
float tensao_mapeada_motor = 0.00;

// Declaracao das variaveis auxiliares para a medicao da tensao do motor gerador
int leitura_gerador = 0;
float tensao_mapeada_gerador = 0.00;
float tensao_calculada_gerador = 0.00;
float resistor_3 = 10000.00;
float resistor_4 = 10000.00;
float tensao_rms = 0.00;
float eficiencia = 0.00;
int contador_media = 0;

// Declaracao das variaveis auxiliares para a verificacao do sentido
int estado_1;
int ultimo_estado_1;
boolean sentido_1;
int estado_2;
int ultimo_estado_2;
boolean sentido_2;

// Declaracao da variavel de controle do sentido
bool nivel = false;
bool partida = false;
bool controle = false;

// Declaracao da variavel de controle da velocidade
int PWM = 0;

// Declaracao das variaveis auxiliares para o calculo da velocidade
unsigned long contador1;
unsigned long contador2;
const int NUMERO_LEITURAS = 2;
const int NUMERO_DENTES = 6;

// Declaracao das variaveis auxiliares para a temporizacao de um minuto
unsigned long tempo_antes = 0;
const long MINUTO = 60000;
unsigned long tempo_antes_2 = 0;
const long SEGUNDO = 1000;

// ---------------------------------------------------------------------------------------
void setup()
{
	// Inicializacao do monitor serial
	Serial.begin(115200);
	Serial.println("Monitoramento da eficiência de geração e eletricidade de motor DC.");

	//Inicializacao do lcd
	lcd.begin(16,2);
  	lcd.setRGB(colorR, colorG, colorB);
  	lcd.print("Eficiencia motor");
  	lcd.setCursor(0,1);
  	lcd.print("DC gerador  v1.0");
  	delay(5000);
  	lcd.clear();
	
	// Configura os pinos de controle do motor como uma saida
	pinMode(PINO_INB, OUTPUT);
	pinMode(PINO_ENB, OUTPUT);
	
	// Inicia os pinos de controle do motor em nivel logico baixo
	digitalWrite(PINO_INB, nivel);
	analogWrite(PINO_ENB, PWM);
	
	// Configura o pino conectado ao botao como uma entrada
	pinMode(PINO_BOTAO, INPUT_PULLUP);
	
	// Configuracao do pino de leitura da tensao da bateria como entrada
	pinMode(PINO_VOLTMETER, INPUT);
	
	// Configuracao do pino conectado ao potenciomentro como entrada
	pinMode(PINO_POT, INPUT);
	
	// Configuracao do pino de leitura da tensao do motor como entrada
	pinMode(PINO_MOTOR, INPUT);
	
	// Configuracao dos pinos de leitura do gerador
	pinMode(PINO_GERADOR_ANTIHORARIO, INPUT);
	pinMode(PINO_GERADOR_HORARIO, INPUT);
	
	// Configuracao dos pinos conectados aos canais do encoder como entrada
	pinMode(PINO_CH2_E, INPUT);
	pinMode(PINO_CH1_E, INPUT);
	pinMode(PINO_CH2_D, INPUT);
	pinMode(PINO_CH1_D, INPUT);
	
	// Inicializa as interrupcoes com os pinos configurados para chamar as funcoes
	// "contador_pulso2" e "contador_pulso1" respectivamente a cada mudanca de estado das portas
	attachInterrupt(digitalPinToInterrupt(PINO_CH2_E), contador_pulso2, CHANGE);
	attachInterrupt(digitalPinToInterrupt(PINO_CH2_D), contador_pulso1, CHANGE);
}

// ---------------------------------------------------------------------------------------
void loop()
{
	// Controla o sentido do motor e reinicia as leituras
	if (digitalRead(PINO_BOTAO) == LOW)
	{
		delay(30);
		if (digitalRead(PINO_BOTAO) == LOW)
		{
			partida = false;
			controle = false;
			nivel = !nivel;
			digitalWrite(PINO_INB, nivel);
			PWM = 0;
			delay(1000);
			partida = true;
			controle = true;
			// Zera os contadores e reinicia a contagem de tempo
			contador1 = 0;
			contador2 = 0;
			tempo_antes = millis();
		}
	}
	
	// Controla a velocidade do motor após a partida
	if (controle)
	{
		PWM = map(analogRead(PINO_POT), 0, 1023, 255, 250);
		analogWrite(PINO_ENB, PWM);
	}
	
	// Verifica a contagem de tempo e exibe as informacoes coletadas do motor
	if (partida){
		if ((millis() - tempo_antes) > MINUTO){
			// A cada minuto
			le_encoders();
			// Reinicia a contagem de tempo
			tempo_antes = millis();
		} if ((millis() - tempo_antes_2) > SEGUNDO){

			// A cada minuto
			le_bateria();
			le_motor_controle();
			le_gerador();

			//Exibe as leituras no LCD
		    lcd.print("Vm= ");
		    lcd.setCursor(4,0);
		    lcd.print(tensao_mapeada_motor);
		    lcd.setCursor(8,0);
		    lcd.print("Vg= ");
		    lcd.setCursor(12,0);
		    lcd.print(tensao_rms);
		    lcd.setCursor(0,1);
		    lcd.print("Eficiencia ");
		    lcd.setCursor(11,1);
		    lcd.print(eficiencia);
			
			// Reinicia a contagem de tempo
		    tensao_rms = 0.00;
		    contador_media = 0;
			tempo_antes_2 = millis();

		}
	}

	// Realiza a leitura do pino do motor e calcula a tensao aplicada ao motor
	if (nivel){

		leitura_gerador = analogRead(PINO_GERADOR_HORARIO);
		tensao_mapeada_gerador = (leitura_gerador * 5.00) / 1024.00;
		tensao_calculada_gerador = (tensao_mapeada_gerador / (resistor_4 / (resistor_3 + resistor_4)));

	}else{

		leitura_gerador = analogRead(PINO_GERADOR_ANTIHORARIO);
		tensao_mapeada_gerador = (leitura_gerador * 5.00) / 1024.00;
		tensao_calculada_gerador = (tensao_mapeada_gerador / (resistor_4 / (resistor_3 + resistor_4)));

	}

	tensao_rms += tensao_calculada_gerador;
	contador_media++;

}

// ---------------------------------------------------------------------------------------
// Funcao de interrupcao
void contador_pulso2()
{
	// Incrementa o contador
	contador2++;
	// Verifica o sentido de rotacao do motor
	estado_1 = digitalRead(PINO_CH2_E);
	if (ultimo_estado_1 == LOW && estado_1 == HIGH){
		if (digitalRead(PINO_CH1_E) == LOW){
			sentido_1 = true;
		}else{
			sentido_1 = false;
		}
	}
	ultimo_estado_1 = estado_1;
}

// Funcao de interrupcao
void contador_pulso1()
{
	// Incrementa o contador
	contador1++;
	// Verifica o sentido de rotacao do motor
	estado_2 = digitalRead(PINO_CH2_D);
	if (ultimo_estado_2 == LOW && estado_2 == HIGH){
		if (digitalRead(PINO_CH1_D) == LOW){
			sentido_2 = true;
		}else{
			sentido_2 = false;
		}
	}
	ultimo_estado_2 = estado_2;
}

// ---------------------------------------------------------------------------------------
// Funcao que verifica o sentido de rotacao e a velocidade dos motores
void le_encoders()
{
	Serial.print("Motor D: ");
	// Verifica a variavel "sentido"
	if (sentido_1){
		// Se ela for verdadeira ("true")
		Serial.print("Sentido: Horário");
		Serial.print("       |  ");
	}else{
		// Se ela for falsa ("false")
		Serial.print("Sentido: Anti-Horário");
		Serial.print("  |  ");
	}
	// Calcula a velocidade e exibe no monitor
	float velocidade_1 = contador2 / (NUMERO_DENTES * NUMERO_LEITURAS); // Calcula a velocidade de acordo com o numero de dentes do disco
	Serial.print("Velocidade: ");
	Serial.print(velocidade_1);
	Serial.println(" RPM");
	Serial.print("Motor E: ");
	// Verifica a variavel "sentido"
	if (sentido_2){
		// Se ela for verdadeira ("true")
		Serial.print("Sentido: Horário");
		Serial.print("       |  ");
	}else{
		// Se ela for falsa ("false")
		Serial.print("Sentido: Anti-Horário");
		Serial.print("  |  ");
	}
	// Calcula a velocidade e exibe no monitor
	float velocidade_2 = contador1 / (NUMERO_DENTES * NUMERO_LEITURAS); // Calcula a velocidade de acordo com o numero de dentes do disco
	Serial.print("Velocidade: ");
	Serial.print(velocidade_2);
	Serial.println(" RPM");
	// Zera os contadores
	contador1 = 0;
	contador2 = 0;
}

// ---------------------------------------------------------------------------------------
// Mede a tensao da bateria
void le_bateria()
{
	// Realiza a leitura do pino da bateria, e entao calcula a tensao
	leitura = analogRead(PINO_VOLTMETER);
	tensao_mapeada = (leitura * 5.00) / 1024.00;
	tensao_calculada = (tensao_mapeada / (resistor_2 / (resistor_1 + resistor_2))) + 1.52;
	if (tensao_calculada < 0.09){
		tensao_calculada = 0;
	}
	Serial.print("Tensão da bateria: ");
	Serial.print(tensao_calculada);
	Serial.print(" V");
}

// ---------------------------------------------------------------------------------------
// Mede a tensao do motor de controle
void le_motor_controle()
{
	// Realiza a leitura do pino do motor e calcula a tensao aplicada ao motor
	leitura_motor = analogRead(PINO_MOTOR);
	tensao_mapeada_motor = ((leitura_motor * tensao_calculada) / 1024.00) - 1.82;
	if (tensao_mapeada_motor < 0){
		tensao_mapeada_motor = 0;
	}
	Serial.print("       | ");
	Serial.print("Tensão aplicada ao motor: ");
	Serial.print(tensao_mapeada_motor);
	Serial.println(" V");
  
}

// ---------------------------------------------------------------------------------------
// Mede a tensao do motor gerador controle
void le_gerador()
{
	tensao_rms = (tensao_rms/contador_media) - 2.34;
	Serial.print("Tensão gerada: ");
	Serial.print(tensao_rms);
	Serial.print(" V");
	
	eficiencia = (tensao_rms / tensao_mapeada_motor) * 100;
	Serial.print("            | ");
	Serial.print("Eficiência do gerador: ");
	Serial.print(eficiencia);
	Serial.println(" %");
 
}