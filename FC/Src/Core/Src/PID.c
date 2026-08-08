#include "PID.h"
#include "NRF24L01.h"

void PID_Circulate(PID_Struct *pid,float dt)
{
	pid->err = pid->desire - pid->measure;
	pid->integral += pid->err*dt;
	if (pid->integral > 500)
	{
		pid->integral = 500;
	}
	else if (pid->integral < -500)
	{
		pid->integral = -500;
	}
	if(pid->last_err == 0)
		pid->last_err = pid->err;
	pid ->der = pid->err - pid->last_err;
	pid->output = pid->kp * pid->err +(pid->ki*pid->integral) + (pid->kd * pid->der/dt);
	pid->last_err = pid->err;
}

void PID_Circulate_Chain(PID_Struct *out_pid,PID_Struct *in_pid,float dt)
{
	PID_Circulate(out_pid,dt);
	in_pid->desire = out_pid->output;
	PID_Circulate(in_pid,dt);
}

