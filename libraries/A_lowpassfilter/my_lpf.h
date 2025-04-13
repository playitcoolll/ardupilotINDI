#pragma once
#include <AP_Common/AP_Common.h>
#include <AP_Param/AP_Param.h>
#include <stdlib.h>
#include <cmath>


class lpf_my {
public:
    lpf_my();

    // 这两个是低通滤波器 对减小输入来说并不好用，飞机还是会对快速的拨杆有响应导致发散
    void my_LowPassFilter_init(float cutoff_freq_in, float sample_rate_in); 
    float my_filter(float input);
    float cutoff_freq;
    float sample_rate;
    float alpha;
    float previous_output;
    float output;


    //指数平滑法
    void ExponentialSmoothing_init(float alpha_in); 
    float apply(float new_value);
    float alpha_ExponentialSmoothing;             // 平滑系数，0.0 < alpha < 1.0
    float previous_value;





    static lpf_my *get_singleton() {
         return _singleton; 
    }

    static lpf_my *_singleton;
};

namespace AP {
    lpf_my &lpf_My();
};