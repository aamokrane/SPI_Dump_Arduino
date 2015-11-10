/Wiring definition
#define DATAOUT 11//MOSI
#define DATAIN  12//MISO 
#define SPICLOCK  13//sck
#define SLAVESELECT 10//ss

//opcodes
#define WREN  6
#define WRDI  4
#define RDSR  5
#define WRSR  1
#define READ  3
#define WRITE 2

#define WRITE_BYTE_LOW_ADDRESS  2 //Microchip 25LC040, addresses lower than 256
#define WRITE_BYTE_HIGH_ADDRESS  10 //Microchip 25LC040, addresses higher that 256
#define READ_BYTE_LOW_ADDRESS  3 //Microchip 25LC040, addresses lower than 256
#define READ_BYTE_HIGH_ADDRESS  11 //Microchip 25LC040, addresses higher than 256

//Few constants used in the program
#define DUMP_SIZE 512   //The size of the content that will be dumped at once from the EEPROM to the Arduino
#define BUFFER_FILL_SIZE 512  //The size of the input buffer user to write data from the Arduino to the EEPROM
#define MAX_ADDRESS 512  //2^16  //For Microchip 25LC040 the memory is 512*8 bit ==> 2^9 address space
#define PAGE_SIZE 16 // The page size in the EEPROM. This is the number of bytes that can be written at a time to the memory
#define NB_PAGES MAX_ADDRESS/PAGE_SIZE   //The number of pages

#define VERBOSE 0

byte eeprom_output_data;

byte clr;
unsigned int address_read=0;
unsigned int address_write=0;

//data buffer
char buffer [BUFFER_FILL_SIZE]; //Data to write from the Arduino to the EEPPROM
char dumped_data[DUMP_SIZE]; //Data read from the EEPROM
int cpt = 0;


char spi_transfer(volatile char data)
{
  SPDR = data;                    // Start the transmission
  while (!(SPSR & (1<<SPIF)))     // Wait the end of the transmission
  {
  };
  return SPDR;                    // return the received byte
}


void initialize_buffer_and_dump()
{
  //Initializes the buffer and the dumped content
  for (int I=0;I<BUFFER_FILL_SIZE;I++)
  {
    buffer[I]='A';
  }

  for (int I=0;I<DUMP_SIZE;I++)
  {
    dumped_data[I]=0x00;
  }
}



void initialize_eeprom_memory(char pattern)
{
  //Fills the buffer and then fill the SPI memory with the content of the buffer
  for (int I=0;I<BUFFER_FILL_SIZE;I++)
  {
    buffer[I] = pattern;
  }

  char buf[] = "THIS IS A HIDDEN STRING, SOMEONE SHOULD FIND THIS ON THIS CHIP       KEY : 65FA45AE6985632565DEFC0FFF65A3B5  3456789101234567891";
  int base = 60;
  for (int I=0;I<sizeof(buf);I++)
  {
    buffer[base+I] = buf[I];
  }

  Serial.print("We are going to write the following content to the EEPROM");
  print_ascii_dump(buffer, BUFFER_FILL_SIZE);
  address_write=0;
  
  for (int I=0;I<BUFFER_FILL_SIZE;I++)
  {
    //fill eeprom w/ buffer
    digitalWrite(SLAVESELECT,LOW);
    spi_transfer(WREN); //write enable
    digitalWrite(SLAVESELECT,HIGH);
    delay(10);
    digitalWrite(SLAVESELECT,LOW);
    if(I < 256)
    {
       spi_transfer(WRITE_BYTE_LOW_ADDRESS); //write instruction for lower addresses
    }
    else
    {
      spi_transfer(WRITE_BYTE_HIGH_ADDRESS); //write instruction for higher addresses
    }
   
    //spi_transfer((char)(address_write>>8));   //send MSByte address first
    spi_transfer((char)(address_write));      //send LSByte address
    spi_transfer(buffer[I]); //write data byte
    if(VERBOSE > 0)
    {
      Serial.print("We sent ");
      Serial.print(buffer[I]);
      Serial.print(" to the EEPROM at address ");
      Serial.print(address_write);
      Serial.print('\n');
    }
    address_write++;
    digitalWrite(SLAVESELECT,HIGH); //release chip
    Serial.print('.');
    delay(200);
  }
}

void print_hex_dump(char* dump_content, int len)
{
  /* This prints the dumped content to the serial port */
  int j = 0;
  char *p = dump_content;
  Serial.write('\n');
  for (int I=0;I<len;I++)
  {
    Serial.print(*p,HEX);
    Serial.print(' ');
    p++;
    j++;
    if(j==16)
    {
      j = 0;
      Serial.write('\n');
   }
  }
}

void print_ascii_dump(char* dump_content, int len)
{
  /* This prints the dumped content to the serial port */
  int j = 0;
  char *p = dump_content;
  Serial.write('\n');
  for (int I=0;I<len;I++)
  {
    //Serial.print(*p,HEX);
    Serial.write(*p);
    Serial.print(' ');
    p++;
    j++;
    if(j==16)
    {
      j = 0;
      Serial.write('\n');
   }
  }
}

byte read_eeprom(unsigned int EEPROM_address)
{
  //READ EEPROM
  int data = 0;
  digitalWrite(SLAVESELECT,LOW);
  spi_transfer(READ); //transmit read opcode
  spi_transfer((char)(EEPROM_address>>8));   //send MSByte address first
  spi_transfer((char)(EEPROM_address));      //send LSByte address
  data = spi_transfer(0xFF); //get data byte
  digitalWrite(SLAVESELECT,HIGH); //release chip, signal end transfer
  return data;
}

byte read_eeprom_long(unsigned int EEPROM_address_long)
{
  //READ EEPROM
  
  if(VERBOSE > 0)
  {
    Serial.print("Asking for READ from the EEPROM at address ");
    delay(1000);
    Serial.print(EEPROM_address_long);
    Serial.print("\n");
  }
  
  int data = 0;
  digitalWrite(SLAVESELECT,LOW);
  if(EEPROM_address_long < 256)
  {
    spi_transfer(READ_BYTE_LOW_ADDRESS); //transmit read opcode for low address
  }
  else
  {
    spi_transfer(READ_BYTE_HIGH_ADDRESS); //transmit read opcode for high address
  }

  //int res = spi_transfer((char)(EEPROM_address_long>>24));   //send MSByte address first
  //Serial.print(res);
  //res = spi_transfer((char)(EEPROM_address_long>>16));  
  //Serial.print(res); 
  
  //int res = spi_transfer((char)(EEPROM_address_long>>8));
  //Serial.print(res);   
  int res = spi_transfer((char)(EEPROM_address_long));      //send LSByte address
  //Serial.print(res);
  //Serial.write('\n');
  data = spi_transfer(0xFF); //get data byte
  digitalWrite(SLAVESELECT,HIGH); //release chip, signal end transfer
  return data;
}

/****************************************************************************************************
 * 
 *                                            SETUP
 * 
 *****************************************************************************************************/

void setup()
{
  Serial.begin(9600);

  pinMode(DATAOUT, OUTPUT);
  pinMode(DATAIN, INPUT);
  pinMode(SPICLOCK,OUTPUT);
  pinMode(SLAVESELECT,OUTPUT);
  digitalWrite(SLAVESELECT,HIGH); //disable device
  // SPCR = 01010000
  //interrupt disabled,spi enabled,msb 1st,master,clk low when idle,
  //sample on leading edge of clk,system clock/4 rate (fastest)
  SPCR = (1<<SPE)|(1<<MSTR);
  clr=SPSR;
  clr=SPDR;
  delay(10);
  //fill buffer with data
  initialize_buffer_and_dump();

  initialize_eeprom_memory('=');
  
  delay(3000);
  Serial.write('H');
  Serial.write('i');
  Serial.write('\n');
  Serial.print("We just finished initializing the buffers, we can start dumping \n");
  delay(100);

  address_read = 0;
  cpt = 0;
}

/****************************************************************************************************
 * 
 *                                            LOOP
 * 
 *****************************************************************************************************/

void loop()
{
  eeprom_output_data = read_eeprom_long(address_read);
  if(VERBOSE > 0)
  {
    Serial.print(address_read);
    Serial.write('\t');
    //Serial.print(eeprom_output_data,HEX);
    Serial.write(eeprom_output_data);
    Serial.write('\n');
  }
  dumped_data[cpt] = eeprom_output_data;
  cpt++;
  if (cpt >= DUMP_SIZE)
  {
    cpt = 0;
    //Serial.write(dumped_data);
    print_ascii_dump(dumped_data, DUMP_SIZE);
    Serial.write('\n');
  }
  else
  {
    Serial.write('.');
  }
  address_read++;
  if (address_read >= MAX_ADDRESS)
  {
    Serial.print("We have finished reading the address space of the EEPROM\n");
    address_read = 0;
    delay(100000); //pause for readability
  }  
  delay(100); //pause for readability
  
}
