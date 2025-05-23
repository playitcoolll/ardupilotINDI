#include "mode.h"
#include "Plane.h"
// #include <A_lowpassfilter/my_lpf.h>

void ModeFBWA::update()
{
    // set nav_roll and nav_pitch using sticks
    plane.nav_roll_cd  = plane.channel_roll->norm_input() * plane.roll_limit_cd;//此处输出为0
    plane.update_load_factor();
    float pitch_input = plane.channel_pitch->norm_input();//此处输出为0

    if (pitch_input > 0) {
        plane.nav_pitch_cd = pitch_input * plane.aparm.pitch_limit_max*100;
    } else {
        plane.nav_pitch_cd = -(pitch_input * plane.pitch_limit_min*100);
    }//此处输出为0

    // plane.adjust_nav_pitch_throttle();//防止飞机在低油门（低油门位置可能出现在降落阶段或滑翔时）时发生失速
    plane.nav_pitch_cd = constrain_int32(plane.nav_pitch_cd, plane.pitch_limit_min*100, plane.aparm.pitch_limit_max.get()*100);
    //此处输出为-200 说明油门这个地方有影响，所以注释掉了，因为对增量动态逆来说，如果低速时候机头没有低到-2度就会一直积分
    // hal.console -> printf("%d\n",plane.nav_pitch_cd);


    // 创建低通滤波器实例
    lpf_my &_lpf_my = AP::lpf_My();
    // plane.nav_roll_cd = _lpf_my.apply(plane.nav_roll_cd); // 不太对劲，滚转两侧的正负不一样导致的???
    plane.nav_pitch_cd = _lpf_my.apply(plane.nav_pitch_cd);


    if (plane.fly_inverted()) {
        plane.nav_pitch_cd = -plane.nav_pitch_cd;
    }
    if (plane.failsafe.rc_failsafe && plane.g.fs_action_short == FS_ACTION_SHORT_FBWA) {
        // FBWA failsafe glide
        plane.nav_roll_cd = 0;
        plane.nav_pitch_cd = 0;
        SRV_Channels::set_output_limit(SRV_Channel::k_throttle, SRV_Channel::Limit::MIN);
    }
    RC_Channel *chan = rc().find_channel_for_option(RC_Channel::AUX_FUNC::FBWA_TAILDRAGGER);
    if (chan != nullptr) {
        // check for the user enabling FBWA taildrag takeoff mode
        bool tdrag_mode = chan->get_aux_switch_pos() == RC_Channel::AuxSwitchPos::HIGH;
        if (tdrag_mode && !plane.auto_state.fbwa_tdrag_takeoff_mode) {
            if (plane.auto_state.highest_airspeed < plane.g.takeoff_tdrag_speed1) {
                plane.auto_state.fbwa_tdrag_takeoff_mode = true;
                plane.gcs().send_text(MAV_SEVERITY_WARNING, "FBWA tdrag mode");
            }
        }
    }
}

void ModeFBWA::run()
{
    // Run base class function and then output throttle
    Mode::run();

    output_pilot_throttle();
}
