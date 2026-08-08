#include "PID.h"
#include "NRF24L01.h"

void PID_Circulate(PID_Struct *pid,float dt)
{
	// uint32_t addr = (uint32_t)pid;

	// if (addr < 0x20000000UL || addr >= 0x20020000UL)
    // {
    //     __BKPT(0);
    //     while (1);
    // }

    // if (dt <= 0.000001f || dt > 0.1f)
    // {
    //     __BKPT(0);
    //     while (1);
    // }

	pid->err = pid->desire - pid->measure;
	pid->integral += pid->err;
	if(pid->last_err == 0)
		pid->last_err = pid->err;
	pid ->der = pid->err - pid->last_err;
	pid->output = pid->kp * pid->err +(pid->ki*pid->integral*dt) + (pid->kd * pid->der/dt);
	pid->last_err = pid->err;
}

void PID_Circulate_Chain(PID_Struct *out_pid,PID_Struct *in_pid,float dt)
{
	PID_Circulate(out_pid,dt);
	in_pid->desire = out_pid->output;
	PID_Circulate(in_pid,dt);
}

