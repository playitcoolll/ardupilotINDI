#pragma once

/// @file	AC_PID.h
/// @brief	Generic PID algorithm, with EEPROM-backed storage of constants.实现一个通用的 PID 控制器算法，具有基于 EEPROM 的常量存储功能（用于保存 PID 参数）

#include <AP_Common/AP_Common.h>//ArduPilot 通用模块的头文件，提供通用的数据结构和方法
#include <AP_Param/AP_Param.h>//用于管理参数的模块，支持通过 GCS（地面站）修改 PID 参数。
#include <stdlib.h>
#include <cmath>
#include <Filter/SlewLimiter.h>
#include <Filter/NotchFilter.h>
#include <Filter/AP_Filter.h>

#define AC_PID_TFILT_HZ_DEFAULT  0.0f   // default input filter frequency默认输入滤波器的截止频率，值为 0.0f，表示关闭滤波器
#define AC_PID_EFILT_HZ_DEFAULT  0.0f   // default input filter frequency默认误差滤波器的截止频率
#define AC_PID_DFILT_HZ_DEFAULT  20.0f   // default input filter frequency默认微分滤波器的截止频率，值为 20.0f，通常微分项需要滤波以减少噪声影响
#define AC_PID_RESET_TC          0.16f   // Time constant for integrator reset decay to zero积分项复位时间常数，值为 0.16f，用于控制积分器复位到零的速度

#include "AP_PIDInfo.h"

/// @class	AC_PID
/// @brief	Copter PID control class
class AC_PID {
public:

    struct Defaults {
        float p;
        float i;
        float d;
        float ff;
        float imax;
        float filt_T_hz;
        float filt_E_hz;
        float filt_D_hz;
        float srmax;
        float srtau;
        float dff;
    };

    // Constructor for PID
    AC_PID(float initial_p, float initial_i, float initial_d, float initial_ff, float initial_imax, float initial_filt_T_hz, float initial_filt_E_hz, float initial_filt_D_hz,
           float initial_srmax=0, float initial_srtau=1.0, float initial_dff=0);
    AC_PID(const AC_PID::Defaults &defaults) :
        AC_PID(
            defaults.p,
            defaults.i,
            defaults.d,
            defaults.ff,
            defaults.imax,
            defaults.filt_T_hz,
            defaults.filt_E_hz,
            defaults.filt_D_hz,
            defaults.srmax,
            defaults.srtau,
            defaults.dff
            )
        { }

    CLASS_NO_COPY(AC_PID);

    //  update_all - set target and measured inputs to PID controller and calculate outputs
    //  target and error are filtered
    //  the derivative is then calculated and filtered
    //  the integral is then updated based on the setting of the limit flag
    float update_all(float target, float measurement, float dt, bool limit = false, float boost = 1.0f);


    float update_all_my_x(float target, float measurement, float dt, float aspeed);
    float update_all_my_y(float target, float measurement, float dt, float aspeed);
    


    //  update_error - set error input to PID controller and calculate outputs
    //  target is set to zero and error is set and filtered
    //  the derivative then is calculated and filtered
    //  the integral is then updated based on the setting of the limit flag
    //  Target and Measured must be set manually for logging purposes.
    // todo: remove function when it is no longer used.
    float update_error(float error, float dt, bool limit = false);

    // get_pid - get results from pid controller
    float get_p() const;
    float get_i() const;
    float get_d() const;
    float get_ff() const;

    // reset_I - reset the integrator
    void reset_I();

    // reset_filter - input filter will be reset to the next value provided to set_input()
    void reset_filter() {
        _flags._reset_filter = true;
    }

    // load gain from eeprom
    void load_gains();

    // save gain to eeprom
    void save_gains();

    // get accessors
    const AP_Float &kP() const { return _kp; }
    AP_Float &kP() { return _kp; }
    AP_Float &kI() { return _ki; }
    AP_Float &kD() { return _kd; }
    AP_Float &kIMAX() { return _kimax; }
    AP_Float &kPDMAX() { return _kpdmax; }
    AP_Float &ff() { return _kff;}
    AP_Float &filt_T_hz() { return _filt_T_hz; }
    AP_Float &filt_E_hz() { return _filt_E_hz; }
    AP_Float &filt_D_hz() { return _filt_D_hz; }
    AP_Float &slew_limit() { return _slew_rate_max; }
    AP_Float &kDff() { return _kdff; }

    float imax() const { return _kimax.get(); }
    float pdmax() const { return _kpdmax.get(); }

    float get_filt_T_alpha(float dt) const;
    float get_filt_E_alpha(float dt) const;
    float get_filt_D_alpha(float dt) const;

    // set accessors
    void set_kP(const float v) { _kp.set(v); }
    void set_kI(const float v) { _ki.set(v); }
    void set_kD(const float v) { _kd.set(v); }
    void set_ff(const float v) { _kff.set(v); }
    void set_imax(const float v) { _kimax.set(fabsf(v)); }
    void set_pdmax(const float v) { _kpdmax.set(fabsf(v)); }
    void set_filt_T_hz(const float v);
    void set_filt_E_hz(const float v);
    void set_filt_D_hz(const float v);
    void set_slew_limit(const float v);
    void set_kDff(const float v) { _kdff.set(v); }

    // set the desired and actual rates (for logging purposes)
    void set_target_rate(float target) { _pid_info.target = target; }
    void set_actual_rate(float actual) { _pid_info.actual = actual; }

    // integrator setting functions
    void set_integrator(float i);
    void relax_integrator(float integrator, float dt, float time_constant);

    // set slew limiter scale factor
    void set_slew_limit_scale(int8_t scale) { _slew_limit_scale = scale; }

    // return current slew rate of slew limiter. Will return 0 if SMAX is zero
    float get_slew_rate(void) const { return _slew_limiter.get_slew_rate(); }

    const AP_PIDInfo& get_pid_info(void) const { return _pid_info; }

    void set_notch_sample_rate(float);

    // parameter var table
    static const struct AP_Param::GroupInfo var_info[];

protected:

    //  update_i - update the integral
    //  if the limit flag is set the integral is only allowed to shrink
    void update_i(float dt, bool limit);

    // parameters
    AP_Float _kp;
    AP_Float _ki;
    AP_Float _kd;
    AP_Float _kff;
    AP_Float _kimax;
    AP_Float _kpdmax;
    AP_Float _filt_T_hz;         // PID target filter frequency in Hz
    AP_Float _filt_E_hz;         // PID error filter frequency in Hz
    AP_Float _filt_D_hz;         // PID derivative filter frequency in Hz
    AP_Float _slew_rate_max;
    AP_Float _kdff;
#if AP_FILTER_ENABLED
    AP_Int8 _notch_T_filter;
    AP_Int8 _notch_E_filter;
#endif

    // the time constant tau is not currently configurable, but is set
    // as an AP_Float to make it easy to make it configurable for a
    // single user of AC_PID by adding the parameter in the param
    // table of the parent class. It is made public for this reason
    AP_Float _slew_rate_tau;

    SlewLimiter _slew_limiter{_slew_rate_max, _slew_rate_tau};

    // flags
    struct ac_pid_flags {
        bool _reset_filter :1; // true when input filter should be reset during next call to set_input
        bool _I_set :1; // true if if the I terms has been set externally including zeroing
    } _flags;

    // internal variables
    float _integrator;        // integrator value
    float _target;            // target value to enable filtering
    float _error;             // error value to enable filtering
    float _derivative;        // derivative value to enable filtering
    int8_t _slew_limit_scale;
    float _target_derivative; // target derivative value to enable dff
#if AP_FILTER_ENABLED
    NotchFilterFloat* _target_notch;
    NotchFilterFloat* _error_notch;
#endif

    AP_PIDInfo _pid_info;

private:
    const float default_kp;
    const float default_ki;
    const float default_kd;
    const float default_kff;
    const float default_kdff;
    const float default_kimax;
    const float default_filt_T_hz;
    const float default_filt_E_hz;
    const float default_filt_D_hz;
    const float default_slew_rate_max;
};
