// comunicator_loop.c

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "comunicator_loop.h"
#include "../uav_library/common.h"
#include "../uav_library/param/param.h"
#include "../uav_library/geo/geo.h"
#include "../uav_library/time/drv_time.h"
#include "../uav_library/io_ctrl/comunicator.h"
#include "../uav_library/io_ctrl/socket_io.h"

#include "../uav_type.h"
#include "FlightGear_exchanged_data.h"

#include "../ORB/ORB.h"
#include "../ORB/topics/parameter_update.h"
#include "../ORB/topics/vehicle_hil_attitude.h"
#include "../ORB/topics/sensors/battery_status.h"
#include "../ORB/topics/sensors/sensor_accel.h"
#include "../ORB/topics/sensors/airspeed.h"
#include "../ORB/topics/sensors/sensor_baro.h"
#include "../ORB/topics/sensors/sensor_combined.h"
#include "../ORB/topics/sensors/sensor_gyro.h"
#include "../ORB/topics/sensors/sensor_mag.h"
#include "../ORB/topics/sensors/sensor_optical_flow.h"
#include "../ORB/topics/position/vehicle_gps_position.h"
#include "../ORB/topics/position/vehicle_hil_global_position.h"
#include "../ORB/topics/actuator/actuator_outputs.h"


pthread_t comunicator_thread_id;
int comunicator_thread_return_value = 0;

static bool_t simulator_fully_loaded = 0;
static absolute_time start_time = 0;

#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
/* Subscribe to topics */
static struct actuator_outputs_s actuator_outputs;
#else
const float delta = 0.003;
const float max_thrust_value = 0.75;
float test_controls[4] = {0, 0, 0, 0};
#endif

/* Advertise topics */
#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
static orb_advert_t pub_hil_attitude = -1;
static orb_advert_t pub_hil_gps = -1;
static orb_advert_t pub_hil_global_pos = -1;
static orb_advert_t pub_hil_sensors = -1;
static orb_advert_t pub_hil_gyro = -1;
static orb_advert_t pub_hil_accel = -1;
static orb_advert_t pub_hil_flow = -1;
static orb_advert_t pub_hil_mag = -1;
static orb_advert_t pub_hil_baro = -1;
static orb_advert_t pub_hil_airspeed = -1;
static orb_advert_t pub_hil_battery = -1;
static struct vehicle_hil_attitude_s hil_attitude;
static struct vehicle_gps_position_s hil_gps;
static struct vehicle_hil_global_position_s hil_global_pos;
static struct sensor_combined_s hil_sensors;
static struct battery_status_s	hil_battery_status;
static struct airspeed_s airspeed;
static struct sensor_gyro_s gyro;
static struct sensor_accel_s accel;
static struct sensor_mag_s mag;
static struct sensor_optical_flow_s flow;
static struct sensor_baro_s baro;

/* packet counter */
static int hil_counter = 0;
static int hil_frames = 0;
static absolute_time old_timestamp = 0;

/* constants */
static const float g = 9.80665;
//static const float m2cm = 100.0;
static const float m2mm = 1000.0;
//static const float m2feets = 3.2808399;
//static const float sec2usec = 1.0e6;
static const float rad2degE7 = 1.0e7*180.0/M_PI;
//static const float rad2mrad = 1.0e3;
//static const float ga2mga = 1.0e3;
static const float rad2deg = 180.0/M_PI;
static const float deg2rad = 1/(180.0/M_PI), M_DEG_TO_RAD = 0.01745329251994;
static const float T0 = 273.15;
static const float R = 287.05;
static const float ground_press = 1.01325; //bar
//static const float ground_tempC = 21.0;
static const float r_earth = 6378100;
static const float bar2mbar = 1000.0;
static const float mrad2rad = 1.0e-3f;
static const float mg2ms2 = 9.80665f / 1000.0f;
static const float mga2ga = 1.0e-3f;
#endif


int parse_flight_data (double* flight_data, char* received_packet)
{
	//Parse UDP data and store into double array
	return	sscanf (received_packet, "%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n",
					&flight_data[FDM_FLIGHT_TIME], &flight_data[FDM_TEMPERATURE], &flight_data[FDM_PRESSURE],
					&flight_data[FDM_LATITUDE], &flight_data[FDM_LONGITUDE], &flight_data[FDM_ALTITUDE], &flight_data[FDM_GROUND_LEVEL],
					&flight_data[FDM_ROLL_ANGLE], &flight_data[FDM_PITCH_ANGLE], &flight_data[FDM_YAW_ANGLE],
					&flight_data[FDM_ROLL_RATE], &flight_data[FDM_PITCH_RATE], &flight_data[FDM_YAW_RATE],
					&flight_data[FDM_X_BODY_VELOCITY], &flight_data[FDM_Y_BODY_VELOCITY], &flight_data[FDM_Z_BODY_VELOCITY],
					&flight_data[FDM_X_EARTH_VELOCITY], &flight_data[FDM_Y_EARTH_VELOCITY], &flight_data[FDM_Z_EARTH_VELOCITY], &flight_data[FDM_AIRSPEED],
					&flight_data[FDM_X_BODY_ACCELERATION], &flight_data[FDM_Y_BODY_ACCELERATION], &flight_data[FDM_Z_BODY_ACCELERATION],
					&flight_data[FDM_ENGINE_ROTATION_SPEED], &flight_data[FDM_ENGINE_THRUST], 
					&flight_data[FDM_MAGNETIC_VARIATION], &flight_data[FDM_MAGNETIC_DIP]);
}

int create_output_packet (char* output_packet)
{
#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
	//Construct a packet to send over UDP with flight flight_outputs
	return	sprintf (output_packet, "%lf\t%lf\t%lf\t%lf\n",
					actuator_outputs.output[0],
					actuator_outputs.output[1],
					actuator_outputs.output[2],
					actuator_outputs.output[3]);
#else
	if (simulator_fully_loaded)
	{
		test_controls[3] = (test_controls[3] >= max_thrust_value)? max_thrust_value : test_controls[3] + delta;

		if (is_rotary_wing)
		{
			test_controls[0] = (test_controls[0] >= max_thrust_value)? max_thrust_value : test_controls[0] + delta;
			test_controls[1] = (test_controls[1] >= max_thrust_value)? max_thrust_value : test_controls[1] + delta;
			test_controls[2] = (test_controls[2] >= max_thrust_value)? max_thrust_value : test_controls[2] + delta;
		}
#endif
}


#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
int publish_flight_data (double* flight_data)
{
	int i, j;
	int return_value = 0;
	float pos_noise = 0, alt_noise = 0, vel_noise = 0; // maybe to be set as params
	float acc_noise = 0, gyro_noise = 0, mag_noise = 0, baro_noise = 0; // maybe to be set as params

	//float flight_time = flight_data[FDM_FLIGHT_TIME]*sec2usec;
	absolute_time time_usec = get_absolute_time();
	float tempC = flight_data[FDM_TEMPERATURE], tempK = tempC + T0;
	//float press = flight_data[FDM_PRESSURE];
	float lat = flight_data[FDM_LATITUDE] + pos_noise/r_earth;
	float lon = flight_data[FDM_LONGITUDE] + pos_noise*cos(lat)/r_earth;
	float alt = flight_data[FDM_ALTITUDE] + alt_noise;
	float ground_level = flight_data[FDM_GROUND_LEVEL] + alt_noise;

	float phi = flight_data[FDM_ROLL_ANGLE], cosPhi = cos(phi), sinPhi = sin(phi);
	float theta = flight_data[FDM_PITCH_ANGLE], cosThe = cos(theta), sinThe = sin(theta);
	float psi = flight_data[FDM_YAW_ANGLE], cosPsi = cos(psi), sinPsi = sin(psi);
	float phidot = flight_data[FDM_ROLL_RATE];
	float thetadot = flight_data[FDM_PITCH_RATE];
	float psidot = flight_data[FDM_YAW_RATE];

	float p = phidot - psidot*sinThe;
	float q = cosPhi*thetadot + sinPhi*cosThe*psidot;
	float r = -sinPhi*thetadot + cosPhi*cosThe*psidot;

	float vx = flight_data[FDM_X_BODY_VELOCITY];
	float vy = flight_data[FDM_Y_BODY_VELOCITY];
	float vz = flight_data[FDM_Z_BODY_VELOCITY];
	float ve = flight_data[FDM_X_EARTH_VELOCITY];
	float vn = flight_data[FDM_Y_EARTH_VELOCITY];
	float vd = flight_data[FDM_Z_EARTH_VELOCITY];
	//float  v = (sqrt(ve*ve + vn*vn + vd*vd))*m2cm;
	float tas = flight_data[FDM_AIRSPEED];

	float fix_type = 3;
	float satellites_visible = 10;
	float eph = 1.0;
	float epv = 5.0;
	float vel = sqrt(vn*vn + ve*ve) + vel_noise; // sog - speed on ground
	float heading_rad;
	float cog = atan2(ve, vn);
	cog = (cog < 0)? cog+2*M_PI : cog;
	cog = cog*rad2deg;

	float ers = flight_data[FDM_ENGINE_ROTATION_SPEED];
	float et = flight_data[FDM_ENGINE_THRUST];
	//float mv = flight_data[FDM_MAGNETIC_VARIATION];
	//float md = flight_data[FDM_MAGNETIC_DIP];

	float C_nb [3][3] = {
		{cosThe*cosPsi, -cosPhi*sinPsi + sinPhi*sinThe*cosPsi, sinPhi*sinPsi + cosPhi*sinThe*cosPsi},
		{cosThe*sinPsi, cosPhi*cosPsi + sinPhi*sinThe*sinPsi, -sinPhi*cosPsi + cosPhi*sinThe*sinPsi},
		{-sinThe, sinPhi*cosThe, cosPhi*cosThe}
	};

	float quat [4] = {
		cos(phi/2)*cos(theta/2)*cos(psi/2)+sin(phi/2)*sin(theta/2)*sin(psi/2),
		sin(phi/2)*cos(theta/2)*cos(psi/2)-cos(phi/2)*sin(theta/2)*sin(psi/2),
		cos(phi/2)*sin(theta/2)*cos(psi/2)+sin(phi/2)*cos(theta/2)*sin(psi/2),
		cos(phi/2)*cos(theta/2)*sin(psi/2)-sin(phi/2)*sin(theta/2)*cos(psi/2)
	};

	float magFieldStrength = 0.5;
	float dip = 60.0*deg2rad;	// XXX check if it is the same of md
	float dec = 0.0*deg2rad;
	float magVectN [3] = {
		magFieldStrength*cos(dip)*cos(dec),
		magFieldStrength*cos(dip)*sin(dec),
        magFieldStrength*sin(dip)
	};

	float magVectB [3] = {
		C_nb[0][0]*magVectN[0]+C_nb[1][0]*magVectN[1]+C_nb[2][0]*magVectN[2],
		C_nb[0][1]*magVectN[0]+C_nb[1][1]*magVectN[1]+C_nb[2][1]*magVectN[2],
		C_nb[0][2]*magVectN[0]+C_nb[1][2]*magVectN[1]+C_nb[2][2]*magVectN[2],
	};

	float xacc = flight_data[FDM_X_BODY_ACCELERATION] / mg2ms2 + acc_noise;
	float yacc = flight_data[FDM_Y_BODY_ACCELERATION] / mg2ms2  + acc_noise;
	float zacc = flight_data[FDM_Z_BODY_ACCELERATION] / mg2ms2  + acc_noise;
	float xgyro = p + gyro_noise;
	float ygyro = q + gyro_noise;
	float zgyro = r + gyro_noise;
	float xmag = magVectB[0] + mag_noise;
	float ymag = magVectB[1] + mag_noise;
	float zmag = magVectB[2] + mag_noise;

	float abs_pressure = (ground_press/exp(alt*(g/R)/tempK) + baro_noise)*bar2mbar;
	float diff_pressure = (0 + baro_noise)*bar2mbar;
	float pressure_alt = alt;

	absolute_time timestamp = get_absolute_time();

	/* sensors general */
	hil_sensors.timestamp = timestamp;

	/* hil gyro */
	hil_sensors.gyro_counter = hil_counter;
	hil_sensors.gyro_raw[0] = xgyro / mrad2rad;
	hil_sensors.gyro_raw[1] = ygyro / mrad2rad;
	hil_sensors.gyro_raw[2] = zgyro / mrad2rad;
	hil_sensors.gyro_rad_s[0] = xgyro;
	hil_sensors.gyro_rad_s[1] = ygyro;
	hil_sensors.gyro_rad_s[2] = zgyro;

	/* accelerometer */
	hil_sensors.accelerometer_counter = hil_counter;
	hil_sensors.accelerometer_raw[0] = xacc;
	hil_sensors.accelerometer_raw[1] = yacc;
	hil_sensors.accelerometer_raw[2] = zacc;
	hil_sensors.accelerometer_m_s2[0] = xacc * mg2ms2;
	hil_sensors.accelerometer_m_s2[1] = yacc * mg2ms2;
	hil_sensors.accelerometer_m_s2[2] = zacc * mg2ms2;
	hil_sensors.accelerometer_mode = 0; // TODO what is this?
	hil_sensors.accelerometer_range_m_s2 = 32.7f; // int16

	/* adc */
	hil_sensors.adc_voltage_v[0] = 0.0f;
	hil_sensors.adc_voltage_v[1] = 0.0f;
	hil_sensors.adc_voltage_v[2] = 0.0f;

	/* magnetometer */
	hil_sensors.magnetometer_counter = hil_counter;
	hil_sensors.magnetometer_raw[0] = xmag / mga2ga;
	hil_sensors.magnetometer_raw[1] = ymag / mga2ga;
	hil_sensors.magnetometer_raw[2] = zmag / mga2ga;
	hil_sensors.magnetometer_ga[0] = xmag;
	hil_sensors.magnetometer_ga[1] = ymag;
	hil_sensors.magnetometer_ga[2] = zmag;
	hil_sensors.magnetometer_range_ga = 32.7f; // int16
	hil_sensors.magnetometer_mode = 0; // TODO what is this
	hil_sensors.magnetometer_cuttoff_freq_hz = 50.0f;

	/* baro */
	hil_sensors.baro_counter = hil_counter;
	hil_sensors.baro_pres_mbar = abs_pressure;
	hil_sensors.baro_alt_meter = pressure_alt;
	hil_sensors.baro_temp_celcius = tempC;

	/* differential pressure */
	hil_sensors.differential_pressure_counter = hil_counter;
	hil_sensors.differential_pressure_pa = diff_pressure * 1e2f; //from hPa to Pa


	/* airspeed from differential pressure, ambient pressure and temp */
	//airspeed.indicated_airspeed_m_s = calc_indicated_airspeed(hil_sensors.differential_pressure_pa);
	airspeed.indicated_airspeed_m_s = tas;	// XXX airspeed should be obtained from differential pressure so here diff_press should be published
	airspeed.true_airspeed_m_s = tas;

	if (orb_publish (ORB_ID(airspeed), pub_hil_airspeed, &airspeed) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the airspeed topic\n");
		return_value = -1;
	}

	/* optical_flow used only for ground distance */
	flow.ground_distance_m = alt - ground_level;	// XXX optical flow is necessary for now (in the position_estimator) but maybe using position_estimator_mc it would be not

	if (orb_publish (ORB_ID(sensor_optical_flow), pub_hil_flow, &flow) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the optical_flow topic\n");
		return_value = -1;
	}

	/* individual sensor publications */
	gyro.x_raw = xgyro / mrad2rad;
	gyro.y_raw = ygyro / mrad2rad;
	gyro.z_raw = zgyro / mrad2rad;
	gyro.x = xgyro;
	gyro.y = ygyro;
	gyro.z = zgyro;
	gyro.temperature = tempC;

	if (orb_publish (ORB_ID(sensor_gyro), pub_hil_gyro, &gyro) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the sensor_gyro topic\n");
		return_value = -1;
	}

	accel.x_raw = xacc;
	accel.y_raw = yacc;
	accel.z_raw = zacc;
	accel.x = xacc * mg2ms2;
	accel.y = yacc * mg2ms2;
	accel.z = zacc * mg2ms2;
	accel.temperature = tempC;

	if (orb_publish (ORB_ID(sensor_accel), pub_hil_accel, &accel) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the sensor_accel topic\n");
		return_value = -1;
	}

	mag.x_raw = xmag / mga2ga;
	mag.y_raw = ymag / mga2ga;
	mag.z_raw = zmag / mga2ga;
	mag.x = xmag;
	mag.y = ymag;
	mag.z = zmag;

	if (orb_publish (ORB_ID(sensor_mag), pub_hil_mag, &mag) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the sensor_mag topic\n");
		return_value = -1;
	}

	baro.pressure = abs_pressure;
	baro.altitude = pressure_alt;
	baro.temperature = tempC;

	if (orb_publish (ORB_ID(sensor_baro), pub_hil_baro, &baro) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the sensor_baro topic\n");
		return_value = -1;
	}


	/* publish combined sensor topic */
	if (orb_publish (ORB_ID(sensor_combined), pub_hil_sensors, &hil_sensors) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the sensor_combined topic\n");
		return_value = -1;
	}


	/* fill in HIL battery status */
	hil_battery_status.voltage_v = 16.2f;	// XXX set to ncells*bat_v_full
	hil_battery_status.current_a = 10.0f;

	/* lazily publish the battery voltage */
	if (orb_publish (ORB_ID(battery_status), pub_hil_battery, &hil_battery_status) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the battery_status topic\n");
		return_value = -1;
	}

	// increment counters
	hil_counter++;
	hil_frames++;

	// output
	if (getenv("VERBOSE") && (timestamp - old_timestamp) > 15000000) {
		fprintf (stdout, "INFO: Receiving hil data from FlightGear at %d hz\n", hil_frames / 15);
		fflush (stdout);
		old_timestamp = timestamp;
		hil_frames = 0;
	}


	/* gps */
	hil_gps.timestamp_time = time_usec;
	hil_gps.time_gps_usec = time_usec;

	hil_gps.timestamp_position = time_usec;
	hil_gps.latitude = lat*rad2degE7;
	hil_gps.longitude = lon*rad2degE7;
	hil_gps.altitude = alt*m2mm;

	hil_gps.timestamp_variance = time_usec;
	hil_gps.eph_m = eph; // from cm to m
	hil_gps.epv_m = epv; // from cm to m
	hil_gps.s_variance_m_s = 5.0f;
	hil_gps.p_variance_m = hil_gps.eph_m * hil_gps.eph_m;
	hil_gps.c_variance_rad = 0; // XXX fix it
	hil_gps.fix_type = fix_type;

	/* gps.cog is in degrees 0..360, heading is -PI..+PI */
	heading_rad = cog * M_DEG_TO_RAD;

	/* go back to -PI..PI */
	if (heading_rad > M_PI)
		heading_rad -= 2.0f * M_PI;

	hil_gps.timestamp_velocity = time_usec;
	hil_gps.vel_m_s = vel; // from cm/s to m/s
	hil_gps.vel_n_m_s = vn;
	hil_gps.vel_e_m_s = ve;
	hil_gps.vel_d_m_s = vd;
	hil_gps.vel_ned_valid = 1;

	/* COG (course over ground) is spec'ed as -PI..+PI */
	hil_gps.cog_rad = heading_rad;

	hil_gps.timestamp_satellites = time_usec;
	hil_gps.satellites_visible = satellites_visible;
	/* publish GPS measurement data */
	if (orb_publish (ORB_ID(vehicle_gps_position), pub_hil_gps, &hil_gps) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the vehicle_gps_position topic\n");
		return_value = -1;
	}


	hil_global_pos.latitude = lat*rad2degE7;
	hil_global_pos.longitude = lon*rad2degE7;
	hil_global_pos.altitude = alt;
	hil_global_pos.vx = vx;
	hil_global_pos.vy = vy;
	hil_global_pos.vz = vz;
	hil_global_pos.yaw = _wrap_pi(psi);
	hil_global_pos.ground_level = ground_level;

	if (orb_publish (ORB_ID(vehicle_hil_global_position), pub_hil_global_pos, &hil_global_pos) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the vehicle_hil_global_position topic\n");
		return_value = -1;
	}


	/* set rotation matrix */
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			hil_attitude.R[i][j] = C_nb[i][j];

	hil_attitude.R_valid = 1;

	/* set quaternion */
	hil_attitude.q[0] = quat[0];
	hil_attitude.q[1] = quat[1];
	hil_attitude.q[2] = quat[2];
	hil_attitude.q[3] = quat[3];
	hil_attitude.q_valid = 1;

	hil_attitude.rate_offsets[0] = 0.0f;
	hil_attitude.rate_offsets[1] = 0.0f;
	hil_attitude.rate_offsets[2] = 0.0f;

	hil_attitude.roll = phi;
	hil_attitude.pitch = theta;
	hil_attitude.yaw = _wrap_pi(psi);
	hil_attitude.roll_rate = phidot;
	hil_attitude.pitch_rate = thetadot;
	hil_attitude.yaw_rate = psidot;
	hil_attitude.roll_acc = 0;
	hil_attitude.pitch_acc = 0;
	hil_attitude.yaw_acc = 0;

	hil_attitude.vx = vx;
	hil_attitude.vy = vy;
	hil_attitude.vz = vz;
	hil_attitude.ax = xacc * mg2ms2;
	hil_attitude.ay = yacc * mg2ms2;
	hil_attitude.az = zacc * mg2ms2;
	hil_attitude.engine_rotation_speed =  ers;
	hil_attitude.thrust =  et;

	if (orb_publish (ORB_ID(vehicle_hil_attitude), pub_hil_attitude, &hil_attitude) < 0)
	{
		fprintf (stderr, "Comunicator thread failed to publish the vehicle_hil_attitude topic\n");
		return_value = -1;
	}

	return return_value;
}
#endif


void* comunicator_loop (void* args)
{
	char received_data[INPUT_DATA_MAX_LENGTH];
	char data_sent[OUTPUT_DATA_MAX_LENGTH];
	double FlightGear_flight_data [FDM_N_PROPERTIES];		// FDM data
	struct sockaddr_in output_sockaddr;
	int i = 0;
	FILE *fp_in = NULL, *fp_out = NULL;

	if (getenv("EXPORT_FLAG"))
	{
		char input_file_name[30], output_file_name[50];
		strcpy (input_file_name, (char *) args);
		strcat(input_file_name, "_test_input_data.out");
		strcpy (output_file_name, (char *) args);
		strcat(output_file_name, "_test_output_data.out");

		fp_in = fopen((const char*) input_file_name, "w");
		fp_out = fopen((const char*) output_file_name, "w");
		if (fp_in == NULL || fp_out == NULL)
		{
			fprintf (stderr, "Can't create the output file to export the flight data\n");
			comunicator_thread_return_value = -1;
			pthread_exit(&comunicator_thread_return_value);
		}

		fprintf (fp_in, "Index\tTime\tTemperature\tPressure\tlatitude\tlongitude\taltitiude\tground_level\troll_angle\tpitch_angle\tyaw_angle\troll_rate\tpitch_rate\tyaw_rate\tx_body_velocity\ty_body_velocity\tz_body_velocity\tVelocity:_East\tVelocity_North\tVelocity_Down\tAirspeed\tX_body_accel\tY_body_accel\tZ_body_accel\tEngine_rotation\tEngine_thrust\tMagnetic_Variation\tMagnetic_Dip\n");
		if (is_rotary_wing)
			fprintf (fp_out, "Index\tthrottle0\tthrottle1\tthrottle2\tthrottle3\n");
		else
			fprintf (fp_out, "Index\taileron\televator\trudder\tthrottle\n");
	}

#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
	int poll_return_value;
	absolute_time usec_max_poll_wait_time = 100000;

	// ************************************* advertise ******************************************
	pub_hil_gps = orb_advertise (ORB_ID(vehicle_gps_position));
	if (pub_hil_gps == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the vehicle_gps_position topic\n");
		exit (-1);
	}

	pub_hil_global_pos = orb_advertise (ORB_ID(vehicle_hil_global_position));
	if (pub_hil_global_pos == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the vehicle_hil_global_position topic\n");
		exit (-1);
	}

	pub_hil_sensors = orb_advertise (ORB_ID(sensor_combined));
	if (pub_hil_sensors == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the sensor_combined topic\n");
		exit (-1);
	}

	pub_hil_gyro = orb_advertise (ORB_ID(sensor_gyro));
	if (pub_hil_gyro == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the sensor_gyro topic\n");
		exit (-1);
	}

	pub_hil_accel = orb_advertise (ORB_ID(sensor_accel));
	if (pub_hil_accel == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the sensor_accel topic\n");
		exit (-1);
	}

	pub_hil_mag = orb_advertise (ORB_ID(sensor_mag));
	if (pub_hil_mag == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the sensor_mag topic\n");
		exit (-1);
	}

	pub_hil_flow = orb_advertise (ORB_ID(sensor_optical_flow));
	if (pub_hil_flow == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the sensor_optical_flow topic\n");
		exit (-1);
	}

	pub_hil_baro = orb_advertise (ORB_ID(sensor_baro));
	if (pub_hil_baro == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the sensor_baro topic\n");
		exit (-1);
	}

	pub_hil_airspeed = orb_advertise (ORB_ID(airspeed));
	if (pub_hil_airspeed == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the airspeed topic\n");
		exit (-1);
	}

	pub_hil_battery = orb_advertise (ORB_ID(battery_status));
	if (pub_hil_battery == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the battery_status topic\n");
		exit (-1);
	}

	pub_hil_attitude = orb_advertise (ORB_ID(vehicle_hil_attitude));
	if (pub_hil_attitude == -1)
	{
		fprintf (stderr, "Comunicator thread failed to advertise the vehicle_hil_attitude topic\n");
		exit (-1);
	}


	// ************************************* subscribe ******************************************
	orb_subscr_t actuator_outputs_sub = orb_subscribe (ORB_ID_VEHICLE_CONTROLS);
	if (actuator_outputs_sub == -1)
	{
		fprintf (stderr, "Comunicator thread failed to subscribe the actuator_controls topic\n");
		exit (-1);
	}
#endif

	
	// **************************************** init sockets ******************************************
	if (!getenv("DO_NOT_SEND_CONTROLS") && client_address_initialization (&output_sockaddr, output_port, output_address) < 0)
	{
		fprintf (stderr, "Can't find the host on the given IP\n");
		comunicator_thread_return_value = -1;
		pthread_exit(&comunicator_thread_return_value);
	}
	
	fprintf (stdout, "\nTrying to establish a connection with FlightGear\n");
	fflush(stdout);
	
	if (getenv("TCP_FLAG"))
	{ //TCP
		fprintf (stdout, "Waiting for FlightGear on port %d\n\n", input_port);
		fflush(stdout);
		
		if (tcp_server_socket_initialization (&input_socket, input_port) < 0)
		{
			fprintf (stderr, "Can't create the input socket correctly\n");
			exit (-1);
		}
		if (!getenv("DO_NOT_SEND_CONTROLS") && tcp_client_socket_initialization (&output_socket, &output_sockaddr) < 0)
		{
			fprintf (stderr, "Can't create the output socket correctly\n");
			exit (-1);
		}
	}
	else
	{ //UDP
		if (udp_server_socket_initialization (&input_socket, input_port) < 0)
		{
			fprintf (stderr, "Can't create the input socket correctly\n");
			exit (-1);
		}
		
		if (!getenv("DO_NOT_SEND_CONTROLS") && udp_client_socket_initialization (&output_socket, output_port) < 0)
		{
			fprintf (stderr, "Can't create the output socket correctly\n");
			exit (-1);
		}

		fprintf (stdout, "Waiting for FlightGear on port %d\n\n", input_port);
		fflush(stdout);
	}


#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
	memset (&airspeed, 0, sizeof(airspeed));
	memset (&hil_attitude, 0, sizeof(hil_attitude));
	/*
	memset (&hil_global_pos, 0, sizeof(hil_global_pos));
#ifndef START_IN_AIR
	hil_global_pos.landed = 1;
#endif
	*/
#endif
	
	start_time = get_absolute_time();

	// **************************************** start primary loop ******************************************
	while (!_shutdown_all_systems)
	{
		// INPUTS
		if (receive_input_packet (received_data, INPUT_DATA_MAX_LENGTH, input_socket) <= 0)
		{
			sleep(0.5);
			
			if (!_shutdown_all_systems)
			{
				fprintf (stderr, "Can't receive the flight-data from FlightGear\n");
				comunicator_thread_return_value = -1;
			}
			
			break;
		}


		if (parse_flight_data (FlightGear_flight_data, received_data) < FDM_N_PROPERTIES)
		{
			// that's undesiderable but there is not much we can do
			fprintf (stderr, "Can't parse the flight-data from FlightGear\n");
		}
		

		// ******************************************* publish *******************************************
		if (!simulator_fully_loaded)
		{
			// precaution needed because on startup flightgear sends many wrong data
			if (FlightGear_flight_data[FDM_FLIGHT_TIME] > 2.0)
				simulator_fully_loaded = 1;
		}
#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
		else if (publish_flight_data (FlightGear_flight_data) < 0)
		{
			// that's undesiderable but there is not much we can do
			fprintf (stderr, "Can't publish flight sensors data\n");
		}
#endif


		if (!getenv("DO_NOT_SEND_CONTROLS"))
		{
#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
			// wait for the result of the autopilot logic calculation
			poll_return_value = orb_poll (ORB_ID_VEHICLE_CONTROLS, actuator_outputs_sub, usec_max_poll_wait_time);
			if (poll_return_value < 0)
			{
				// that's undesiderable but there is not much we can do
				fprintf (stderr, "Comunicator thread experienced an error waiting for actuator_controls topic\n");
			}
			else if (!poll_return_value)
			{
				// that's undesiderable but there is not much we can do
				//fprintf (stderr, "Comunicator thread experienced a timeout whike waiting for actuator_controls topic\n");
			}
			else
				orb_copy (ORB_ID_VEHICLE_CONTROLS, actuator_outputs_sub, (void *) &actuator_outputs);
#endif
			

			// OUTPUTS
			if (create_output_packet (data_sent) < CTRL_N_CONTROLS)
			{
				// that's undesiderable but there is not much we can do
				fprintf (stderr, "Can't create the packet to be sent to FlightGear\n");
			}
			

			if (send_output_packet (data_sent, strlen(data_sent), output_socket, &output_sockaddr) < strlen(data_sent))
			{
				sleep(0.5);
				
				if (!_shutdown_all_systems)
				{
					fprintf (stderr, "Can't send the packet to FlightGear\n");
					comunicator_thread_return_value = -1;
				}
				
				break;
			}


			if (getenv("EXPORT_FLAG"))
			{
				if (fp_in)
					fprintf (fp_in, "%d\t%s", i, received_data);
				if (fp_out)
					fprintf (fp_out, "%d\t%s", i, data_sent);
				i++;
			}
		}
	}
	
#ifndef BYPASS_OUTPUT_CONTROLS_AND_DO_UAV_MODEL_TEST_DEMO
	/*
	 * do unsubscriptions
	 */
	if (orb_unsubscribe (ORB_ID_VEHICLE_CONTROLS, actuator_outputs_sub, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unsubscribe to actuator_outputs topic\n");

	/*
	 * do unadvertises
	 */
	if (orb_unadvertise (ORB_ID(vehicle_gps_position), pub_hil_gps, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the vehicle_gps_position topic\n");

	if (orb_unadvertise (ORB_ID(vehicle_hil_global_position), pub_hil_global_pos, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the vehicle_hil_global_position topic\n");

	if (orb_unadvertise (ORB_ID(sensor_combined), pub_hil_sensors, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the sensor_combined topic\n");

	if (orb_unadvertise (ORB_ID(sensor_gyro), pub_hil_gyro, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the sensor_gyro topic\n");

	if (orb_unadvertise (ORB_ID(sensor_accel), pub_hil_accel, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the sensor_accel topic\n");

	if (orb_unadvertise (ORB_ID(sensor_mag), pub_hil_mag, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the sensor_mag topic\n");

	if (orb_unadvertise (ORB_ID(sensor_optical_flow), pub_hil_flow, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the sensor_optical_flow topic\n");

	if (orb_unadvertise (ORB_ID(sensor_baro), pub_hil_baro, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the sensor_baro topic\n");

	if (orb_unadvertise (ORB_ID(airspeed), pub_hil_airspeed, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the airspeed topic\n");

	if (orb_unadvertise (ORB_ID(battery_status), pub_hil_battery, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the battery_status topic\n");

	if (orb_unadvertise (ORB_ID(vehicle_hil_attitude), pub_hil_attitude, pthread_self()) < 0)
		fprintf (stderr, "Comunicator thread failed to unadvertise the vehicle_hil_attitude topic\n");
#endif

	if (getenv("EXPORT_FLAG"))
	{
		if (fp_in)
			fclose(fp_in);
		if (fp_out)
			fclose(fp_out);
	}

	pthread_exit(&comunicator_thread_return_value);
	return 0;
}
