#include "StateMachineC.h"

typedef struct
{
	State s0;
	State s1;
    StateMachine sm;
} Tester;

State Tester_s0(OWNER owner, Signal e)
{
    Tester* t = owner;
    
    switch (e) {
        case SM_INIT:
            StateMachine_initializer(t->sm, t->s1);
            break;
            
        default:
            break;
    }
	Tester* tester = owner;
	return tester->state2;
}

State Tester_s1(OWNER owner, Signal e)
{
	Tester* tester = owner;
	return tester->state1;
}

int main(int argc, char* argv[])
{
	Tester tester;

	tester.s0 = State_ctor(Tester_s0);
	tester.s1 = State_ctor(Tester_s1);
    tester.sm = StateMachine_ctor();
    
    StateMachine_open(tester.sm, &tester, tester.s0);
    
	return 0;
}