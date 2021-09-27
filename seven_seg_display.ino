  /*------------------------------------------*
   * Displayed x Digit 7 segment common anode *
   * Created by me[R]a, yumera@gmail.com      *
   * 05-May-19 20:13 , Šid                    *
   * Arduino NANO                             *
   *------------------------------------------*/
  #include <EEPROM.h>
  #define latchPin    6   // rck
  #define clockPin    7   // sck
  #define dataPin     5   // ser in
  #define button1     2   // button 1 menu settings.
  #define button2     3   // button 2 increment broja ++
  #define DIGIT_ITEMS 5   // broj cifara

  /*Segment Decoding Table by me[R]a for shift LSBFIRST za MBFIRST obrnuti redlosed bitova
   *------------------------------------------------------------------------------------------------*
   * Char      A B C D E F G   DP  Hex Value     Binary      Decimal             Patern             *
   *------------------------------------------------------------------------------------------------*            
   *   0       1 1 1 1 1 1 0   0     FC        B11111100       252                 A                *
   *   1       0 1 1 0 0 0 0   0     60        B01100000       096            -----------           *
   *   2       1 1 0 1 1 0 1   0     DA        B11011010       218           |           |          *
   *   3       1 1 1 1 0 0 1   0     F2        B11110010       242         F |           | B        *
   *   4       0 1 1 0 0 1 1   0     66        B01100110       102           |     G     |          *
   *   5       1 0 1 1 0 1 1   0     B6        B10110110       182            -----------           *
   *   6       1 0 1 1 1 1 1   0     BE        B10111110       190           |           |          * 
   *   7       1 1 1 0 0 0 0   0     E0        B11100000       224         E |           | C        *
   *   8       1 1 1 1 1 1 1   0     FE        B11111110       254           |           |   DP     *
   *   9       1 1 1 1 0 1 1   0     F6        B11110110       246            -----------    --     *
   * space     0 0 0 0 0 0 0   0     00        B00000000       000                 D                *
   *------------------------------------------------------------------------------------------------*/            

  byte SegDigit[] = {0xFC, 0x60, 0xDA, 0xF2, 0x66, 0xB6, 0xBE, 0xE0, 0xFE, 0xF6, 0x00};                       
  byte digitBuffer[DIGIT_ITEMS] = {0};
  byte dot;                               // decimalna tacka
  volatile int count_digit = DIGIT_ITEMS; // redosled cifara za podesavanje brojeva
  String strByte;                         // string pristigao serijskom komunikacijom                     
  volatile byte state = LOW;              // stanje menija za menjanje cifri LOW nema menjanja HIGH startovana je promena cifri
  unsigned int loop_time = 0;
  volatile byte edit = LOW;

void setup () {
  Serial.begin(115200);
  // citam brojeve cifara iz eeproma pocevsi od address 0
  for(int i = DIGIT_ITEMS; i > -1; i--){
    digitBuffer[i] = EEPROM.read(i);
  }
  dot=EEPROM.read(0x0F);// pozicija decimalne tacke memorisana na eeprom adresi 15 dec
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin , OUTPUT);   
  pinMode(button1 , INPUT_PULLUP);
  pinMode(button2 , INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(button1), numMenu,   RISING);
  attachInterrupt(digitalPinToInterrupt(button2), increment, RISING);
}  

void loop () { 
  if (edit == HIGH)// Ako je zavrseno podesavanje ide tri put blink 
    endEdit();      
  else            // Prikaz trenutnog stanja cifara 
    updateDisplay();
  
}

void numMenu(){
  /*-----------------------------------------------------------------------------*
   * Biram cifru koju cu da menjam. Cifra pocinje da blinka a sledecim ptiriskom *
   * prelazim na drugu. Posle zadnje cifre prestaje blinkanje i setovani broj    *
   * se upisuje u eeprom memoriju pocevsi od adrese 0. To je iz razloga ako      *
   * dodje do nasilnog iskljucenja displeja kako bi se pri ponvnom ukljucejne    *
   * prikazao zadnje podeseni broj                                               *
   *-----------------------------------------------------------------------------*/
  //Interrupt funkcija za butoon1   
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
    // Ako interrupts dolazi brze od 250ms ignorisemo pritisak na button (DEBOUNCE)
    if (interrupt_time - last_interrupt_time > 250) {
      if(--count_digit >= -1){  // -1 zbog sve cifre + tacka
        state = HIGH;
      }else {
        state = LOW;
        count_digit = DIGIT_ITEMS;
        for(int i = DIGIT_ITEMS; i > -1; i--){
          EEPROM.write(i, digitBuffer[i]);
        }
        EEPROM.write(0x0F, dot);//poziciju decimalne tacke upisujem u eeprom na adresu 15(0x0F)
        edit = HIGH;
      }
    }
    last_interrupt_time = interrupt_time;
}

void increment(){
  /*-----------------------------------------------------------------------------*
   * Uvecavam za 1 svaku cifru koja trenutno treperi. Kad dostignem max 9        *
   * pocinje brojanje od 0 i tako u krug.                                        *
   * Interrupt funkcija za butoon2                                               *
   *-----------------------------------------------------------------------------*/
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // Ako interrupts dolazi brze od 150ms ignorisemo pritisak na button (DEBOUNCE)
    if (interrupt_time - last_interrupt_time > 250) {
      if (state == HIGH) {
        if (count_digit > -1)
          if (++digitBuffer[count_digit] > 10) digitBuffer[count_digit] = 0;
        else
          if (++dot > DIGIT_ITEMS) dot = 0;    
      }
    }
    last_interrupt_time = interrupt_time;
}

void updateDisplay(){
  /*-----------------------------------------------------------------------------*
   * Osvezavamo display (sa novim podacima) nema vreme osvezavanja izvrsava se   *
   * za svaki loop()                                                             *
   *-----------------------------------------------------------------------------*/
  digitalWrite(latchPin, LOW);
  for (int i = DIGIT_ITEMS; i > -1; i--){
    byte showDigit = SegDigit[digitBuffer[i]];
    
    if(state == HIGH && count_digit == -1 && loop_time > 500){
      showDigit = SegDigit[digitBuffer[i]] & 0xFE;              // B11111110 resetujem decimalnu tacku
    }else{
      showDigit = SegDigit[digitBuffer[i]] | 0x01;              // B00000001 setujem decimalnu tacku
      
      if(dot == i) showDigit = SegDigit[digitBuffer[i]] | 0x01; // B00000001 setujem decimalnu tacku
      else showDigit = SegDigit[digitBuffer[i]] & 0xFE;         // B11111110 resetujem decimalnu tacku
    }
  /*-----------------------------------------------------------------------------*
   * blinkam sa cifrom koju menjam  (time>500 about 500ms)                       *
   *-----------------------------------------------------------------------------*/
    if (state == HIGH && count_digit == i && loop_time > 500) shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
    else shiftOut(dataPin, clockPin, LSBFIRST, showDigit);
    
  /*-----------------------------------------------------------------------------*/
  }
  digitalWrite(latchPin, HIGH);
  
  loop_time++; 
  if (loop_time/2 > 500) 
    loop_time = 0;
}

void endEdit(){
  /*-----------------------------------------------------------------------------*
   * blinkam tri puta sa svim ciframa kao potvrdu zavrsetka podesavanja          *
   *-----------------------------------------------------------------------------*/
  for(int j = 0; j < 3; j++){
    digitalWrite(latchPin, LOW);
    delay(200);
    for(int i = DIGIT_ITEMS; i > -1; i--){
      shiftOut(dataPin, clockPin, LSBFIRST, 0x00);
    }
    digitalWrite(latchPin, HIGH);  
    digitalWrite(latchPin, LOW);
    delay(200);
    for(int i = DIGIT_ITEMS; i > -1; i--){
      if(dot==i){
        shiftOut(dataPin, clockPin, LSBFIRST, SegDigit[digitBuffer[i]] | 0x01); // B00000001 setujem decimalnu tacku);
      }else{
        shiftOut(dataPin, clockPin, LSBFIRST, SegDigit[digitBuffer[i]] & 0xFE); // B11111110 resetujem decimalnu tacku
      }
    }
    digitalWrite(latchPin, HIGH);
  }
  edit = LOW;
}
