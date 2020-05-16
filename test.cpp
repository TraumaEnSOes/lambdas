#include <algorithm>
#include <functional>
#include <iostream>

struct CStruct {
  void *userData;
  // Datos PRIVADOS
  void ( *onClose )( CStruct * );
};

extern "C" void doClose( CStruct *st, void (*cb)( struct CStruct * ) ) {
  st->onClose = cb;
}

extern "C" void reallyClose( CStruct *st ) {
  if( st->onClose ) { st->onClose( st ); }
}

namespace internal {

struct Wrapper {
  unsigned refs;
  std::function< void( CStruct * ) > callback;
};

} // namespace internal.

class Wrapper {
  union {
    CStruct *_data;
    internal::Wrapper **_wrapper;
  };

  static void Callback( CStruct *cst ) {
    auto &priv = **reinterpret_cast< internal::Wrapper ** >( cst );

    if( priv.callback ) { priv.callback( cst ); }
  }

protected:
  Wrapper( void *ptr ) : _data( static_cast< CStruct * >( ptr ) ) { }

  void setData( void *ptr ) {
    _data = static_cast< CStruct * >( ptr );
  }

public:
  Wrapper( ) : _data( nullptr ) { }

  void *target( ) { return _data; }
  const void *target( ) const { return _data; }
  const void *ctarget( ) const { return _data; }

  void close( ) { doClose( _data, nullptr ); }
  void close( std::function< void( Wrapper ) > cb ) {
    if( cb ) {
      auto lambda = [cb]( CStruct *cst ) -> void {
        cb( Wrapper( cst ) );
      };

      ( *_wrapper )->callback = lambda;
      doClose( _data, Callback );
    } else {
      doClose( _data, nullptr );
    }
  }
  template< typename CLASS > void close( CLASS *self, void ( CLASS::*cb )( Wrapper ) ) {
    auto lambda = [self,cb]( CStruct *cst ) -> void {
      ( self->*cb )( Wrapper( cst ) );
    };

    ( *_wrapper )->callback = lambda;

    doClose( _data, Callback );
  }
};

struct Data : public Wrapper {
  Data( ) : Wrapper( ) {
    char *tmp = new char[sizeof( CStruct ) + sizeof( internal::Wrapper )];

    std::fill_n( tmp, sizeof( CStruct ) + sizeof( internal::Wrapper ), 0 );
    reinterpret_cast< CStruct * >( tmp )->userData = tmp + sizeof( CStruct );
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

  reallyClose( reinterpret_cast< CStruct * >( data1.target( ) ) );
  reallyClose( reinterpret_cast< CStruct * >( data2.target( ) ) );
  reallyClose( reinterpret_cast< CStruct * >( data3.target( ) ) );

  return 0;
}
