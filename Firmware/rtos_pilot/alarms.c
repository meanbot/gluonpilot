#include <stdio.h>

#include "alarms.h"
#include "navigation.h"
#include "sensors.h"
#include "gluonscript.h"


struct BatteryAlarm battery_alarm = { .panic_v = 0.0, .warning_v = 0.0, .panic_line = -1, .alarm_battery_panic = 0, .alarm_battery_warning = 0};


ScriptHandlerReturn alarms_handle_gluonscriptcommand (struct GluonscriptCode *code)
{

	// Alarms, check every 10 seconds
	if (gluonscript_data.tick % (GLUONSCRIPT_HZ*10) == 0) // check for alarms every 10 seconds
	{
		if ((int)sensor_data.battery_voltage_10 < (int)(battery_alarm.panic_v*10.0))
		{
			//printf("%d < %d\r\n", sensor_data.battery_voltage_10, (int)(battery_alarm.panic_v*10.0));
			battery_alarm.alarm_battery_panic++;
			if (battery_alarm.panic_line >= 0 && battery_alarm.alarm_battery_panic == 1)  // only do this one time
			{
				gluonscript_data.current_codeline = battery_alarm.panic_line;
				//printf ("Goto %d\r\n", gluonscript_data.current_codeline);
				return HANDLED_UNFINISHED;
			}	
		}
		else if (sensor_data.battery_voltage_10 < (int)(battery_alarm.warning_v*10.0))
			battery_alarm.alarm_battery_warning++;
	}
	
	// handle command
	if (code->opcode == SET_BATTERY_ALARM)
	{
		battery_alarm.panic_v = code->y;
		battery_alarm.warning_v = code->x;
		battery_alarm.panic_line = code->a;
		return HANDLED_FINISHED;
	}	
	return NOT_HANDLED;
}	