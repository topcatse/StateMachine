//=============================================- -*- C++ -*- ===================
//
// File Name     StateMachine.h
// Author        Tommy Carlsson (topcatse)
//
// This file contains the interfaces of StateMachine and SmEvent.
//==============================================================================
#pragma once
#if !defined ( BASE_STATE_MACHINE_H_ )
#define BASE_STATE_MACHINE_H_
//==============================================================================

// ANSI/STL
#include <stack>

//==============================================================================
namespace Base {
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


#if !defined ( SM_LACKS_INCLASS_MEMBER_INITIALIZATION )
#  define SM_STATIC_CONSTANT( type, assignment ) static const type assignment
#else
#  define SM_STATIC_CONSTANT( type, assignment ) enum { assignment }
# endif

//==============================================================================

/**
 * The base class of any StateMachine event.
 * This class is targeted to be used togheter with
 * the StateMachine class below as it is a friend.
 */
template < typename T = int >
class SmEvent
{
public:
    typedef unsigned short Signal;
    
    /// Constructor.
    SmEvent( Signal const& s, T* p = 0 ) : ptr_( p ), signal_( s ) {}

    /// Destructor.
    ~SmEvent() {}

    /// User shall start numering his signals with USER_START.
    SM_STATIC_CONSTANT( Signal, USER_START = 3 );

    Signal const& signal() const { return signal_; }
    void signal(Signal const& signal) { signal_ = signal; }

    T& operator*() const { return* ptr_; }

    T* operator->() const { return ptr_; }

    T* get() const { return ptr_; }
    
private:
    T*     ptr_;
    Signal signal_;    
};

//==============================================================================

// Forward declaration
template < class OWNER, class T = int > class StatePtr; 

/// Function pointer wrapper (cf. Sutter, More Exceptional C++)
template < class OWNER, class T >
class StatePtr
{
    typedef SmEvent< T > UserEvent;
    
public:
    /// User shall start numering his signals with USER_START.
    SM_STATIC_CONSTANT( Signal, USER_START = 3 );

    /// State definition.
    typedef StatePtr ( OWNER::* State )( SmEvent< T > const* );
 
    /// Constructor.
    explicit StatePtr( State state = 0 ) : owner_( 0 )
                                         , state_( state ) 
    {}
    
    /// This class is not to be inherited from even though the destructor
    /// is defined in the public area.
    ~StatePtr() {}

    /// Initializer. Object takes ownership of load.
    void init( OWNER* owner, State state ) 
    {
        owner_ = owner;
        state_ = state;
    }
        
    /// Copy constructor.
    StatePtr( StatePtr const& other ) : owner_( other.owner_ )
                                      , state_( other.state_ ) 
    {}		

    /// Utility swap method.
    void swap( StatePtr& other )
    {
        std::swap( owner_, other.owner_ );
        std::swap( state_, other.state_ );
    }
        
    /// Assignment operator.
    StatePtr& operator=( StatePtr const& other )
    {
        StatePtr tmp( other );
        swap( tmp );
        return *this;
    }

    /// Conversion operator to State.
    operator State() { return state_; }

    /// Equality operator.
    bool operator==( StatePtr const& rhs )
    {
        return state_ == rhs.state_;
    }

    /// Inequality operator.        
    bool operator!=( StatePtr const& rhs )
    {
        return state_ != rhs.state_;
    }

    /// Invoke transition in owner.
    StatePtr operator()( SmEvent< T > const* e )
    {
        return (owner_->*state_)( e );
    }

private:
    State               state_;
    OWNER*              owner_;
};

//==============================================================================

/**
 * A Hierarchical State Machine framework.
 */
template < class OWNER, class T = int >
class StateMachine
{
    typedef SmEvent< T >         UserEvent;
    typedef StatePtr< OWNER, T > UserState;    
    
public:
    /// Check if user is in given state.
    /// 2 if user is in given state,
    /// 1 if in substate,
    /// 0 otherwise.
    int isInState( UserState const& state );

    /// Current state accessor.
    UserState current() const;

protected:
    /// Initialze and execute initial transistion.
    void open( OWNER*           owner,
               UserState const& initial,
               UserEvent const* e = 0 );

    /// Dispatch event. Takes take ownership of event.
    bool dispatch( UserEvent* e );
   
    /// Call when there is a default initialization state.
    void initializer( UserState const& s ) { current( s ); }
    
    /// Call when a transition shall occur.
    void transition( UserState const& s ) { target( s ); }

    /// To be returned when there is no parent state.
    UserState topState( UserEvent const* e = 0 ) 
    { 
        static const UserState us( &OWNER::topState );
        return us;
    }

    /// Shall be called when an event has been accepted.
    UserState handled( UserEvent const* e = 0 )
    { 
        static const UserState us( &OWNER::handled );
        return us;
    }

    /// Dtor. This class is not to be derived from.
    ~StateMachine();

    /// Use to specify predefined state actions when a state is initialized, 
    /// is entered or leaves a state. INQUIRE is reserved for internal use.
    enum StandardSignals { INQUIRE = -1, INIT, ENTRY, EXIT };
   
private: 
    /// Current state mutator.
    void current( UserState const& current );

    /// Pitcher state accessor.
    UserState pitcher() const;

    /// Pitcher state mutator.
    void pitcher( UserState const& pitcher );
   
    /// Target state mutator.
    void target( UserState const& state );
   
    /// Target state accessor.
    UserState target() const;

    /// Invoke init event in given state and init/entry events in all
    /// possibly subsequent states. 
    /// If init returns false, a self transition is invoked.
    void init( UserState const& state );
   
    /// Trace down to state that possibly handles the given signal.
    /// Releases the event.
    bool findPitcher( UserEvent* e );

    /// Invoke exit on all states from current state down to
    /// pitcher state.
    void exitDownToPitcher();
   
    /// Used to store state hierarchy in transition.
    typedef std::deque< UserState > Path;

    /// Invoke entry event in given path.
    /// path: all states between pitcher and target..
    /// path: the states in the path might be updated but
    /// not the path itself.
    void retraceEntryPath( Path& path );

    /// Helper events used by dispatch().
    static SmEvent< T > const inquireEvent_;
    static SmEvent< T > const initEvent_;
    static SmEvent< T > const entryEvent_;
    static SmEvent< T > const exitEvent_;

    /// The current state.
    UserState current_;

    /// Pitcher state during a transition.
    UserState pitcher_;
    
    /// Target state during a transition.
    UserState target_;

    /// The statemachine owner.
    OWNER* owner_;
};

//------------------------------------------------------------------------------
} // namespace Base {
//------------------------------------------------------------------------------

/// include the "implementation"
#include "StateMachine.inl"

//==============================================================================
#endif /* BASE_STATE_MACHINE_H_ */
//==============================================================================
