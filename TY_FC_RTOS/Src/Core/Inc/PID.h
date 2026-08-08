#ifndef __PID_H
#define __PID_H


#include "stdint.h"

typedef struct{
	float kp;
	float ki;
	float kd;
	float err;
	float last_err;
	float desire;
	float measure;
	float integral;
	float output;
	float der;
}PID_Struct;

void PID_Circulate(PID_Struct *pid,float dt);
void PID_Circulate_Chain(PID_Struct *out_pid,PID_Struct *in_pid,float dt);


#endif

