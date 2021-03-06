/*****************************************************************************
 * @copyright
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 * *******************************************************************************/
#pragma once
#include <stdint.h> 
 
#ifdef _MSC_VER
#define __attribute__(x) 
#endif 

//Define the structures are shared between adsp and apps processor
#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_DATATYPES_3x1_VECTOR_SIZE 3
#define SENSOR_DATATYPES_3x3_MAT_SIZE    9

#define SENSOR_IMU_MAX_DESCRIPTION_LEN   256

   /**
   * Enum to define the possible types/usecases for which and IMU can be used for.
   * This type should be set by IMU data generators when registering with the sensor
   * server.  This will be used by the client's of the API to select the IMU type.
   **/
   typedef enum
   {
      SENSOR_IMU_TYPE_BEGIN = 0,
      SENSOR_IMU_TYPE_FLIGHT_PRIMARY, // used by flight stack.
      SENSOR_IMU_TYPE_FLIGHT_SECONDARY,   // used by flight stack.
      SENSOR_IMU_TYPE_CAMERA_NAV,         // potentially used by algorithms like VIO.
      SENSOR_IMU_TYPE_CAMERA_VIDEO,       // Indicates the IMU potentially mounted close to an hires camera.
      SENSOR_IMU_TYPE_CAMERA_STEREO,      // Indicated the IMU mounted on a Stereo Camera.
      SENSOR_IMU_TYPE_OTHER,
      SENSOR_IMU_TYPE_COUNT
   }sensor_imu_type;

   typedef enum
   {
      X_AXIS = 0,
      Y_AXIS,
      Z_AXIS,
      IMU_MAX_DIMENSIONS
   } sensor_imu_axis;


   /**
   * Enum to control the clock type of the apps processor to use when synchronizing the
   * adsp time to apps processor This is needed only of the implementation of the API
   * uses the ADSP.
   **/
   typedef enum
   {
      SENSOR_CLOCK_SYNC_TYPE_MONOTONIC = 0,
      SENSOR_CLOCK_SYNC_TYPE_REALTIME
   } sensor_clock_sync_type;

   /**
   * Structure for the IMU ids that includes the following:
   *  1. unique id,
   *  2. the function of the IMU and
   *  3. Specifies the amout of buffering done on the server for this IMU.
   *  4. a description.
   */
   typedef struct
   {
      int16_t         imu_id;
      sensor_imu_type imu_type;
      int32_t         imu_max_buffer_size;
      char            imu_description[SENSOR_IMU_MAX_DESCRIPTION_LEN];
   } sensor_imu_id;

   /**
   * Structure to store the IMU data
   * linear acceleration is in the units of g/sec^2
   * angular velocity is in rad/sec
   * temperature is in units of deg.C
   * Note:
   *     The field: timestamp_in_us - represents the timestamp adjusted to the apps processor.
   *     The field: raw_timestamp_in_us --> Represents the raw adsp timestamp of when the sample was taken.
   */
   typedef struct
   {
      /**
      * Timestamp in microseconds synced to same clock source for the camera
      * frame timestamp.
      * timestamp_in_us = raw_timestamp_in_us + ticks_imu_clock_behind_camera_clock
      */
      uint64_t timestamp_in_us;

      /**
      * Timestamp in microseconds from the source clock available to the IMU driver
      */
      uint64_t raw_timestamp_in_us;

      /**
      * Incremental counter on the IMU data, generated by the API.
      */
      uint32_t sequence_number;

      /**
      * Accelemeter data, raw or calibrated depending on the API used to get the data.
      * The units are g/sec^2
      */
      float    linear_acceleration[SENSOR_DATATYPES_3x1_VECTOR_SIZE];

      /**
      * Gyro data, raw or calibrated based on the API used to get the data.
      * The units are rad/sec
      */
      float    angular_velocity[SENSOR_DATATYPES_3x1_VECTOR_SIZE];

      /**
      * IMU Temperature reading in C.
      */
      float    temperature;
  // } sensor_imu;
   } __attribute__( (packed) ) sensor_imu;

   /**
   * Structure to hold the imu driver settings
   **/
   typedef struct
   {
      /**
      * if the driver is not initialized, this is set 0.
      * if the driver is initialized, this is set to 1
      **/
      int16_t is_initialized;

      /**
      * This sample rate is for both accel and gyro.
      * a value if -1 is invalid
      **/
      int16_t sample_rate_in_hz;

      /**
      * a value of -1 for any of the setting below is an invalid value.
      * The value of -1 is returned if the is_initialized==0
      **/
      int16_t compass_sample_rate_in_hz;
      int16_t accel_lpf_in_hz;
      int16_t gyro_lpf_in_hz;
   } __attribute__( (packed) ) sensor_mpu_driver_settings;


   /**
   * Struture to hold the Rotation Matrix as determined by the
   * flight stack.
   * This includes completed bias compensated IMU as well.
   * Note:
   *     The field: timestamp_in_us - represents the timestamp adjusted to the apps processor.
   *     The field: raw_timestamp_in_us --> Represents the raw adsp timestamp of when the sample was taken.
   */
   typedef struct
   {
      /**
      * Timestamp in microseconds synced to same clock source for the camera
      * frame timestamp.
      * timestamp_in_us = raw_timestamp_in_us + ticks_imu_clock_behind_camera_clock
      */
      uint64_t timestamp_in_us;

      /**
      * Timestamp in microseconds from the source clock available to the IMU driver
      */
      uint64_t raw_timestamp_in_us;

      /**
      * Incremental counter on the IMU data, generated by the API.
      */
      uint32_t sequence_number;

      /**
      * Accelemeter data, raw or calibrated depending on the API used to get the data.
      * The units are g/sec^2
      */
      float    linear_acceleration[SENSOR_DATATYPES_3x1_VECTOR_SIZE];

      /**
      * Gyro data, raw or calibrated based on the API used to get the data.
      * The units are rad/sec
      */
      float    angular_velocity[SENSOR_DATATYPES_3x1_VECTOR_SIZE];

      /**
      * Flight attitude rotation matrix(Rsb)
      */
      float rotation_matrix[SENSOR_DATATYPES_3x3_MAT_SIZE];
   } __attribute__( (packed) ) sensor_attitude;


   typedef struct  {
	  int16_t acc[3];
	  int16_t gyro[3];
      int16_t temp;
      uint64_t timestamp_in_us;
   } sensor_imu_raw;

#ifdef __cplusplus
}
#endif
