#include "algo_adaptor.h"
#include "math_method.h"

void sensor_subsys_algorithm_resampling_type(resampling_t *resample)
{
    uint32_t scale_up_factor = 10;//10%
    uint32_t scale_prd = scale_up_factor * resample->input_sample_delay / 100;

    if (((resample->current_time_stamp - resample->last_time_stamp) >= (resample->input_sample_delay - scale_prd)) &&
            ((resample->current_time_stamp - resample->last_time_stamp) <= (resample->input_sample_delay + scale_prd))) {
        resample->input_count = 1;
    } else {
        resample->input_count = (resample->current_time_stamp - resample->last_time_stamp) / resample->input_sample_delay;
        if (resample->input_count > 10) {
            resample->input_count = 10;
        }
        if (resample->input_count < 1) {
            resample->input_count = 0;
        }
    }
}

int32_t check_if_same_input(const struct data_unit_t *data_start, uint32_t last_time_stamp, int32_t data_count)
{
    int32_t same_data = 0;
    if (data_count >= 1) {
        uint32_t data_idx = data_count - 1;
        uint32_t time_stamp_ms = ((data_start + data_idx)->time_stamp) / 1000000;

        if ((time_stamp_ms <= last_time_stamp + 10) && (time_stamp_ms > last_time_stamp - 10)) {
            same_data = 1;
        }
    }

    return same_data;
}
