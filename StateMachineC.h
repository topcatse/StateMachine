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

#include "Deque.h"

//==============================================================================

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
#   define SM_TRACE( X ) LOG( DEBUG ) << X
#endif /* SM_TRACE */

//==============================================================================

typedef unsigned short Signal;
typedef void*          OWNER;
struct State;

//==============================================================================

/// User shall start numbering his signals with USER_START.
typedef enum { SM_USER_START = 3 } StartSignal;
 
typedef struct State* (*StateFcn)(OWNER, Signal);

struct State
{ 
	StateFcn stateFcn_;
    OWNER    owner_; // Cached, not owned
};

typedef struct State* State;

/// Constructor
void State_ctor( State self, StateFcn stateFcn );

/// Initializer. Object takes ownership of load.
void State_init( State self, OWNER owner, StateFcn stateFcn );
	
/// Copy constructor	
void State_copyCtor( State self, State const other );

/// Utility swap method.
void State_swap( State self, State other );
	
/// Assignment operator.
//State* State_assign( State self, State const other );

/// Conversion operator to StateFcn.
StateFcn State_stateFcn( State self );

/// Equality operator.
int State_isEqual( State self, State const rhs );

/// Inequality operator.
int State_isNotEqual( State self, State const rhs );

/// Invoke transition in owner.
State State_invoke( State self, Signal const e );

//==============================================================================

/**
 * A Hierarchical StateFcn Machine framework.
 */
struct StateMachine_t
{
    /// The current stateFcn.
    State current_;

    /// Pitcher stateFcn during a transition.
    State pitcher_;
    
    /// Target stateFcn during a transition.
    State target_;

    /// The stateFcn machine owner.
    OWNER owner_;
};

typedef struct StateMachine_t* StateMachine;

/// Use to specify predefined stateFcn actions when a stateFcn is initialized, 
/// is entered or leaves a stateFcn. INQUIRE is reserved for internal use.
enum StandardSignals { SM_DUMMY = -2, SM_INQUIRE = -1, SM_INIT, SM_ENTRY, SM_EXIT };

/// Initialize and execute initial transition.
void StateMachine_open(StateMachine self,
					   OWNER        owner,
					   State const  initial,
					   Signal       e);

/// Check if user is in given stateFcn.
/// 2 if user is in given stateFcn,
/// 1 if in sub stateFcn,
/// 0 otherwise.
int StateMachine_isInState(StateMachine self, State const stateFcn);

/// Current stateFcn accessor.
State StateMachine_current(StateMachine self);

/// Dispatch event.
int StateMachine_dispatch(StateMachine self, Signal e);

/// Call when there is a default initialization stateFcn.
void StateMachine_initializer(StateMachine self, State const s);

/// Call when a transition shall occur.
void StateMachine_transition(StateMachine self, State const s);

/// To be returned when there is no parent stateFcn.
State StateMachine_topState(OWNER owner, Signal e);

/// Shall be called when an event has been accepted.
State StateMachine_handled(OWNER owner, Signal e);

//==============================================================================
#endif /* BASE_STATE_MACHINE_H_ */
//==============================================================================
