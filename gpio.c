/* --
 * -- File:	gpio.c
 * -- Date:	09.12.2022
 * -- Author:	gelk
 * --
 * ------------------------------------------------------------------
 */
#include "gpio.h"

void *virtual_gpio_base;

uint8_t fd;


/****************************************************************
 * Init Function for File based control
 ****************************************************************/

int8_t file_gpio_init (uint8_t gpio, char *direction)
{
    FILE *fp;
    char str[100];

    // Export gpio
    sprintf(str, "/sys/class/gpio/export");
    fp = fopen(str, "w");
    if (fp== NULL) {
        printf("Error opening file in gpio_init %s\n", str);
        return 1;
    }
    fprintf(fp, "%i", gpio);
    fflush(fp);
    fclose(fp);

    // Set direction
    sprintf(str, "/sys/class/gpio/gpio%i/direction", gpio);
    fp = fopen(str, "w");
    if (fp== NULL) {
        printf("Error opening file in set direction %s\n", str);
        return -1;
    }
    fprintf(fp, "%s", direction);
    fflush(fp);
    fclose(fp);

    return 0;
}

/****************************************************************
 set_edge_rising()
 ****************************************************************/

int set_edge_rising(int gpio)
{

    FILE *fp;
    char str[100];
    sprintf(str,"/sys/class/gpio/gpio%d/edge",gpio);
    fp = fopen(str,"w");
    if(fp== NULL){
        printf("Error opening file\n");
        return -1;
    }
    fprintf(fp,"rising");
    fclose(fp);
    return 0;
}

/****************************************************************
 * mmap virtual base calculation
 ****************************************************************/
uint8_t mmap_virtual_base()
{
    uint8_t  m_mfd;

    if ((m_mfd = open("/dev/mem", O_RDWR)) < 0)
    {
        printf("FAIL by open /dev/mem\n");
        return m_mfd;
    }
    // http://man7.org/linux/man-pages/man2/mmap.2.html
    virtual_gpio_base = (void *)mmap(NULL, sysconf(_SC_PAGE_SIZE), PROT_READ|PROT_WRITE, MAP_SHARED, m_mfd, GPIO_BASE_ADDR);
    close(m_mfd);

    if (virtual_gpio_base == MAP_FAILED)
    {
        return errno;
    }

    return 0;
}

/*
 * Init GPIO pins
*/
static void mmap_gpio_direction( uint8_t gpio, uint8_t direction)
{
    uint32_t *gpio_reg;
    /* set io direction */
    if (gpio <= 9)
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base);
    }
    else if ((gpio > 9) & (gpio < 20))
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_FSEL1);
    }
    else
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_FSEL2);
    }

    *gpio_reg &=  ~(0x7 << FSELX_OFFSET(gpio));              // Clear 3 bits of gpio
    *gpio_reg |= (direction << FSELX_OFFSET(gpio));

}

void mmap_gpio_set( uint8_t gpio, uint8_t value)
{
    uint32_t *gpio_reg;
    if (value == 1)
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_SET0);
    }
    else
    {
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_CLR0);
    }
    *gpio_reg =  (0x1 << gpio);
}

uint8_t mmap_gpio_read(uint8_t gpio)
{
    uint8_t value;
    uint32_t *gpio_reg;
        gpio_reg = (uint32_t *) (virtual_gpio_base + GPIO_LVL0);
    value = (*gpio_reg);

return value ;
}

/****************************************************************
 * i2c-dev
 ****************************************************************/
void
write_ctrl_register(uint8_t device_addr, uint8_t mcp_reg, uint8_t write_data){

    uint8_t wr_buf[2];
    char *i2c_device = "/dev/i2c-4";

    switch (device_addr)
    {
    case MATRIX_TO:
        i2c_device = "/dev/i2c-5";
        break;
    case MATRIX_FROM:
        i2c_device = "/dev/i2c-5";
        break;
    case DTMF_READ:
        i2c_device = "/dev/i2c-5";
        break;
    default:
        i2c_device = "/dev/i2c-4";
    }
//Switches the I2C bus depending on I2C device address


    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 174: Failed to open I2C device \n O_RDWR");
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Error gpio.c Line 179: Failed to set I2C device as slave.\n");

    }

    //Write I2C Register
    wr_buf[0]=mcp_reg; // Register Address
    wr_buf[1]=write_data; //
    if (write(fd,wr_buf,2) != 2)
        printf("Error gpio.c Line 188: Failed to write bus\n");


    close(fd);

}
/***************************************************************************
 * Function Reads the I2C Register
 ***************************************************************************/

uint8_t
read_ctrl_register(uint8_t device_addr, uint8_t mcp_reg){

    uint8_t register_data = 0;
    uint8_t rd_buf[1];
    uint8_t wr_buf[1];

    char *i2c_device = "/dev/i2c-4";
//Set the i2c Bus 4 or 5 depending on the I2Cchip beeing written
//Only MATRIX_TO, MATRIX_FROM and DTMF_I2C devices are on I2c-5
// All the rest is in I2C-4
    switch (device_addr)
    {
    case MATRIX_TO:
        i2c_device = "/dev/i2c-5";
        break;
    case MATRIX_FROM:
        i2c_device = "/dev/i2c-5";
        break;
    case DTMF_READ:
        i2c_device = "/dev/i2c-5";
        break;
    default:
        i2c_device = "/dev/i2c-4";
    }


    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 217: Failed to open I2C Bus \n O_RDWR");
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Error gpio.c Line 222: Failed to set I2C Bus as slave.\n");
        exit(1);
    }

    wr_buf[0] = mcp_reg;
    if (write(fd,wr_buf,1) != 1)
        printf("Error gpio.c Line 228: Failed to write from i2c bus\n");


    if (read(fd,rd_buf,1) != 1) {
        printf("Error gpio.c Line 232: Failed to read i2c bus\n");

    } else {
        register_data = rd_buf[0];
    }

    close(fd);
    return register_data;
}


void set_connections(uint8_t from, uint8_t to){

//Converts numeric to bit value
    uint8_t exch_lines[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

  write_ctrl_register(MATRIX_FROM, MCP_OLAT, exch_lines[from]);
  write_ctrl_register(MATRIX_TO, MCP_OLAT, exch_lines[to]);

}

void init_gpios(){

    int8_t gpio_err_msg [12]={0,0,0,0,0,0,0,0,0,0,0,0};
   uint8_t i;

    /****************************************************************
     * Initialize RPI I/Os
     ****************************************************************/

    //Initialize GPIOs as IN or OUT
        gpio_err_msg[0] = file_gpio_init(DC_LOOP_INT, "in");
        gpio_err_msg[1] = file_gpio_init(LOOP_CLOSED_N_1, "in");
        gpio_err_msg[2] = file_gpio_init(LOOP_CLOSED_N_2, "in");
        gpio_err_msg[3] = file_gpio_init(LOOP_CLOSED_N_3, "in");
        gpio_err_msg[4] = file_gpio_init(LOOP_CLOSED_N_4, "in");
        gpio_err_msg[5] = file_gpio_init(LOOP_CLOSED_N_5, "in");
        gpio_err_msg[6] = file_gpio_init(LOOP_CLOSED_N_6, "in");
        gpio_err_msg[7] = file_gpio_init(LOOP_CLOSED_N_7, "in");
        gpio_err_msg[8] = file_gpio_init(LOOP_CLOSED_N_8, "in");
        gpio_err_msg[9] = file_gpio_init(DTMF_INT, "in");
        gpio_err_msg[10] = file_gpio_init(RING_INDICATOR_N, "in");

        //Initialize GPIOs with edge trigger

        gpio_err_msg[11] = set_edge_rising(DC_LOOP_INT);

        for (i =0; i < 12; i++)
        {
            if (gpio_err_msg[i] < 0) {
                printf("ERROR: Failed to initialize GPIOs as in or out");
                exit(EXIT_FAILURE);
            }
}
        /****************************************************************
         * Initialize MCP I2C Controller Pins
         ****************************************************************/

    //Set Matrix Controllers to Output
    write_ctrl_register(MATRIX_FROM, MCP_IODIR, 0x00);
    write_ctrl_register(MATRIX_TO, MCP_IODIR, 0x00);
    //Set the AC/DC Registers to output
    write_ctrl_register(PHONE_DC, MCP_IODIR, 0x00);
    write_ctrl_register(PHONE_AC, MCP_IODIR, 0x00);
    //Set the AC/DC Registers to DC
    write_ctrl_register(PHONE_DC, MCP_OLAT, 0xff); // set to one
    write_ctrl_register(PHONE_AC, MCP_OLAT, 0x00); // set to zero
// Set the Connect control register to output and off
    write_ctrl_register(CONNECT_CTRL, MCP_IODIR, 0x00);
    write_ctrl_register(CONNECT_CTRL, MCP_OLAT, 0x00);
    //DTMF_READ set 0-3 as read, 4-7 as write
    write_ctrl_register(DTMF_READ, MCP_IODIR, 0x0f);
    /*******************************************************
    *** LOOP Detect Register
    *****************************************************/

    //LOOP Detect are input registers
    write_ctrl_register(LOOP_DETECT, MCP_IODIR, 0xff);
    // Polarity of the input signal
    write_ctrl_register(LOOP_DETECT, MCP_IPOL, 0x00);
    //Enables interrupts
    write_ctrl_register(LOOP_DETECT, MCP_GPINTEN, 0xff);
    //controlls rising or falling, not relevant since we
    //interrupt on both edges
    write_ctrl_register(LOOP_DETECT, MCP_DEFVAL, 0xff);
    write_ctrl_register(LOOP_DETECT, MCP_INTCON, 0x00);
    //Genearte Interrupt on rising and falling edge
    write_ctrl_register(LOOP_DETECT, MCP_IOCON, 0x02);
    write_ctrl_register(LOOP_DETECT, MCP_GPPU, 0x00);


}

uint8_t
hex2lines(uint8_t hex){
    //Converts a hex number from the arguments to a single bit
    //Define Array
    uint8_t exch_lines[9] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
    uint8_t lines =exch_lines[hex];

    return lines;
}
uint8_t
hex2notlines(uint8_t hex){
    //Converts a hex number from the arguments to a single bit
    //Returns low active bits
    uint8_t exch_lines[9] = {0xff, 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
    uint8_t lines =exch_lines[hex];

    return lines;
}
//
// Write Single Bit in I2C
//
void
write_mcp_bit(uint8_t device_addr, uint8_t mcp_reg , uint8_t bit_pos, char value){
    char rd_buf[0];
    char wr_buf[0];
    uint8_t cur_value = 0;
    uint8_t mask;

    char *i2c_device = "/dev/i2c-4";
//Set the i2c Bus 4 or 5 depending on the I2Cchip beeing written
//Only MATRIX_TO, MATRIX_FROM and DTMF_I2C devices are on I2c-5
// All the rest is in I2C-4
    switch (device_addr)
    {
    case MATRIX_TO:
        i2c_device = "/dev/i2c-5";
        break;
    case MATRIX_FROM:
        i2c_device = "/dev/i2c-5";
        break;
    case DTMF_READ:
        i2c_device = "/dev/i2c-5";
        break;
    default:
        i2c_device = "/dev/i2c-4";
    }

    if ((fd = open(i2c_device,O_RDWR)) < 0) {
        printf("Error gpio.c Line 335: Failed to open I2C Bus \n O_RDWR");
        exit(1);
    }

    if (ioctl(fd,I2C_SLAVE,device_addr) < 0) {
        printf("Failed to acquire I2C bus access and/or talk to slave.\n");
        exit(1);
    }

    wr_buf[0] = mcp_reg;
    if (write(fd,wr_buf,1) != 1)
        printf("Failed to write to the i2c bus.\n");

    if (read(fd,rd_buf,1) != 1) {
        /* ERROR HANDLING: i2c transaction failed */
        printf("Failed to read from the i2c bus.\n");
    } else {
        cur_value = rd_buf[0];
    }

    mask = 0x1 << bit_pos;
    if (value == 1){
        cur_value |= mask;
    }
    else {
        cur_value &= ~mask;
    }
    wr_buf[0] = mcp_reg; // Register Address
    wr_buf[1] = cur_value; //
    if (write(fd,wr_buf,2) != 2)
        printf("Failed to write to the i2c bus.\n");

    close(fd);
}

//Using pseudo interrupts via file system
int8_t wait_select(uint8_t sec, uint8_t usec, uint8_t gpio, bool timeout)
{
    int filepath = 0;
    char str[100];
    fd_set fd;
    int retval;
    struct timeval tv;

    sprintf(str,"/sys/class/gpio/gpio%d/value",gpio);
    filepath = open(str, O_RDONLY);
    FD_ZERO(&fd);
    FD_SET(filepath, &fd);
    tv.tv_sec = sec;
    tv.tv_usec = usec;
    read(filepath, str, 100);   // clear edge trigger with "read"
    if (timeout == true){
    retval = select(filepath+1, NULL, NULL, &fd, &tv);
    }
    else {
    retval = select(filepath+1, NULL, NULL, &fd, NULL);
    }
    //close(filepath);
    FD_CLR(filepath, &fd);

    return retval;
}

bool
loop_detected(){
uint8_t read_loop;
bool loop_closed = false;

 read_loop = read_ctrl_register(LOOP_DETECT, MCP_GPIO);
    if (read_loop < 0xff) {
        loop_closed = true;
    }
    else {
            loop_closed = false;
        }
    return loop_closed;
    }

/****************************************************************
 * Function reads Single GPIO via File System
 ****************************************************************/

int32_t gpio_read (uint32_t gpio)
{
    FILE *fp;
    char str[100];
    int32_t ret;

    sprintf(str, "/sys/class/gpio/gpio%i/value", gpio);
    fp = fopen(str, "r");
    if (fp== NULL) {
        printf("Error opening file in gpio_read %s\n", str);
        return -1;
    }
    fread(str, 101, 1, fp);
    sscanf(str, "%i", &ret);
    fclose(fp);

    return ret;
}

