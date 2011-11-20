/*! 
 *  Calculates the desired pitch and roll for navigation
 *
 *  @file     navigation.c
 *  @author   Tom Pycke
 *  @date     28-apr-2010
 *  @since    0.2
 */
 
#include <math.h>

// Include all FreeRTOS header files
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "FreeRTOS/croutine.h"
#include "FreeRTOS/semphr.h"

// Gluonpilot libraries
#include "dataflash/dataflash.h"
#include "ppm_in/ppm_in.h"

#include "configuration.h"
#include "sensors.h"
#include "navigation.h"
#include "common.h"
#include "trigger.h"

#define NAVIGATION_HZ 5  // 5 Hz

volatile struct NavigationData navigation_data;

float cos_latitude;
//! Convert latitude coordinates from radians into meters.
float latitude_meter_per_radian = 6363057.32484;

//! Convert longitude coordinates from radians into meters. 
//! This current value is only valid when you live around 50� N.
float longitude_meter_per_radian = 4107840.76433121;   // = Pi/180*(a / (1-e^2*sin(lat)^2)^(1/2))*cos(lat)
                                                                    // ~ cos(latitude) * latitude_meter_per_radian
void navigation_set_home();
float heading_rad_fromto (float diff_long, float diff_lat);
float distance_between_meter(float long1, float long2, float lat1, float lat2);
void navigation_do_circle(struct NavigationCode *current_code);
float get_variable(enum navigation_variable i);
int waypoint_reached(struct NavigationCode *current_code);
void convert_parameters_to_abs(int i);
struct NavigationCode * next_code(int current_codeline);

unsigned int ticks_counter = 0;

struct BatteryAlarm {
	float panic_v;
	float warning_v;
	int panic_line;
} battery_alarm;	


/*!
 *  Initializes the navigation.
 */
void navigation_init ()
{
	navigation_data.home_longitude_rad = 0.0;
	navigation_data.home_latitude_rad = 0.0;
	navigation_data.home_gps_height = 0.0;
	navigation_data.home_pressure_height = sensor_data.pressure_height;  // as opposed to GPS height!!

	navigation_data.desired_heading_rad = 0.0;
	//navigation_data.distance_next_waypoint = 0.0;
	
	navigation_data.airborne = 0;
	
	navigation_data.current_codeline = 0;
	navigation_data.last_code = 0;
	
	navigation_data.time_airborne_s = 0;
        navigation_data.time_block_s = 0;
	navigation_data.wind_heading_set = 0;
	navigation_data.relative_positions_calculated = 0;
	navigation_data.desired_throttle_pct = -1;
	navigation_load();
	
	battery_alarm.panic_v = 0.0;
	battery_alarm.warning_v = 0.0;
	battery_alarm.panic_line = -1;
}


void navigation_calculate_relative_position(int i)
{
	switch (navigation_data.navigation_codes[i].opcode)
	{
		case FROM_TO_REL:
                           navigation_data.navigation_codes[i].opcode = FROM_TO_ABS;
                           convert_parameters_to_abs(i);
                           break;
		case FLY_TO_REL:
                           navigation_data.navigation_codes[i].opcode = FLY_TO_ABS;
                           convert_parameters_to_abs(i);
                           break;
		case CIRCLE_REL:
                           navigation_data.navigation_codes[i].opcode = CIRCLE_ABS;
                           convert_parameters_to_abs(i);
                           break;
		case CIRCLE_TO_REL:
                           navigation_data.navigation_codes[i].opcode = CIRCLE_TO_ABS;
                           convert_parameters_to_abs(i);
                           break;
		case FLARE_TO_REL:
                           navigation_data.navigation_codes[i].opcode = FLARE_TO_ABS;
                           convert_parameters_to_abs(i);
                           break;
		case GLIDE_TO_REL:
                           navigation_data.navigation_codes[i].opcode = GLIDE_TO_ABS;
                           convert_parameters_to_abs(i);
                           break;
		default:
                           break;
	}	
}	

/*!
 *    Calculate absolute lat/lon positions for relative waypoints.
 */
void navigation_calculate_relative_positions()
{
	int i;
	for (i = 0; i < MAX_NAVIGATIONCODES; i++)
	{
		navigation_calculate_relative_position(i);
	}
	navigation_data.relative_positions_calculated = 1;
}


void convert_parameters_to_abs(int i)
{
    navigation_data.navigation_codes[i].x /= latitude_meter_per_radian;
    navigation_data.navigation_codes[i].x += navigation_data.home_latitude_rad;
    navigation_data.navigation_codes[i].y /= longitude_meter_per_radian;
    navigation_data.navigation_codes[i].y += navigation_data.home_longitude_rad;
}


void navigation_burn()
{
	dataflash_write(NAVIGATION_PAGE, sizeof(navigation_data.navigation_codes), (unsigned char*) & (navigation_data.navigation_codes));
}

	
void navigation_load()
{
	dataflash_read(NAVIGATION_PAGE, sizeof(navigation_data.navigation_codes), (unsigned char*) & (navigation_data.navigation_codes));
}	


void navigation_update()
{
	ticks_counter++;
	// keep our "time" up to date
	if (ticks_counter % NAVIGATION_HZ == 0)
	{
		navigation_data.time_airborne_s++;
        navigation_data.time_block_s++;
	}
	
	// Alarms, check every 10 seconds
	if (ticks_counter % (NAVIGATION_HZ * 10) == 0) // check for alarms every 10 seconds
	{
		if (sensor_data.battery_voltage_10 < (int)(battery_alarm.panic_v*10.0))
		{
			navigation_data.alarm_battery_panic++;
			if (battery_alarm.panic_line >= 0 && navigation_data.alarm_battery_panic == 1)  // only do this one time
				navigation_data.current_codeline = battery_alarm.panic_line;
		}
		else if (sensor_data.battery_voltage_10 < (int)(battery_alarm.warning_v*10.0))
			navigation_data.alarm_battery_warning++;
	}
	
	// Set the "home"-position
	if (!navigation_data.airborne)
	{ 
		if (/*(ppm.channel[config.control.channel_motor] > 1600 || control_state.simulation_mode) &&*/
		    /*sensor_data.gps.speed_ms >= 2.0 &&*/ sensor_data.gps.status == ACTIVE && sensor_data.gps.satellites_in_view >= 5)
		{
			navigation_data.time_airborne_s = 0.0;  // reset this to know the real time airborne
			navigation_data.airborne = 1;
			navigation_set_home();
			navigation_data.last_waypoint_latitude_rad = navigation_data.home_latitude_rad;
			navigation_data.last_waypoint_longitude_rad = navigation_data.home_longitude_rad;
			navigation_calculate_relative_positions();  // we should send the new waypoints or calculate relative positions on the fly
		}
		else
		{
			if (sensor_data.gps.status != ACTIVE || sensor_data.gps.satellites_in_view < 5)
			{
				navigation_data.home_pressure_height = sensor_data.pressure_height;  // as opposed to GPS height!!
				//printf("\r\nHome altitude set\r\n");
			}
			navigation_set_home(); // set temporary home, not airborne
			
			navigation_data.desired_pre_bank = 0.0;
			//navigation_data.current_codeline = 0;
			// also return home @ 100m height
			navigation_data.desired_heading_rad = navigation_heading_rad_fromto(sensor_data.gps.longitude_rad,
	                                                   		                    sensor_data.gps.latitude_rad);
            navigation_data.desired_altitude_agl = 100.0;
		}	
		//return;
	}
	// set initial heading on take-off (fixed wing)
	if (!navigation_data.wind_heading_set && sensor_data.gps.speed_ms >= 3.0)
	{
		navigation_data.wind_heading_set = 1;
		navigation_data.wind_heading = sensor_data.gps.heading_rad;
		navigation_data.time_airborne_s = 0.0;  // reset this to know the real time airborne
		// lock yaw
		sensor_data.yaw = sensor_data.gps.heading_rad;
	}
	

	struct NavigationCode *current_code = & navigation_data.navigation_codes[navigation_data.current_codeline];
	switch(current_code->opcode)
	{
		case CLIMB:
			navigation_data.desired_pre_bank = 0.0f;
			navigation_data.desired_throttle_pct = -1;

			if (navigation_data.wind_heading_set)
				navigation_data.desired_heading_rad = navigation_data.wind_heading;
			else 
				navigation_data.desired_heading_rad = sensor_data.gps.heading_rad;

			navigation_data.desired_altitude_agl = current_code->x + 1000.0f;
			if (sensor_data.pressure_height - navigation_data.home_pressure_height > current_code->x)
				navigation_data.current_codeline++;
			break;
		case FROM_TO_REL:
		case FROM_TO_ABS:
		{
			navigation_data.desired_pre_bank = 0.0f;
			navigation_data.desired_throttle_pct = -1;
			
			float leg_x = (current_code->x - navigation_data.last_waypoint_latitude_rad) * latitude_meter_per_radian;  // lat
  			float leg_y = (current_code->y - navigation_data.last_waypoint_longitude_rad) * longitude_meter_per_radian;  // lon
  			float leg2 = MAX(leg_x * leg_x + leg_y * leg_y, 1.f);
  			float nav_leg_progress = ((sensor_data.gps.latitude_rad - navigation_data.last_waypoint_latitude_rad) * latitude_meter_per_radian * leg_x + 
  			                          (sensor_data.gps.longitude_rad - navigation_data.last_waypoint_longitude_rad) * longitude_meter_per_radian * leg_y) / leg2;
  			float nav_leg_length = sqrtf(leg2);

			  /** distance of carrot (in meter) */
			float carrot = 4.0f * sensor_data.gps.speed_ms;
			
			//nav_leg_progress += MAX(carrot / nav_leg_length, 0.f);
			
			if (nav_leg_progress >= 1.0f) // did we pass (miss) the waypoint?
			{
				navigation_data.desired_heading_rad = navigation_heading_rad_fromto((float)(sensor_data.gps.longitude_rad - (double)current_code->y),
		                                                                            (float)(sensor_data.gps.latitude_rad - (double)current_code->x));
			}
			else
			{
				nav_leg_progress += MAX(carrot / nav_leg_length, 0.f); // fly towards carrot
				
				navigation_data.desired_heading_rad = navigation_heading_rad_fromto(
					(float)(sensor_data.gps.longitude_rad - (double)( navigation_data.last_waypoint_longitude_rad + nav_leg_progress * leg_y / longitude_meter_per_radian)),
		            (float)(sensor_data.gps.latitude_rad - (double)( navigation_data.last_waypoint_latitude_rad + nav_leg_progress * leg_x / latitude_meter_per_radian ) ) );
			}
				                                                         
	        navigation_data.desired_altitude_agl = current_code->a;
			
			if (waypoint_reached(current_code))
			{
				navigation_data.current_codeline++;
				navigation_data.last_waypoint_latitude_rad = current_code->x;
				navigation_data.last_waypoint_longitude_rad = current_code->y;
				navigation_data.last_waypoint_altitude_agl = navigation_data.desired_altitude_agl;
			}
			
			break;
		}
		case FLY_TO_REL:
		case FLY_TO_ABS:
			navigation_data.desired_pre_bank = 0.0;
			navigation_data.desired_throttle_pct = -1;
			
			navigation_data.desired_heading_rad = navigation_heading_rad_fromto((float)(sensor_data.gps.longitude_rad - (double)current_code->y),
	                                                                            (float)(sensor_data.gps.latitude_rad - (double)current_code->x));
	                                                         
	        navigation_data.desired_altitude_agl = current_code->a;
			
			if (waypoint_reached(current_code))
			{
				navigation_data.current_codeline++;
				navigation_data.last_waypoint_latitude_rad = current_code->x;
				navigation_data.last_waypoint_longitude_rad = current_code->y;
				navigation_data.last_waypoint_altitude_agl = navigation_data.desired_altitude_agl;
			}	
			break;
		
		case CIRCLE_REL:
		case CIRCLE_ABS:
			navigation_data.desired_throttle_pct = -1;
			navigation_do_circle(current_code);
			navigation_data.desired_altitude_agl = current_code->b;
			navigation_data.last_waypoint_latitude_rad = current_code->x;
			navigation_data.last_waypoint_longitude_rad = current_code->y;
			navigation_data.last_waypoint_altitude_agl = navigation_data.desired_altitude_agl;
			navigation_data.current_codeline++;
			break;
		case CIRCLE_TO_REL:
		case CIRCLE_TO_ABS:
		{
			struct NavigationCode code;
			// circle center = in between previous and current waypoint
			code.x = (navigation_data.last_waypoint_latitude_rad + current_code->x) / 2.0;
			code.y = (navigation_data.last_waypoint_longitude_rad + current_code->y) / 2.0;
			code.a = (int)(navigation_distance_between_meter(navigation_data.last_waypoint_longitude_rad, current_code->y, navigation_data.last_waypoint_latitude_rad, current_code->x))/2;
			
			// decide to turn right or left
			struct NavigationCode *next = next_code(navigation_data.current_codeline);
			float dir1 = navigation_heading_rad_fromto(navigation_data.last_waypoint_longitude_rad - current_code->y,
	                                                   navigation_data.last_waypoint_latitude_rad - current_code->x);
			float dir2 = navigation_heading_rad_fromto(current_code->y - next->y,
	                                                   current_code->x - next->x);
			float diffheading = dir1 - dir2;
			if (diffheading > DEG2RAD(180.0))
				diffheading -= DEG2RAD(360.0);
			else if (diffheading < DEG2RAD(-180.0))
				diffheading += DEG2RAD(360.0);
	                                                   
			if (diffheading > 0.0)
			{
				code.a = -code.a;
			}	
			code.b = current_code->b;  // altitude_agl
			navigation_data.desired_throttle_pct = -1;
			navigation_do_circle(&code);
			navigation_data.desired_altitude_agl = code.b;
			
			if (get_variable(ABS_ALT_AND_HEADING_ERR) < 20.0)
			{
				navigation_data.last_waypoint_latitude_rad = current_code->x;
				navigation_data.last_waypoint_longitude_rad = current_code->y;
				navigation_data.last_waypoint_altitude_agl = navigation_data.desired_altitude_agl;
				navigation_data.current_codeline++;
			}	
			break;
		}	
		case GOTO:
			if (current_code->a < 0)
				navigation_data.current_codeline = navigation_data.current_codeline + current_code->a;
			else
				navigation_data.current_codeline = current_code->a;
			break;
		case UNTIL_GR:
			if (get_variable(current_code->a) > current_code->x)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline--;
			break;
		case UNTIL_SM:
			//printf("-> %f < %f => %d\n\r", get_variable(current_code->a), current_code->x, (int)(get_variable(current_code->a) < current_code->x));
			if (get_variable(current_code->a) < current_code->x)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline--;
			break;
		case UNTIL_EQ:
			if (fabs(get_variable(current_code->a) - current_code->x) < 1e-6f)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline--;
			break;
		case UNTIL_NE:
			if (fabs(get_variable(current_code->a) - current_code->x) > 1e-6f)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline--;
			break;
		case IF_GR:
			if (get_variable(current_code->a) > current_code->x)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline += 2;
			break;
		case IF_SM:
			if (get_variable(current_code->a) < current_code->x)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline += 2;
			break;
		case IF_EQ:
			if (fabs(get_variable(current_code->a) - current_code->x) < 1e-6f)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline += 2;
			break;
		case IF_NE:
			if (fabs(get_variable(current_code->a) - current_code->x) > 1e-6f)
				navigation_data.current_codeline++;
			else
				navigation_data.current_codeline += 2;
			break;
		case SERVO_SET:
			servo_set_us(current_code->a, current_code->b);  // a = channel(0..7), b = microseconds (1000...2000)
			navigation_data.current_codeline++;
			break;
		case SERVO_TRIGGER:
		{
			trigger_servo(current_code->a, current_code->b, current_code->x);
		}	
			navigation_data.current_codeline++;
			break;
		case EMPTYCMD:
			navigation_data.desired_pre_bank = 0.0f;
			navigation_data.desired_throttle_pct = -1;
			navigation_data.current_codeline = 0;
			// also return home @ 100m height
			navigation_data.desired_heading_rad = navigation_heading_rad_fromto(sensor_data.gps.longitude_rad,
	                                                   		         sensor_data.gps.latitude_rad);
            navigation_data.desired_altitude_agl = 100.0f;
			break;
        case BLOCK:
            navigation_data.time_block_s = 0;
            navigation_data.current_codeline++;
            break;
        case FLARE_TO_REL:
        case FLARE_TO_ABS:
        {
			navigation_data.desired_pre_bank = 0.0f;
			
			navigation_data.desired_throttle_pct = current_code->b;
			
			float leg_x = (current_code->x - navigation_data.last_waypoint_latitude_rad) * latitude_meter_per_radian;  // lat
  			float leg_y = (current_code->y - navigation_data.last_waypoint_longitude_rad) * longitude_meter_per_radian;  // lon
  			float leg2 = MAX(leg_x * leg_x + leg_y * leg_y, 1.f);
  			float nav_leg_progress = ((sensor_data.gps.latitude_rad - navigation_data.last_waypoint_latitude_rad) * latitude_meter_per_radian * leg_x + 
  			                          (sensor_data.gps.longitude_rad - navigation_data.last_waypoint_longitude_rad) * longitude_meter_per_radian * leg_y) / leg2;
  			float nav_leg_length = sqrtf(leg2);

			  /** distance of carrot (in meter) */
			float carrot = 4.0f * sensor_data.gps.speed_ms;
			
			nav_leg_progress += MAX(carrot / nav_leg_length, 0.f);
			
			/*if (nav_leg_progress >= 1.0f)
			{
				navigation_data.desired_heading_rad = navigation_heading_rad_fromto((float)(sensor_data.gps.longitude_rad - (double)current_code->y),
		                                                                            (float)(sensor_data.gps.latitude_rad - (double)current_code->x));
			}
			else*/
				navigation_data.desired_heading_rad = navigation_heading_rad_fromto(
					(float)(sensor_data.gps.longitude_rad - (double)( navigation_data.last_waypoint_longitude_rad + nav_leg_progress * leg_y / longitude_meter_per_radian)),
		            (float)(sensor_data.gps.latitude_rad - (double)( navigation_data.last_waypoint_latitude_rad + nav_leg_progress * leg_x / latitude_meter_per_radian ) ) );
				                                                         
	        navigation_data.desired_altitude_agl = current_code->a;
		    break;	
		}
		case GLIDE_TO_REL:
        case GLIDE_TO_ABS:
        {
			navigation_data.desired_pre_bank = 0.0f;
			
			navigation_data.desired_throttle_pct = current_code->b;
			
			float leg_x = (current_code->x - navigation_data.last_waypoint_latitude_rad) * latitude_meter_per_radian;  // lat
  			float leg_y = (current_code->y - navigation_data.last_waypoint_longitude_rad) * longitude_meter_per_radian;  // lon
  			float leg2 = MAX(leg_x * leg_x + leg_y * leg_y, 1.f);
  			float nav_leg_progress = ((sensor_data.gps.latitude_rad - navigation_data.last_waypoint_latitude_rad) * latitude_meter_per_radian * leg_x + 
  			                          (sensor_data.gps.longitude_rad - navigation_data.last_waypoint_longitude_rad) * longitude_meter_per_radian * leg_y) / leg2;
  			float nav_leg_length = sqrtf(leg2);

			  /** distance of carrot (in meter) */
			float carrot = 4.0f * sensor_data.gps.speed_ms;
			
			float nav_leg_progress_aim = nav_leg_progress + MAX(carrot / nav_leg_length, 0.f);
			
			/*if (nav_leg_progress >= 1.0f)
			{
				navigation_data.desired_heading_rad = navigation_heading_rad_fromto((float)(sensor_data.gps.longitude_rad - (double)current_code->y),
		                                                                            (float)(sensor_data.gps.latitude_rad - (double)current_code->x));
			}
			else*/
				navigation_data.desired_heading_rad = navigation_heading_rad_fromto(
					(float)(sensor_data.gps.longitude_rad - (double)( navigation_data.last_waypoint_longitude_rad + nav_leg_progress_aim * leg_y / longitude_meter_per_radian)),
		            (float)(sensor_data.gps.latitude_rad - (double)( navigation_data.last_waypoint_latitude_rad + nav_leg_progress_aim * leg_x / latitude_meter_per_radian ) ) );
				                
			//nav_leg_progress -= MAX(carrot*0.75 / nav_leg_length, 0.f);     
	        
	        //navigation_data.desired_altitude_agl = navigation_data.last_waypoint_altitude_agl * (1.0-nav_leg_progress_aim) + current_code->a * (nav_leg_progress_aim);
	        
	        // hard_aim
	        float altitude_agl = (sensor_data.pressure_height - navigation_data.home_pressure_height);
	        //float desired_pitch = -fabs(atanf(altitude_agl / (nav_leg_length*(1.0-nav_leg_progress))));
	        float desired_pitch = -fabs(atanf(altitude_agl / (nav_leg_length*(1.0-nav_leg_progress_aim))));
	        navigation_data.desired_altitude_agl = desired_pitch / config.control.pid_altitude2pitch.p_gain + altitude_agl;
	        
	        if (desired_pitch > DEG2RAD(-3.0) && altitude_agl > 5 && sensor_data.gps.speed_ms > 1.0)
	        	navigation_data.desired_throttle_pct = 10;
	        
	        printf("\r\n%d: %d %d (%d-%d | %d)\r\n", (int)(nav_leg_progress*100.f), (int)navigation_data.desired_altitude_agl, (int)altitude_agl, (int)(desired_pitch/3.14*180.0), (int)(control_state.desired_pitch/3.14*180.0), (int)(sensor_data.pitch/3.14*180.0));
		    break;	
		}
		case SET_LOITER_POSITION:
			navigation_data.loiter_waypoint_latitude_rad = sensor_data.gps.latitude_rad;
			navigation_data.loiter_waypoint_longitude_rad = sensor_data.gps.longitude_rad;
			navigation_data.loiter_waypoint_altitude_agl = sensor_data.pressure_height - navigation_data.home_pressure_height;
			navigation_data.current_codeline++;
			break;
		case LOITER_CIRCLE:
		{
			struct NavigationCode code;
			code.x = navigation_data.loiter_waypoint_latitude_rad;
			code.y = navigation_data.loiter_waypoint_longitude_rad;
			code.a = current_code->a; // radius
			code.b = navigation_data.loiter_waypoint_altitude_agl; // altitude agl
			navigation_data.desired_throttle_pct = -1;
			navigation_do_circle(&code);
			navigation_data.desired_altitude_agl = code.b;
			navigation_data.last_waypoint_latitude_rad = code.x;
			navigation_data.last_waypoint_longitude_rad = code.y;
			navigation_data.last_waypoint_altitude_agl = navigation_data.desired_altitude_agl;
			navigation_data.current_codeline++;
			break;
		}	
		case SET_BATTERY_ALARM:
			battery_alarm.panic_v = current_code->y;
			battery_alarm.warning_v = current_code->x;
			battery_alarm.panic_line = current_code->a;
			navigation_data.current_codeline++;
			break;
		default:
			navigation_data.desired_pre_bank = 0.0f;
			navigation_data.current_codeline = 0;
			// also return home @ 100m height
			navigation_data.desired_heading_rad = navigation_heading_rad_fromto(sensor_data.gps.longitude_rad,
	                                                   		         sensor_data.gps.latitude_rad);
	        navigation_data.desired_altitude_agl = 100.0f; 
			break;
	
	}	
}


/*!
 *   Are we flying towards or away from the waypoint?
 */
int flying_towards_waypoint(struct NavigationCode *current_code)
{
	float heading_error_rad = navigation_data.desired_heading_rad - sensor_data.gps.heading_rad;
		
	if (heading_error_rad >= DEG2RAD(180.0f))
		heading_error_rad -= DEG2RAD(360.0f);
	else if (heading_error_rad <= DEG2RAD(-180.0f))
		heading_error_rad += DEG2RAD(360.0f);
	
	if (fabs(heading_error_rad) > DEG2RAD(90.0f))
		return 0;
	else
		return 1;	
}

	
int waypoint_reached(struct NavigationCode *current_code)
{
	if (navigation_distance_between_meter(sensor_data.gps.longitude_rad, current_code->y,
			                              sensor_data.gps.latitude_rad, current_code->x) < config.control.waypoint_radius_m)
	{
		if (! flying_towards_waypoint(current_code))  // we are flying away from the waypoint AND we are close enough
			return 1;
		else
			return 0;
	} 
	return 0;
}
	

float get_variable(enum navigation_variable i)
{
	switch (i)
	{
		case HEIGHT:
			return sensor_data.pressure_height - navigation_data.home_pressure_height;
		case SPEED_MS:
			return sensor_data.gps.speed_ms;
		case HEADING_DEG:
			return RAD2DEG(sensor_data.gps.heading_rad);
		case FLIGHT_TIME_S:
			return (float)navigation_data.time_airborne_s;
		case SATELLITES_IN_VIEW:
			return sensor_data.gps.satellites_in_view;
		case HOME_DISTANCE:
			return navigation_distance_between_meter(sensor_data.gps.longitude_rad, navigation_data.home_longitude_rad,
			                                         sensor_data.gps.latitude_rad, navigation_data.home_latitude_rad);
		case PPM_LINK_ALIVE:
			return ppm.connection_alive ? 1.0f : 0.0f;
		case CHANNEL_1:
			return (float)ppm.channel[0];
		case CHANNEL_2:
			return (float)ppm.channel[1];
		case CHANNEL_3:
			return (float)ppm.channel[2];
		case CHANNEL_4:
			return (float)ppm.channel[3];
		case CHANNEL_5:
			return (float)ppm.channel[4];
		case CHANNEL_6:
			return (float)ppm.channel[5];
		case CHANNEL_7:
			return (float)ppm.channel[6];
		case CHANNEL_8:
			return (float)ppm.channel[7];
		case BATT_V:
			return (float)(sensor_data.battery_voltage_10)/10.0f;
        case BLOCK_TIME:
            return (float)navigation_data.time_block_s;
        case ABS_ALTITUDE_ERROR:
        	return fabs(control_state.desired_altitude - sensor_data.pressure_height);
        case ABS_HEADING_ERROR:
        {
	        struct NavigationCode *next_code = & navigation_data.navigation_codes[navigation_data.current_codeline+1];
	        if (next_code->opcode != FROM_TO_ABS && next_code->opcode != FLY_TO_ABS && next_code->opcode != CIRCLE_ABS && 
                next_code->opcode != FLARE_TO_ABS && next_code->opcode != GLIDE_TO_ABS || next_code->opcode != CIRCLE_TO_ABS)
            {
                next_code = & navigation_data.navigation_codes[navigation_data.current_codeline+2];
	            if (next_code->opcode != FROM_TO_ABS && next_code->opcode != FLY_TO_ABS && next_code->opcode != CIRCLE_ABS && 
	                next_code->opcode != FLARE_TO_ABS && next_code->opcode != GLIDE_TO_ABS || next_code->opcode != CIRCLE_TO_ABS)
	            {
	                next_code = & navigation_data.navigation_codes[navigation_data.current_codeline+3];
	            	if (next_code->opcode != FROM_TO_ABS && next_code->opcode != FLY_TO_ABS && next_code->opcode != CIRCLE_ABS && 
	                	next_code->opcode != FLARE_TO_ABS && next_code->opcode != GLIDE_TO_ABS || next_code->opcode != CIRCLE_TO_ABS)
	               		printf("\r\nBad ABS_HEADING_ERR position\r\n");
	            }   		
			}
			
            float heading_error = navigation_heading_rad_fromto((float)(sensor_data.gps.longitude_rad - (double)(next_code->y)),
	                                                           (float)(sensor_data.gps.latitude_rad - (double)(next_code->x)));
	        heading_error = RAD2DEG(heading_error - sensor_data.gps.heading_rad);
	        if (heading_error > 180.0f)
	        	heading_error -= 360.0f;
	        else if (heading_error < -180.0f)
	        	heading_error += 360.0f;
        	return fabs(heading_error);
	    } 
        case ABS_ALT_AND_HEADING_ERR:
        {
            struct NavigationCode *next = next_code(navigation_data.current_codeline);
			                
            float heading_error = navigation_heading_rad_fromto((float)(sensor_data.gps.longitude_rad - (double)(next->y)),
	                                                           (float)(sensor_data.gps.latitude_rad - (double)(next->x)));
			heading_error = RAD2DEG(heading_error - sensor_data.gps.heading_rad);
			//printf("\r\n%d\r\n", (int)heading_error);
	        if (heading_error > 180.0f)
	        	heading_error -= 360.0f;
	        else if (heading_error < -180.0f)
	        	heading_error += 360.0f;
        	return fabs(control_state.desired_altitude - sensor_data.pressure_height) + fabs(heading_error);
        }	
        default:
			return 0.0;
	}	
}	


struct NavigationCode * next_code(int current_codeline)
{
	struct NavigationCode *next = & navigation_data.navigation_codes[current_codeline+1];
	
	if (next->opcode != FROM_TO_ABS && next->opcode != FLY_TO_ABS && next->opcode != CIRCLE_ABS && 
        next->opcode != FLARE_TO_ABS && next->opcode != GLIDE_TO_ABS && next->opcode != CIRCLE_TO_ABS)
	{
		if (next->opcode == GOTO)
		{
			if (next->a >= 0)
				current_codeline = next->a - 2;
			else
				current_codeline = (current_codeline + 1) + next->a - 2;
		}		
		
		next = & navigation_data.navigation_codes[current_codeline+2];
		if (next->opcode != FROM_TO_ABS && next->opcode != FLY_TO_ABS && next->opcode != CIRCLE_ABS && 
            next->opcode != FLARE_TO_ABS && next->opcode != GLIDE_TO_ABS && next->opcode != CIRCLE_TO_ABS)
		{
			if (next->opcode == GOTO)
			{
				if (next->a >= 0)
					current_codeline = next->a - 3;
				else
					current_codeline = (current_codeline + 1) + next->a - 3;
			}
			next = & navigation_data.navigation_codes[current_codeline+3];
			if (next->opcode != FROM_TO_ABS && next->opcode != FLY_TO_ABS && next->opcode != CIRCLE_ABS && 
			    next->opcode != FLARE_TO_ABS && next->opcode != GLIDE_TO_ABS)
				printf("\r\nNext code not found!!\r\n");
		}   		
	}
	return next;
}
	

void navigation_do_circle(struct NavigationCode *current_code)
{
	float r = (float)current_code->a; // meter
	float rad_s = sensor_data.gps.speed_ms / r;   // rad/s for this circle
#define carrot 4.0
	float distance_ahead = carrot * sensor_data.gps.speed_ms;
	float abs_r = fabs(r);

	// heading towards center of circle
	float current_alpha = navigation_heading_rad_fromto(current_code->y - sensor_data.gps.longitude_rad,
	                                                    current_code->x - sensor_data.gps.latitude_rad);  // 0� = top of circle

	float distance_center = 
	 	navigation_distance_between_meter(sensor_data.gps.longitude_rad, current_code->y,
	                                      sensor_data.gps.latitude_rad, current_code->x);
	float next_alpha;
	float rad_ahead = rad_s*carrot;
	
	if (rad_ahead > 0.0f)
		rad_ahead = BIND(rad_ahead, DEG2RAD(10.0f), DEG2RAD(45.0f));   // because of /cos(rad_ahead)
	else
		rad_ahead = BIND(rad_ahead, DEG2RAD(-45.0f), DEG2RAD(-10.0f));   // because of /cos(rad_ahead)
		
		
	//if (distance_center > abs_r + distance_ahead ||
	//    distance_center < abs_r - distance_ahead)  // too far in or out of the circle?
	//{
	//	next_alpha = current_alpha + DEG2RAD(90.0); // fly to point where you would touch the circle
	//}
	//else
	//{
		/*if (rad_ahead > 3.14159/4.0)	
			next_alpha = current_alpha + 3.14159/4.0; // go to the position one second ahead
		if (rad_ahead < 3.14159/16.0)	
			next_alpha = current_alpha + 3.14159/16.0; // go to the position one second ahead
		else*/
			next_alpha = current_alpha + rad_ahead; //atan(distance_ahead/r);   CHANGE
	//}	

	
	navigation_data.desired_pre_bank = (distance_center > abs_r + distance_ahead*2.0 || 
	                                   distance_center < abs_r - distance_ahead) ? 0 :
  				                          atan(sensor_data.gps.speed_ms*sensor_data.gps.speed_ms / (G*r));

	float next_r = abs_r / cosf(rad_ahead); // CHANGE sqrt(r*r + distance_ahe^ ad*distance_ahead);
			
	// max desired_heading
	float pointlon = current_code->y + sinf(next_alpha) * next_r / longitude_meter_per_radian;
	float pointlat = current_code->x + cosf(next_alpha) * next_r / latitude_meter_per_radian;
	
	
	navigation_data.desired_heading_rad = navigation_heading_rad_fromto(sensor_data.gps.longitude_rad - pointlon,
		                                                                sensor_data.gps.latitude_rad - pointlat);
		                                                     
	if (navigation_data.desired_heading_rad > DEG2RAD(360.0f))
		navigation_data.desired_heading_rad -= DEG2RAD(360.0f);
	else if (navigation_data.desired_heading_rad < 0.0f)
		navigation_data.desired_heading_rad += DEG2RAD(360.0f);
		
	navigation_data.desired_altitude_agl = current_code->b;
	
	/*printf("-> %f | %f", distance_center, current_alpha);
	printf("(%f) %f\r\n", navigation_data.desired_pre_bank/3.14159*180.0, navigation_data.desired_heading_rad/3.14159*180.0);
	printf("(%f, %f) @ %d\r\n", RAD2DEG(current_code->x), RAD2DEG(current_code->y), current_code->a);*/
}	


/*!
 *  Save the current position as the GPS position.
 */
void navigation_set_home()
{
	navigation_data.home_longitude_rad = sensor_data.gps.longitude_rad;
	navigation_data.home_latitude_rad = sensor_data.gps.latitude_rad;
	navigation_data.home_gps_height = sensor_data.gps.height_m;
	
	cos_latitude = cos(sensor_data.gps.latitude_rad);
	longitude_meter_per_radian = latitude_meter_per_radian * cos_latitude;  // approx
	
	// set loiter position to home
	navigation_data.loiter_waypoint_latitude_rad = sensor_data.gps.latitude_rad;
	navigation_data.loiter_waypoint_longitude_rad = sensor_data.gps.longitude_rad;
	navigation_data.loiter_waypoint_altitude_agl = sensor_data.pressure_height - navigation_data.home_pressure_height;
}



/*!
 *  Calculates the heading from waypoint a to waypoint b.
 *  @param diff_long Origin longitude - destination longitude.
 *  @param diff_lat Origin latitude - destination latitude.
 */
float navigation_heading_rad_fromto (float diff_long, float diff_lat)
{
	//diff_lat *= cos_latitude;   // Local, flat earth approximation!
	diff_long *= cos_latitude;   // Local, flat earth approximation!
	
	float waypointHeading = atan2(diff_long, -diff_lat);

	// make clockwise direction positive (CCW is +ve as is)
	if(sin(diff_long) > 0.0)
		waypointHeading = (2.0*PI) - waypointHeading;
	else
		waypointHeading = -waypointHeading;

	return waypointHeading;
}


/*!
 *  Calculates the distance between 2 waypoints.
 *  Won't give good results if the waypoints are several 100 kms apart.
 */
float navigation_distance_between_meter(float long1, float long2, float lat1, float lat2)
{
	/* Haversine */
	//return acosf(sinf(lat1)*sinf(lat2) + cosf(lat1)*cosf(lat2)*cosf(long2-long1)) * 6371.0

	/*
		float difflong = (long1 - long2) * cos(lat1) * latitude_meter_per_radian;
		float difflat = (lat1 - lat2) * latitude_meter_per_radian;
	*/	
	
	/* simple: */
	float difflong = (long1 - long2) * longitude_meter_per_radian;
	float difflat = (lat1 - lat2) * latitude_meter_per_radian;

	return sqrt(difflong*difflong + difflat*difflat);
}
