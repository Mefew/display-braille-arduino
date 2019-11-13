#include <Arduino.h>
#include <LedControl.h>

bool is_connected = false, is_first_loop = true;
int num_of_symbols_in_current_line = 0;
uint8_t* symbols_received;

//Settings
int line_size = 0;
bool activation_value = true;

const int BUTTON_ADVANCE_PIN = 2;
const int BUTTON_GO_BACK_PIN = 3;
const unsigned long TIMEOUT_DEFAULT = 2000;
enum Order {
  HELLO = 0,
  SENDING_SYMBOLS = 1,
  SENDING_SETTINGS = 2,
  RECEIVED = 3,
  ADVANCE = 4,
  GO_BACK = 5,
  END_OF_BOOK = 6,
  DEBUG_MESSAGE = 7
};
typedef enum Order Order;

// Numero de matrizes Led utilizadas
#define     MAX_DEVICES     2  
  
// Ligacoes ao Arduino
#define     DATA_PIN     4  
#define     CS_PIN       5  
#define     CLK_PIN      6  

LedControl lc = LedControl(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
int brilho = 1;

void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }

  //Inicializa os displays
  lc.shutdown(0, false);
  lc.shutdown(1, false);

  //Ajusta o brilho de cada display
  lc.setIntensity(0, brilho);
  lc.setIntensity(1, brilho);

  //Apaga os displays
  lc.clearDisplay(0);
  lc.clearDisplay(1);

  while(!is_connected)
  {
    if (waitForMessage(5000) == HELLO) {
      processOrder(HELLO);
      write_order(HELLO);
    }
  }

  if (waitForMessage(TIMEOUT_DEFAULT) == SENDING_SETTINGS) {
    processOrder(SENDING_SETTINGS);
  }
  else {
    write_order(DEBUG_MESSAGE);
    Serial.println("Expected order SENDING_SETTINGS, received something else.");
  }
}

void loop () {
  if (is_first_loop || digitalRead(BUTTON_ADVANCE_PIN) == HIGH) {
    write_order(ADVANCE);
    int message = waitForMessage(TIMEOUT_DEFAULT);
    if (message == SENDING_SYMBOLS || message == END_OF_BOOK){
      processOrder (message);
    }
    else {
      write_order(DEBUG_MESSAGE);
      Serial.println("Expected order SENDING_SYMBOLS or END_OF_BOOK, received something else.");
    }
    is_first_loop = false;
  }

  else if (digitalRead(BUTTON_GO_BACK_PIN) == HIGH) {
    write_order(GO_BACK);
    int message = waitForMessage(TIMEOUT_DEFAULT);
    if (message == SENDING_SYMBOLS || message == END_OF_BOOK){
      processOrder (message);
    }
    else {
      write_order(DEBUG_MESSAGE);
      Serial.println("Expected order SENDING_SYMBOLS or END_OF_BOOK, received something else.");
    }
  }

  delay(150);
}

int waitForMessage (unsigned long timeout) {
  unsigned long startTime = millis();
  while ((Serial.available() < 1) && (millis() - startTime < timeout)){}
  if (Serial.available() < 1) {
    write_order(DEBUG_MESSAGE);
    Serial.println("Reached timeout while waiting for a message.");
  }
  else {
    return (int) Serial.read();
  }
}

void processOrder (Order order) {
  
  if (order == SENDING_SETTINGS) {
    write_order(RECEIVED);
    for (int i = 0; i < 2; i++) {
      int message = waitForMessage(TIMEOUT_DEFAULT);
      if (i==0)  {
        //Number of symbols to display per line. Maximum of 255
        line_size = message;
        symbols_received = (uint8_t*) malloc (line_size * sizeof(uint8_t));
        if (symbols_received == NULL) {
          write_order(DEBUG_MESSAGE);
          Serial.println("Could not allocate memory for symbols_received using malloc()");
        }
        write_order(RECEIVED);
      }
      if (i==1) {
        //Decides if the Braille dots are activated on high or low electric values. False means: Low = Active. True means: High = Active
        if (message == 0)
          activation_value = false;
        write_order(RECEIVED);
      }
    }
    
    
  }
  
  else if (order == SENDING_SYMBOLS) {
    write_order(RECEIVED);
    num_of_symbols_in_current_line = waitForMessage(TIMEOUT_DEFAULT);
    write_order(RECEIVED);
    for (int i = 0; i < num_of_symbols_in_current_line; i++) {
        symbols_received[i] = waitForMessage(TIMEOUT_DEFAULT);
        write_order(RECEIVED);
    }

    for (int i = num_of_symbols_in_current_line; i < line_size; i++) {
      symbols_received[i] = 0;
    }
    display_symbols();
  }
  
  else if (order == HELLO) {
    is_connected = true;
  }

  else if (order == END_OF_BOOK) {
    
  }
}

void display_symbols () {
  for (int i = 0; i < 8; i++) {                                                                                                   
    for (int j = 0; j < 3; j++) {
      if (i % 2 == 0) {
        lc.setLed(0, j, i, get_point_value(symbols_received[i/2], j));
        lc.setLed(1, j, i, get_point_value(symbols_received[i/2 + 4], j));
      }
      if (i % 2 == 1) {
        lc.setLed(0, j, i, get_point_value(symbols_received[i/2], j+3));
        lc.setLed(1, j, i, get_point_value(symbols_received[i/2 + 4], j+3));
      }
    }
  }

  for (int i = 0; i < 8; i++) {                                                                                                   
    for (int j = 4; j < 7; j++) {
      if (i % 2 == 0) {
        lc.setLed(0, j, i, get_point_value(symbols_received[i/2+8], j-4));
        lc.setLed(1, j, i, get_point_value(symbols_received[i/2+4+8], j-4));
      }
      if (i % 2 == 1) {
        lc.setLed(0, j, i, get_point_value(symbols_received[i/2+8], j-4+3));
        lc.setLed(1, j, i, get_point_value(symbols_received[i/2+4+8], j-4+3));
      }
    }
  }
}

void write_order(enum Order myOrder)
{
  Serial.write(myOrder);
}

int get_point_value(uint8_t cell, int point_number) {
  switch (point_number) {
    case 0:
    return cell & B00000001 ? 1 : 0;
    break;
    case 1:
    return cell & B00000010 ? 1 : 0;
    break;
    case 2:
    return cell & B00000100 ? 1 : 0;
    break;
    case 3:
    return cell & B00001000 ? 1 : 0;
    break;
    case 4:
    return cell & B00010000 ? 1 : 0;
    break;
    case 5:
    return cell & B00100000 ? 1 : 0;
    break;
  }
}
