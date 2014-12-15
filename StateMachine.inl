//=============================================- -*- C++ -*- ===================
//
// File Name     StateMachine.inl
// Author        Tommy Carlsson
//
// This file contains the implementation of StateMachine.
//
//==============================================================================

// ANSI/STL
#include <algorithm>
#include <cassert>

//==============================================================================
namespace Base {
//==============================================================================

template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::inquireEvent_ = SmEvent< T >( INQUIRE );

template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::initEvent_    = SmEvent< T >( INIT );

template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::entryEvent_   = SmEvent< T >( ENTRY );

template< class OWNER, class T >
SmEvent< T >
const StateMachine< OWNER, T >::exitEvent_    = SmEvent< T >( EXIT );

//------------------------------------------------------------------------------

template< class OWNER, class T >
StateMachine< OWNER, T >::~StateMachine()
{
   SM_TRACE( "StateMachine::~StateMachine" );
   // No op.
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::open( OWNER*                      owner,
                                StatePtr< OWNER, T > const& initial,
                                UserEvent const*            e )
{
   SM_TRACE( "StateMachine< OWNER, T >::open" );

   owner_   = owner;
   pitcher_ = owner->topState();
   current_ = owner->topState();
   target_  = initial;
   
   target()(&entryEvent_);
   init( target() );
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
int
StateMachine< OWNER, T >::isInState( StatePtr< OWNER, T > const& state )
{
   SM_TRACE( "StateMachine< OWNER, T >::isInState" );

   if ( current() == state )
   {
      return 2;
   }

   // and all states down to (not including) top.

   StatePtr< OWNER, T > next( current()( &inquireEvent_ ) );

   while ( next != owner_->topState() )
   {
      if ( next == state )
      {
         return 1;
      }
      next = next( &inquireEvent_ );
   }

   return 0;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
StatePtr< OWNER, T >
StateMachine< OWNER, T >::current() const
{
   SM_TRACE( "StateMachine< OWNER, T >::current" );
   return current_;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::current( StatePtr< OWNER, T > const& current )
{
   SM_TRACE( "StateMachine< OWNER, T >::current( State current )" );
   current_ = current;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
StatePtr< OWNER, T >
StateMachine< OWNER, T >::pitcher() const
{
   SM_TRACE( "StateMachine< OWNER, T >::pitcher" );
   return pitcher_;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::pitcher( StatePtr< OWNER, T > const& pitcher )
{
   SM_TRACE( "StateMachine< OWNER, T >::pitcher( State pitcher )" );
   pitcher_ = pitcher;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::target( StatePtr< OWNER, T > const& state )
{
   SM_TRACE( "StateMachine< OWNER, T >::target( State state )" );
   target_ = state;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
StatePtr< OWNER, T >
StateMachine< OWNER, T >::target() const
{
   SM_TRACE( "StateMachine< OWNER, T >::target" );
   return target_;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
bool
StateMachine< OWNER, T >::dispatch( UserEvent* e )
{
   SM_TRACE( "StateMachine< OWNER, T >::dispatch" );

   assert( e && "Bad event to StateMachine::dispatch" );

   using namespace std;

   // Used to elaborate internal transition.
   target( owner_->topState() );

   if ( !findPitcher( e ) )
   {
      // UserEvent is not handled.
      SM_TRACE( "StateMachine no pitcher" );
      return false;
   }

   // ( h) Internal transition.
   // Side-effect: Call to findPitcher() above, which possibly,
   // sets a new target. If target is unchanged it is considered as an 
   // internal transition so step out. 
   if ( target() == owner_->topState() )
   {
       SM_TRACE( "StateMachine handled case (h)" );
       return true;
   }

   exitDownToPitcher();

   // (a) Handle transition to self.
   if ( pitcher() == target() )
   {
      SM_TRACE( "StateMachine handled case (a)" );
      pitcher()( &exitEvent_ );      
      target()( &entryEvent_ );
      init( target() );
      return true;
   }   

   // (b) Handle pitcher == targets' parent.
   StatePtr< OWNER, T > targetParent  = target()( &inquireEvent_ );
   if ( pitcher() == targetParent )
   {
      SM_TRACE( "StateMachine handled case (b)" );
      target()( &entryEvent_ );
      init( target() );
      return true;
   }
   
   // (c) Handle pitcher's parent == targets' parent.
   StatePtr< OWNER, T > pitcherParent = pitcher()( &inquireEvent_ );
   if ( pitcherParent == targetParent )
   {
      SM_TRACE( "StateMachine handled case (c)" );
      pitcher()( &exitEvent_ );      
      target()( &entryEvent_ );
      init( target() );
      return true;
   }

   // (d) Handle pitcher's parent == target.
   if ( pitcherParent == target() )
   {
      SM_TRACE( "StateMachine handled case (d)" );
      pitcher()( &exitEvent_ );      
      init( target() );
      return true;
   }

   // The target state hierarchy needs to be recorded. 
   Path  trace;
   trace.push_front( target() );
   trace.push_front( targetParent );

   // (e) Handle pitcher == target's parent parent ... hierarchy.
   StatePtr< OWNER, T > next( targetParent( &inquireEvent_ ) );
   while ( next != owner_->topState() )
   {
      if ( next == pitcher() )
      {
         SM_TRACE( "StateMachine handled case (e)" );
         retraceEntryPath( trace );
         init( target() );
         return true;
      }
      trace.push_front( next );
      next = next( &inquireEvent_ );
   }
   trace.push_front( owner_->topState() );

   // The remaining cases impose EXIT of pitcher.
   pitcher()( &exitEvent_ );

   // (f) Handle pitcher's parent == target's parent parent ... hierarchy.
   typename Path::iterator pos;
   if ( ( pos = find( trace.begin(),
                      trace.end(),
                      pitcherParent ) ) != trace.end() )
   {
      // Found Least Base Ancestor @ pos. 
      // Erase it and its ancestors because ENTRY on these is not correct.
      SM_TRACE( "StateMachine handled case (f)" );
      trace.erase( trace.begin(), ++pos );
      retraceEntryPath( trace );
      init( target() );
      return true;
   }

   // (g) Handle pitcher's parent parent ... hierarchy for each target.
   bool search(true);
   next = pitcherParent;
   while ( search )
   {
      if ( ( pos = find( trace.begin(),
                         trace.end(),
                         next ) ) != trace.end() )
      {
         // Found Least Base Ancestor @ pos.
         // Erase it and its ancestors because ENTRY on these
         // is not correct.
         SM_TRACE( "StateMachine handled case (g)" );
         trace.erase( trace.begin(), ++pos );
         retraceEntryPath( trace );
         init( target() );
         return true;
      }
      if ( next != owner_->topState() )
      {
         next( &exitEvent_ );
         next = next( &inquireEvent_ );
      }
      else
      {
         search = false;
      }
   }

   assert( false && "Impossible StateMachine transition case" );
   return false;
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
bool
StateMachine< OWNER, T >::findPitcher( UserEvent* e )
{
   SM_TRACE( "StateMachine< OWNER, T >::findPitcher" );

   pitcher( current() );

   StatePtr< OWNER, T > next;

   while ( ( next = pitcher()( e ) ) != owner_->handled() && 
           pitcher() != owner_->topState() )
   {
      pitcher( next );
   }

   return ( pitcher() != owner_->topState() );
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::exitDownToPitcher()
{
   SM_TRACE( "StateMachine< OWNER, T >::exitDownToPitcher" );
   assert( pitcher() != owner_->topState() && "exitDownToPitcher" );

   StatePtr< OWNER, T > next( current() );

   while ( next != pitcher() )
   {
      assert( next );

      StatePtr< OWNER, T > tmp = next( &exitEvent_ );

      if ( tmp == owner_->handled() )
      {
         // EXIT handled, elicit parent.
         next = next( &inquireEvent_ );
      }
      else
      {
         // EXIT not handled (tmp = parent).
         next = tmp;
      }
   }
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::retraceEntryPath( Path& trace )
{
   SM_TRACE( "StateMachine< OWNER, T >::retraceEntryPath" );

   typename Path::const_iterator iter;
   typename Path::const_iterator end( trace.end() );

   for ( iter = trace.begin(); iter != end; ++iter )
   {
       StatePtr< OWNER, T > sw = *iter;
       sw( &entryEvent_ );
   }
}

//------------------------------------------------------------------------------

template< class OWNER, class T >
void
StateMachine< OWNER, T >::init( StatePtr< OWNER, T > const& state )
{
   SM_TRACE( "StateMachine< OWNER, T >::init" );

   current( state );

   StatePtr< OWNER, T > next( state );

   while ( next( &initEvent_ ) == owner_->handled() )
   {
      // INIT was handled so current has been modified (by initEvent_ call).
      next = current();
      next( &entryEvent_ );
   }
}

//==============================================================================
} // namespace Base {
//==============================================================================