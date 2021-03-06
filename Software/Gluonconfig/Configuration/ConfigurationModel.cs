﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Configuration;
using Communication.Frames.Configuration;

namespace Gluonpilot
{
    [Serializable]
    public class ConfigurationModel
    {
        public PidModel Roll2AileronPidModel = new PidModel();
        public PidModel Pitch2ElevatorPidModel = new PidModel();
        public PidModel Heading2RollPidModel = new PidModel();
        public PidModel Altitude2PitchPidModel = new PidModel();
    
        public bool ReverseServo1;
        public bool ReverseServo2;
        public bool ReverseServo3;
        public bool ReverseServo4;
        public bool ReverseServo5;
        public bool ReverseServo6;

        public int[] ServoMin = new int[6];
        public int[] ServoMax = new int[6];
        public int[] ServoNeutral = new int[6];
        public bool ManualTrim;

        public int GpsInitialBaudrate;
        public int GpsOperationalBaudrate;
        public int GpsEnableWaas;

        public int TelemetryGyroAccRaw;
        public int TelemetryGyroAccProc;
        public int TelemetryPpm;
        public int TelemetryGpsBasic;
        public int TelemetryPressureTemp;
        public int TelemetryAttitude;
        public int TelemetryControl;

        public int NeutralAccX;
        public int NeutralAccY;
        public int NeutralAccZ;
        public int NeutralGyroX;
        public int NeutralGyroY;
        public int NeutralGyroZ;

        public int ImuRotated;
        public int NeutralPitch;

        public int ChannelRoll;
        public int ChannelPitch;
        public int ChannelYaw;
        public int ChannelMotor;
        public int ChannelAp;
        public int RcTransmitterFromPpm;

        public double ControlMaxPitch;
        public double ControlMinPitch;
        public double ControlMaxRoll;
        public int ControlMixing;
        public int ControlAileronDiff;
        public double CruisingSpeed;
        public double dummy;
        public double WaypointRadius;
        public bool StabilizationWithAltitudeHold;

        public int AutoThrottleMinPct;
        public int AutoThrottleMaxPct;
        public int AutoThrottleCruisePct;
        public double AutoThrottlePGain;
        public bool AutoThrottleEnabled;

        public int AltitudeMode;

        public int OsdBitmask;


        /*!
         *    Converts to _model to AllConfig communication frame.
         */
        public AllConfig ToAllConfig()
        {
            Console.WriteLine("To all config");
            AllConfig ac = new AllConfig();
            ConfigurationModel _model = this;
            ac.acc_x_neutral = _model.NeutralAccX;
            ac.acc_y_neutral = _model.NeutralAccY;
            ac.acc_z_neutral = _model.NeutralAccZ;
            ac.gyro_x_neutral = _model.NeutralGyroX;
            ac.gyro_y_neutral = _model.NeutralGyroY;
            ac.gyro_z_neutral = _model.NeutralGyroZ;

            ac.imu_rotated = _model.ImuRotated;
            ac.neutral_pitch = _model.NeutralPitch;

            ac.channel_pitch = _model.ChannelPitch;
            ac.channel_roll = _model.ChannelRoll;
            ac.channel_motor = _model.ChannelMotor;
            ac.channel_yaw = _model.ChannelYaw;
            ac.channel_ap = _model.ChannelAp;
            ac.rc_ppm = _model.RcTransmitterFromPpm;

            ac.control_max_pitch = _model.ControlMaxPitch;
            ac.control_min_pitch = _model.ControlMinPitch;
            ac.control_max_roll = _model.ControlMaxRoll;
            ac.control_mixing = _model.ControlMixing;
            ac.control_waypoint_radius = _model.WaypointRadius;
            ac.control_stabilization_with_altitude_hold = _model.StabilizationWithAltitudeHold;
            ac.control_cruising_speed = _model.CruisingSpeed;
            ac.control_aileron_differential = _model.ControlAileronDiff;
            ac.control_altitude_mode = _model.AltitudeMode;

            ac.telemetry_basicgps = _model.TelemetryGpsBasic;
            ac.telemetry_gyroaccraw = _model.TelemetryGyroAccRaw;
            ac.telemetry_gyroaccproc = _model.TelemetryGyroAccProc;
            ac.telemetry_ppm = _model.TelemetryPpm;
            ac.telemetry_pressuretemp = _model.TelemetryPressureTemp;
            ac.telemetry_attitude = _model.TelemetryAttitude;
            ac.telemetry_control = _model.TelemetryControl;

            ac.gps_initial_baudrate = _model.GpsInitialBaudrate;
            ac.gps_enable_waas = _model.GpsEnableWaas;

            ac.pid_pitch2elevator_p = _model.Pitch2ElevatorPidModel.P;
            ac.pid_pitch2elevator_i = _model.Pitch2ElevatorPidModel.I;
            ac.pid_pitch2elevator_d = _model.Pitch2ElevatorPidModel.D;
            ac.pid_pitch2elevator_imin = _model.Pitch2ElevatorPidModel.IMin;
            ac.pid_pitch2elevator_imax = _model.Pitch2ElevatorPidModel.IMax;
            ac.pid_pitch2elevator_dmin = _model.Pitch2ElevatorPidModel.DMin;

            ac.pid_roll2aileron_p = _model.Roll2AileronPidModel.P;
            ac.pid_roll2aileron_i = _model.Roll2AileronPidModel.I;
            ac.pid_roll2aileron_d = _model.Roll2AileronPidModel.D;
            ac.pid_roll2aileron_imin = _model.Roll2AileronPidModel.IMin;
            ac.pid_roll2aileron_imax = _model.Roll2AileronPidModel.IMax;
            ac.pid_roll2aileron_dmin = _model.Roll2AileronPidModel.DMin;

            ac.pid_heading2roll_p = _model.Heading2RollPidModel.P;
            ac.pid_heading2roll_i = _model.Heading2RollPidModel.I;
            ac.pid_heading2roll_d = _model.Heading2RollPidModel.D;
            ac.pid_heading2roll_imin = _model.Heading2RollPidModel.IMin;
            ac.pid_heading2roll_imax = _model.Heading2RollPidModel.IMax;
            ac.pid_heading2roll_dmin = _model.Heading2RollPidModel.DMin;

            ac.pid_altitude2pitch_p = _model.Altitude2PitchPidModel.P;
            ac.pid_altitude2pitch_i = _model.Altitude2PitchPidModel.I;
            ac.pid_altitude2pitch_d = _model.Altitude2PitchPidModel.D;
            ac.pid_altitude2pitch_imin = _model.Altitude2PitchPidModel.IMin;
            ac.pid_altitude2pitch_imax = _model.Altitude2PitchPidModel.IMax;
            ac.pid_altitude2pitch_dmin = _model.Altitude2PitchPidModel.DMin;


            ac.servo_reverse[0] = _model.ReverseServo1;
            ac.servo_reverse[1] = _model.ReverseServo2;
            ac.servo_reverse[2] = _model.ReverseServo3;
            ac.servo_reverse[3] = _model.ReverseServo4;
            ac.servo_reverse[4] = _model.ReverseServo5;
            ac.servo_reverse[5] = _model.ReverseServo6;
            ac.manual_trim = _model.ManualTrim;
            ac.servo_min[0] = _model.ServoMin[0];
            ac.servo_min[1] = _model.ServoMin[1];
            ac.servo_min[2] = _model.ServoMin[2];
            ac.servo_min[3] = _model.ServoMin[3];
            ac.servo_min[4] = _model.ServoMin[4];
            ac.servo_min[5] = _model.ServoMin[5];
            ac.servo_max[0] = _model.ServoMax[0];
            ac.servo_max[1] = _model.ServoMax[1];
            ac.servo_max[2] = _model.ServoMax[2];
            ac.servo_max[3] = _model.ServoMax[3];
            ac.servo_max[4] = _model.ServoMax[4];
            ac.servo_max[5] = _model.ServoMax[5];
            ac.servo_neutral[0] = _model.ServoNeutral[0];
            ac.servo_neutral[1] = _model.ServoNeutral[1];
            ac.servo_neutral[2] = _model.ServoNeutral[2];
            ac.servo_neutral[3] = _model.ServoNeutral[3];
            ac.servo_neutral[4] = _model.ServoNeutral[4];
            ac.servo_neutral[5] = _model.ServoNeutral[5];
            ac.auto_throttle_enabled = _model.AutoThrottleEnabled;
            ac.auto_throttle_min_pct = _model.AutoThrottleMinPct;
            ac.auto_throttle_max_pct = _model.AutoThrottleMaxPct;
            ac.auto_throttle_cruise_pct = _model.AutoThrottleCruisePct;
            ac.auto_throttle_p_gain_10 = (int)(_model.AutoThrottlePGain * 10);

            ac.osd_bitmask = _model.OsdBitmask;

            return ac;
        }

        /*!
         *    Updates it's state according to allconfig
         */
        public void SetFromAllConfig(AllConfig ac)
        {
            ConfigurationModel _model = this;
            _model.NeutralAccX = ac.acc_x_neutral;
            _model.NeutralAccY = ac.acc_y_neutral;
            _model.NeutralAccZ = ac.acc_z_neutral;
            _model.NeutralGyroX = ac.gyro_x_neutral;
            _model.NeutralGyroY = ac.gyro_y_neutral;
            _model.NeutralGyroZ = ac.gyro_z_neutral;

            _model.NeutralPitch = ac.neutral_pitch;
            _model.ImuRotated = ac.imu_rotated;

            _model.TelemetryGpsBasic = ac.telemetry_basicgps;
            _model.TelemetryGyroAccRaw = ac.telemetry_gyroaccraw;
            _model.TelemetryGyroAccProc = ac.telemetry_gyroaccproc;
            _model.TelemetryPpm = ac.telemetry_ppm;
            _model.TelemetryPressureTemp = ac.telemetry_pressuretemp;
            _model.TelemetryAttitude = ac.telemetry_attitude;
            _model.TelemetryControl = ac.telemetry_control;

            _model.ChannelAp = ac.channel_ap;
            _model.ChannelPitch = ac.channel_pitch;
            _model.ChannelRoll = ac.channel_roll;
            _model.ChannelYaw = ac.channel_yaw;
            _model.ChannelMotor = ac.channel_motor;
            _model.RcTransmitterFromPpm = ac.rc_ppm;

            _model.ControlMixing = ac.control_mixing;
            _model.ControlMaxPitch = ac.control_max_pitch;
            _model.ControlMinPitch = ac.control_min_pitch;
            _model.ControlMaxRoll = ac.control_max_roll;
            _model.ControlAileronDiff = ac.control_aileron_differential;
            _model.AltitudeMode = ac.control_altitude_mode;

            _model.CruisingSpeed = ac.control_cruising_speed;
            _model.StabilizationWithAltitudeHold = ac.control_stabilization_with_altitude_hold;
            _model.WaypointRadius = ac.control_waypoint_radius;

            _model.GpsInitialBaudrate = ac.gps_initial_baudrate;
            _model.GpsOperationalBaudrate = ac.gps_operational_baudrate;
            _model.GpsEnableWaas = ac.gps_enable_waas;

            _model.Pitch2ElevatorPidModel = new PidModel(ac.pid_pitch2elevator_p,
                                                         ac.pid_pitch2elevator_i,
                                                         ac.pid_pitch2elevator_d,
                                                         ac.pid_pitch2elevator_imin,
                                                         ac.pid_pitch2elevator_imax,
                                                         ac.pid_pitch2elevator_dmin);
            _model.Roll2AileronPidModel = new PidModel(ac.pid_roll2aileron_p,
                                                         ac.pid_roll2aileron_i,
                                                         ac.pid_roll2aileron_d,
                                                         ac.pid_roll2aileron_imin,
                                                         ac.pid_roll2aileron_imax,
                                                         ac.pid_roll2aileron_dmin);
            _model.Heading2RollPidModel = new PidModel(ac.pid_heading2roll_p,
                                                         ac.pid_heading2roll_i,
                                                         ac.pid_heading2roll_d,
                                                         ac.pid_heading2roll_imin,
                                                         ac.pid_heading2roll_imax,
                                                         ac.pid_heading2roll_dmin);
            _model.Altitude2PitchPidModel = new PidModel(ac.pid_altitude2pitch_p,
                                                         ac.pid_altitude2pitch_i,
                                                         ac.pid_altitude2pitch_d,
                                                         ac.pid_altitude2pitch_imin,
                                                         ac.pid_altitude2pitch_imax,
                                                         ac.pid_altitude2pitch_dmin);
            _model.ReverseServo1 = ac.servo_reverse[0];
            _model.ReverseServo2 = ac.servo_reverse[1];
            _model.ReverseServo3 = ac.servo_reverse[2];
            _model.ReverseServo4 = ac.servo_reverse[3];
            _model.ReverseServo5 = ac.servo_reverse[4];
            _model.ReverseServo6 = ac.servo_reverse[5];

            // Due to an annoying bug
            _model.ServoMin = new int[6];
            _model.ServoMax = new int[6];
            _model.ServoNeutral = new int[6];

            for (int i = 0; i < 6; i++)
            {
                _model.ServoMin[i] = ac.servo_min[i];
                _model.ServoMax[i] = ac.servo_max[i];
                _model.ServoNeutral[i] = ac.servo_neutral[i];
            }
            _model.ManualTrim = ac.manual_trim;

            _model.AutoThrottleEnabled = ac.auto_throttle_enabled;
            _model.AutoThrottleMinPct = ac.auto_throttle_min_pct;
            _model.AutoThrottleMaxPct = ac.auto_throttle_max_pct;
            _model.AutoThrottleCruisePct = ac.auto_throttle_cruise_pct;
            _model.AutoThrottlePGain = (double)ac.auto_throttle_p_gain_10 / 10;

            _model.OsdBitmask = ac.osd_bitmask;
        }
    }
}
