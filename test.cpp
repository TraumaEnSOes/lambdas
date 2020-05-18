#include <algorithm>
#include <functional>
#include <iostream>

struct LegacyStruct {
  void *userData;
  // Datos PRIVADOS
  void ( *onClose )( LegacyStruct * );
};

extern "C" void legacyAction( LegacyStruct *st, void (*cb)( struct LegacyStruct * ) ) {
  st->onClose = cb;
}

extern "C" void reallyClose( LegacyStruct *st ) {
  if( st->onClose ) { st->onClose( st ); }
}

namespace internal {

struct Wrapper {
  unsigned refs;
  std::function< void( LegacyStruct * ) > callback;
};

} // namespace internal.

class Wrapper {
  union {
    LegacyStruct *_data;
    internal::Wrapper **_wrapper;
  };

  static void Callback( LegacyStruct *cst ) {
    auto &priv = **reinterpret_cast< internal::Wrapper ** >( cst );

    if( priv.callback ) { priv.callback( cst ); }
  }

protected:
  Wrapper( void *ptr ) : _data( static_cast< LegacyStruct * >( ptr ) ) { }

  void setData( void *ptr ) {
    _data = static_cast< LegacyStruct * >( ptr );
  }

  template< typename T, typename C, typename RESULT = void (*)( C * ) >
  static RESULT makeWrapper( void (*globalCB)( C * ), std::function< void( T ) > &user, std::function< void( C * ) > *storage ) {
    // La std::function< > está vacía.
    if( !user ) { return nullptr; }

    // Comprobamos si es compatible con una función-no-miembro.
    {
      void (**compatible)( T ) = user.template target< void(*)( T ) >( );
      if( compatible ) {
        // return reinterpret_cast< RESULT >( *compatible );
        // return (RESULT)( *compatible );
        return reinterpret_cast< void (*)( C * ) >( *compatible );
      }
    }

    // Hay que usar una lambda para adaptar.
    auto lambda = [user]( C *ptr ) {
      user( T( ptr ) );
    };

    *storage = lambda;
    return globalCB;
  }

public:
  Wrapper( ) : _data( nullptr ) { }

  void *target( ) { return _data; }
  const void *target( ) const { return _data; }
  const void *ctarget( ) const { return _data; }

  void close( ) { legacyAction( _data, nullptr ); }
  
  void close( std::function< void( Wrapper ) > cb ) {
    legacyAction( _data, makeWrapper( Callback, cb, &( ( *_wrapper )->callback ) ) );
#if 0
    if( cb ) {
      // void (**compatible)( Wrapper ) = cb.target< void(*)( Wrapper ) >( );

      if( compatible ) {
        legacyAction( _data, reinterpret_cast< void(*)( LegacyStruct * ) >( *compatible ) );
      } else {
        auto lambda = [cb]( LegacyStruct *cst ) -> void {
          cb( Wrapper( cst ) );
        };

        ( *_wrapper )->callback = lambda;
        legacyAction( _data, Callback );
      }
    } else {
      legacyAction( _data, nullptr );
    }
#endif
  }

  template< typename CLASS > void close( CLASS *self, void ( CLASS::*cb )( Wrapper ) ) {
    auto lambda = [self,cb]( LegacyStruct *cst ) -> void {
      ( self->*cb )( Wrapper( cst ) );
    };

    ( *_wrapper )->callback = lambda;

    legacyAction( _data, Callback );
  }
};

struct Data : public Wrapper {
  Data( ) : Wrapper( ) {
    char *tmp = new char[sizeof( LegacyStruct ) + sizeof( internal::Wrapper )];

    std::fill_n( tmp, sizeof( LegacyStruct ) + sizeof( internal::Wrapper ), 0 );
    reinterpret_cast< LegacyStruct * >( tmp )->userData = tmp + sizeof( LegacyStruct );
    setData( tmp );
  }
};

struct Printer {
  const char *msg;

  Printer( const char *m ) : msg( m ) { }

  void print( Wrapper wr ) {
    std::cout << "Función Printer::print( '" << msg << "' ), Wrapper " << wr.target( ) << '\n';
  }
};

static void Close3( Wrapper sr ) {
  std::cout << "Función 'Close3( ), argumento " << sr.target( ) << '\n';
}

int main( ) {
  Printer p( "Prueba de función-miembro" );

  Data data1;
  Data data2;
  Data data3;

  std::cout << "data1: " << data1.target( ) << '\n';
  std::cout << "data2: " << data2.target( ) << '\n';
  std::cout << "data3: " << data3.target( ) << '\n';

  data1.close( []( Wrapper wr ) -> void { std::cout << "Lambda, argumento " << static_cast< void * >( wr.target( ) ) << '\n'; } );
  data2.close( &p, &Printer::print );
  data3.close( Close3 );

  reallyClose( reinterpret_cast< LegacyStruct * >( data1.target( ) ) );
  reallyClose( reinterpret_cast< LegacyStruct * >( data2.target( ) ) );
  reallyClose( reinterpret_cast< LegacyStruct * >( data3.target( ) ) );

  return 0;
}
