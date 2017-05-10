
#include "sensors.h"
#include "sensor_manager.h"
#include "sensor_input_pdr.h"

static int mag_count = 0;
static int acc_count = 0;
static int gyro_count = 0;


static int gyro_operate(Sensor_Command command, void* buffer_in, int size_in, void* buffer_out, int size_out)
{
	return 0;
}


static int gyro_run_algorithm(struct data_t *output)
{
	if(output->data == NULL)
		return 0;
	output->data->sensor_type = SENSOR_TYPE_GYROSCOPE;
	output->data->gyroscope_t.azimuth = (int)sensor_input[gyro_count%1000][3];
	output->data->gyroscope_t.pitch= (int)sensor_input[gyro_count%1000][4];
	output->data->gyroscope_t.roll= (int)sensor_input[gyro_count%1000][5];
	output->data->gyroscope_t.status = 2;
	output->data->time_stamp = timestamp_get_ns()/1000000;
	gyro_count++;
	printf("gyro_run_algorithm(%d, %d, %d)\n\r", (int)sensor_input[gyro_count%1000][3], (int)sensor_input[gyro_count%1000][4], (int)sensor_input[gyro_count%1000][5]);
	return 0;
}

int FakeGYRO_Init(void)
{
	int ret;
	struct SensorDescriptor_t gyro_desp;

	gyro_desp.hw.max_sampling_rate = 5;//ms
	gyro_desp.hw.support_HW_FIFO = 0;
	gyro_desp.input_list = NULL;	
	gyro_desp.operate = gyro_operate;
	gyro_desp.run_algorithm = gyro_run_algorithm;
	gyro_desp.set_data = NULL;
	gyro_desp.sensor_type = SENSOR_TYPE_GYROSCOPE;
	gyro_desp.version = 0x0001;

	ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_GYROSCOPE, 1);
	if(ret < 0) {
		printf("Gyro_init sensor_subsys_algorithm_register_data_buffer error!!!\n\r");
	}

	ret = sensor_subsys_algorithm_register_type(&gyro_desp);
	if(ret < 0) {
		printf("Gyro_init sensor_subsys_algorithm_register_type error!!!\n\r");
	}
	return 0;
}

static int mag_operate(Sensor_Command command, void* buffer_in, int size_in, void* buffer_out, int size_out)
{
	return 0;
}


static int mag_run_algorithm(struct data_t *output)
{
	if(output->data == NULL)
		return 0;
	output->data->sensor_type = SENSOR_TYPE_MAGNETIC_FIELD;
	output->data->magnetic_t.x = (int)sensor_input[mag_count%1000][6]/10;
	output->data->magnetic_t.y = (int)sensor_input[mag_count%1000][7]/10;
	output->data->magnetic_t.z = (int)sensor_input[mag_count%1000][8]/10;
	output->data->magnetic_t.status = 2;
	output->data->time_stamp = timestamp_get_ns()/1000000;

	mag_count++;
	printf("mag_run_algorithm(%d, %d, %d)\n\r", (int)sensor_input[mag_count%1000][6], (int)sensor_input[mag_count%1000][7], (int)sensor_input[mag_count%1000][8]);
	return 0;
}

int FakeMAG_Init(void)
{
	int ret;
	struct SensorDescriptor_t mag_desp;

	mag_desp.hw.max_sampling_rate = 20;//ms
	mag_desp.hw.support_HW_FIFO = 0;
	mag_desp.input_list = NULL;	
	mag_desp.operate = mag_operate;
	mag_desp.run_algorithm = mag_run_algorithm;
	mag_desp.set_data = NULL;
	mag_desp.sensor_type = SENSOR_TYPE_MAGNETIC_FIELD;
	mag_desp.version = 0x0001;

	ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_MAGNETIC_FIELD, 1);
	if(ret < 0) {
		printf("ACC_Init sensor_subsys_algorithm_register_data_buffer error!!!\n\r");
	}

	ret = sensor_subsys_algorithm_register_type(&mag_desp);
	if(ret < 0) {
		printf("ACC_Init sensor_subsys_algorithm_register_type error!!!\n\r");
	}
	return 0;
}

static int acc_operate(Sensor_Command command, void* buffer_in, int size_in, void* buffer_out, int size_out)
{
	return 0;
}


static int acc_run_algorithm(struct data_t *output)
{
	if(output->data == NULL)
		return 0;
	output->data->sensor_type = SENSOR_TYPE_ACCELEROMETER;
	output->data->accelerometer_t.x = (int)sensor_input[acc_count%1000][0]/10;
	output->data->accelerometer_t.y = (int)sensor_input[acc_count%1000][1]/10;
	output->data->accelerometer_t.z = (int)sensor_input[acc_count%1000][2]/10;
	output->data->accelerometer_t.status = 2;
	output->data->time_stamp = timestamp_get_ns()/1000000;

	acc_count++;
	printf("acc_run_algorithm(%d, %d, %d)\n\r", (int)sensor_input[acc_count%1000][0]/10, (int)sensor_input[acc_count%1000][1]/10, (int)sensor_input[acc_count%1000][2]/10);
	return 0;
}

int FakeACC_Init(void)
{
	int ret;
	struct SensorDescriptor_t acc_desp;

	acc_desp.hw.max_sampling_rate = 5;//ms
	acc_desp.hw.support_HW_FIFO = 0;
	acc_desp.input_list = NULL;	
	acc_desp.operate = acc_operate;
	acc_desp.run_algorithm = acc_run_algorithm;
	acc_desp.set_data = NULL;
	acc_desp.sensor_type = SENSOR_TYPE_ACCELEROMETER;
	acc_desp.version = 0x0001;

	ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_ACCELEROMETER, 1);
	if(ret < 0) {
		printf("ACC_Init sensor_subsys_algorithm_register_data_buffer error!!!\n\r");
	}

	ret = sensor_subsys_algorithm_register_type(&acc_desp);
	if(ret < 0) {
		printf("ACC_Init sensor_subsys_algorithm_register_type error!!!\n\r");
	}
	return 0;
}


int pedo_set_data(const struct data_t *input_list, void *reserve)
{
	printf("sensor(%d)pedo_set_data\n\r", input_list->data->sensor_type);

	return 0;
}

static int pedo_operate(Sensor_Command command, void* buffer_in, int size_in, void* buffer_out, int size_out)
{
	return 0;
}


static int pedo_run_algorithm(struct data_t *output)
{
	printf("pedo_run_algorithm\n\r");
	if(output->data == NULL)
		return 0;

	output->data->sensor_type = SENSOR_TYPE_PEDOMETER;
	return 0;
}

int FakePedometerInit(void)
{
	int ret;
	struct SensorDescriptor_t pedo_desp;
	
	printf("PedometerInit\n\r");

	struct input_list_t acc;
	acc.input_type = SENSOR_TYPE_ACCELEROMETER;
	acc.next_input = NULL;
	acc.sampling_delay = 20;//ms
	
	pedo_desp.hw.max_sampling_rate = -1;//ms
	pedo_desp.hw.support_HW_FIFO = 0;
	pedo_desp.input_list = &acc;	
	pedo_desp.operate = pedo_operate;
	pedo_desp.run_algorithm = pedo_run_algorithm;
	pedo_desp.set_data = pedo_set_data;
	pedo_desp.sensor_type = SENSOR_TYPE_PEDOMETER;
	pedo_desp.version = 0x0001;

	ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_PEDOMETER, 1);
	if(ret < 0) {
		printf("PedometerInit sensor_subsys_algorithm_register_data_buffer error!!!\n\r");
	}

	ret = sensor_subsys_algorithm_register_type(&pedo_desp);
	if(ret < 0) {
		printf("PedometerInit sensor_subsys_algorithm_register_type error!!!\n\r");
	}
	return 0;

}

int pdr_set_data(const struct data_t *input_list, void *reserve)
{
	printf("sensor(%d)pdr_set_data\n\r", input_list->data->sensor_type);

	return 0;
}

static int pdr_operate(Sensor_Command command, void* buffer_in, int size_in, void* buffer_out, int size_out)
{
	return 0;
}


static int pdr_run_algorithm(struct data_t *output)
{
	printf("pdr_run_algorithm\n\r");
	if(output->data == NULL)
		return 0;

	output->data->sensor_type = SENSOR_TYPE_PDR;
	return 0;
}

int FakePDRInit(void)
{
	int ret;
	struct SensorDescriptor_t pdr_desp;

	struct input_list_t mag;
	mag.input_type = SENSOR_TYPE_MAGNETIC_FIELD;
	mag.next_input = NULL;
	mag.sampling_delay = 50;//ms


	struct input_list_t pedo;
	pedo.input_type = SENSOR_TYPE_PEDOMETER;
	pedo.next_input = &mag;
	pedo.sampling_delay = 200;//ms
	
	pdr_desp.hw.max_sampling_rate = -1;//ms
	pdr_desp.hw.support_HW_FIFO = 0;
	pdr_desp.input_list = &pedo;	
	pdr_desp.operate = pdr_operate;
	pdr_desp.run_algorithm = pdr_run_algorithm;
	pdr_desp.set_data = pdr_set_data;
	pdr_desp.sensor_type = SENSOR_TYPE_PDR;
	pdr_desp.version = 0x0001;

	ret = sensor_subsys_algorithm_register_data_buffer(SENSOR_TYPE_PDR, 1);
	if(ret < 0) {
		printf("PDRInit sensor_subsys_algorithm_register_data_buffer error!!!\n\r");
	}

	ret = sensor_subsys_algorithm_register_type(&pdr_desp);
	if(ret < 0) {
		printf("PDRInit sensor_subsys_algorithm_register_type error!!!\n\r");
	}
	return 0;
}

