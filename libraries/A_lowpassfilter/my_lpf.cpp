#include "my_lpf.h"
#include <cmath>  // 使用 math.h 中的函数
#include <AP_HAL/AP_HAL.h>
#include <chrono>
#include <AP_Scheduler/AP_Scheduler.h>


lpf_my::lpf_my()
{
    _singleton = this;
}

void lpf_my::my_LowPassFilter_init(float cutoff_freq_in, float sample_rate_in) {
    // 设置截止频率和采样频率
    this->cutoff_freq = cutoff_freq_in;
    this->sample_rate = sample_rate_in;
    
    // 计算滤波器系数
    float RC = 1.0 / (cutoff_freq * 2 * M_PI);
    this->alpha = sample_rate / (sample_rate + RC);
    
    // 初始化
    this->previous_output = 0.0f;
}

// 低通滤波器的应用方法
float lpf_my::my_filter(float input) {
    this->output = this->previous_output + this->alpha * (input - this->previous_output);
    this->previous_output = this->output;
    return this->output;
}

//指数平滑初始化
void lpf_my::ExponentialSmoothing_init(float alpha_in) {
    this->alpha_ExponentialSmoothing = alpha_in;
    // 初始化
    this->previous_value = 0.0f;
}

float lpf_my::apply(float new_value) {
    previous_value = alpha_ExponentialSmoothing * new_value + (1 - alpha_ExponentialSmoothing) * previous_value;
    return previous_value;
}

// singleton instance
lpf_my *lpf_my::_singleton;

namespace AP {

    lpf_my &lpf_My()
    {
        return *lpf_my::get_singleton();
    }

}