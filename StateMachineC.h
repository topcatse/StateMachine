//=============================================- -*- C -*- ===================
//
// File Name     StateMachine.h
// Author        Tommy Carlsson (topcatse)
//
// This file contains the interfaces of StateMachine and SmEvent.
//==============================================================================
#if !defined ( BASE_STATE_MACHINE_H_ )
#define BASE_STATE_MACHINE_H_
//==============================================================================

#include <stdbool.h>

// This section should be defined elsewhere.

/// Define this if tracing SHALL occur.
//#define SM_NTRACE 0

/// Turns off the tracing feature default.
#if !defined ( SM_NTRACE )
#   define SM_NTRACE 1
#endif /* SM_NTRACE */
   
/// Trace macro definition.   
#if ( defined( SM_NTRACE ) && SM_NTRACE == 1 )
#   define SM_TRACE( X )
#else
#   include <stdio.h>
#   define SM_TRACE( X ) printf("%s\r\n", X);
#endif /* SM_TRACE */

#define STATEMACHINE_MAX_DEPTH 20

//==============================================================================

typedef unsigned short Signal;
typedef void*          OWNER;
struct State;

//==============================================================================

/// User shall start numbering his signals with USER_START.
typedef enum { SM_USER_START = 3 } StartSignal;
 
typedef struct State* (*StateFcn)( OWNER, Signal );

struct State
{ 
	StateFcn stateFcn_;
    OWNER    owner_; // Cached, not owned
};

typedef struct State* State;

/// Constructor
State State_ctor( OWNER owner, StateFcn stateFcn );

/// Initializer. Object takes ownership of load.
void State_init( State self, OWNER owner, StateFcn stateFcn );

/// Conversion operator to StateFcn.
StateFcn State_stateFcn( State self );

/// Equality operator.
bool State_isEqual( State self, State const rhs );

/// Inequality operator.
bool State_isNotEqual( State self, State const rhs );

/// Invoke transition in owner.
State State_invoke( State self, Signal e );

//==============================================================================

/**
 * A Hierarchical State Machine framework.
 */
struct StateMachine_t
{
    /// The current state.
    State current_;

    /// Pitcher state during a transition.
    State pitcher_;
    
    /// Target state during a transition.
    State target_;

    /// The state machine owner.
    OWNER owner_;
};

typedef struct StateMachine_t* StateMachine;

/// Use to specify predefined state actions when a state is initialized,
/// is entered or leaves a state. INQUIRE is reserved for internal use.
enum StandardSignals { SM_DUMMY = -2, SM_INQUIRE = -1, SM_INIT, SM_ENTRY, SM_EXIT };

/// State machine constructor.
StateMachine StateMachine_ctor();

/// State machine destructor.
void StateMachine_dtor(StateMachine self);

/// Initialize and execute initial transition.
void StateMachine_open(StateMachine self,
					   OWNER        owner,
					   State const  initial);

/// Check if user is in given state.
/// 2 if user is in given state,
/// 1 if in sub state,
/// 0 otherwise.
int StateMachine_isInState(StateMachine self, State const state);

/// Current state accessor.
State StateMachine_current(StateMachine self);

/// Dispatch event.
bool StateMachine_dispatch(StateMachine self, Signal e);

/// Call when there is a default initialization state.
void StateMachine_initializer(StateMachine self, State const s);

/// Call when a transition shall occur.
void StateMachine_transition(StateMachine self, State const s);

/// To be returned when there is no parent state.
State StateMachine_topState(OWNER owner, Signal e);

/// Shall be called when an event has been accepted.
State StateMachine_handled(OWNER owner, Signal e);

//==============================================================================
#endif /* BASE_STATE_MACHINE_H_ */
//==============================================================================
