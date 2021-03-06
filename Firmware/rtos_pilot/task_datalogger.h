#ifndef DATALOGGER_H
#define DATALOGGER_H

#define MAX_INDEX 16 

// Uncomment this define if you want to use 50Hz raw data logging
//#define RAW_50HZ_LOG 1

struct LogIndex
{
	int  page_num;
	long date;
	long time;
	long boot_sequence; // Gps OK, Ppm OK, Reboot
};

extern struct LogIndex datalogger_index_table[MAX_INDEX];

#ifdef DETAILED_LOG

struct LogLine
{
	float gps_latitude_rad;
	float gps_longitude_rad;
	int gps_height_m;
	int gps_speed_m_s;
	int gps_heading;
	char gps_satellites;
	int desired_roll;
	int desired_pitch;
	int desired_heading;
	int desired_height;
	int pitch;
	//int pitch_acc;
	int roll;
	//int roll_acc;
	int yaw;
	//unsigned int acc_x;
	//unsigned int acc_y;
	//unsigned int acc_z;
	float acc_x_g;
	float acc_y_g;
	float acc_z_g;
	//unsigned int gyro_x;
	//unsigned int gyro_y;
	//unsigned int gyro_z;
	int p;
	int q;
	int r;
	int control_state;
	
	char temperature_c;
	int  height_m;
	int  navigation_code_line;
};

#elif defined RAW_50HZ_LOG

struct LogLine
{
	float gps_latitude_rad;
	float gps_longitude_rad;
	long  gps_time;
	//int gps_height_m;
	unsigned char gps_speed_m_s_10;
	unsigned char gps_heading_2;
	
	unsigned int acc_x;
	unsigned int acc_y;
	unsigned int acc_z;
	unsigned int gyro_x;
	unsigned int gyro_y;
	unsigned int gyro_z;
	//unsigned int idg500_vref;
	//int control_state;
	int roll;
	int pitch;
	int pitch_acc;
	
	int  height_m_5;
};

#else

struct LogLine
{
	double gps_latitude_rad;    // 8
	double gps_longitude_rad;   // 8
    unsigned long date;                  // 4
    unsigned long time;                  // 4
	int gps_height_m;           // 2
	unsigned char gps_speed_m_s;// 1
	int gps_heading;            // 2
	int pitch;                  // 2
	int roll;                   // 2
	int yaw;                    // 2

	char  control_state;        // 1
	char temperature_c;         // 1
	int  height_m;              // 2
	int  navigation_code_line;  // 2 
    unsigned int servo_trigger; // 2 = 44
};

#endif



void datalogger_init();
void datalogger_writeline(struct LogLine *line);
void datalogger_task( void *parameters );
void datalogger_format();
int datalogger_print_next_page(int index, void(*printer)(struct LogLine*));
int datalogger_print_next_page_of_all(int index, void(*printer)(struct LogLine*));

void datalogger_enable();
void datalogger_disable();



#endif // DATALOGGER_H
