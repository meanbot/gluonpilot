
void i2c_wait_acken();
 
// function iniates a start condition on bus
void i2c_start(void);

//function iniates a restart condition on bus
void i2c_restart(void);

//Resets the I2C bus to Idle
void reset_i2c_bus(void);

//This function will initialize the I2C(1) peripheral.
void i2c_init(void);

//basic I2C byte send
char send_i2c_byte(int data);

//function reads 1 byte of data, returns the read data
char i2c_read_byte(void);

//function reads data, returns the read data, with ack
//does not reset bus!!!
//char i2c_read_ack(void); 

// poll to check if a device is on the bus
//unsigned char I2Cpoll(char addr);

// write to an address
void I2Cwrite(char addr, char subaddr, char value);

//read from an address
char I2Cread(char addr, char subaddr);
