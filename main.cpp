/// @file main.cpp
/// @brief Основной файл демона lcd_agent
///
/// @copyright Copyright 2019 InfoTeCS.

#include <iostream>
#include <cstdlib>
#include <csignal>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>

#include <mode_ctrl.h>

using namespace mode_ctrl;

namespace
{

volatile sig_atomic_t terminate = 0;
std::condition_variable stopProgram;

/// @brief Функция очистки ресурсов при завершении программы
void Cleanup()
{
}

/// @brief Функция обработки сигналов
/// @param[in] signo номер сигнала
void SigHandler( int signo )
{
     switch ( signo )
     {
          case SIGTERM:
          case SIGINT:
          {
               terminate = signo;
               stopProgram.notify_all();
               break;
          }
          default:
               break;
     }
}

/// @brief Функция регистрации обработчика сигналов
/// @param[in] signo номер сигнала
/// @return true в случае успеха, иначе false
bool RegisterSighandler( int signo )
{
     struct sigaction act;
     memset( &act, 0, sizeof( act ) );

     act.sa_handler = SigHandler;
     act.sa_flags = SA_RESETHAND | SA_NOCLDSTOP;

     sigfillset( &act.sa_mask );
     int result = sigaction( signo, &act, nullptr );
     return ( result >= 0 );
}

/// @brief Ожидание завершения работы службы
/// @param[in] terminate номер сигнала
void ProgramWait( volatile sig_atomic_t& terminate )
{
     while( !terminate )
     {
          std::mutex dummy;
          std::unique_lock<std::mutex> lock( dummy );
          stopProgram.wait( lock );
     }
}

} // anonymous namespace

int main( int argc, char* argv[] )
try
{
     std::ignore = argc;
     std::ignore = argv;
     ( void )argc;
     // Open log.
     atexit( Cleanup );

     if ( !RegisterSighandler( SIGTERM ) || !RegisterSighandler( SIGINT ) )
     {
          throw std::runtime_error( "Can't catch SIGTERM" );
     }

     RuleController ctrl;
     ProgramWait( terminate );
     ctrl.WaitThreads();
     return EXIT_SUCCESS;
}
catch( const std::exception &error )
{
     std::cerr << static_cast<const char*>( error.what() ) << std::endl;
     return EXIT_FAILURE;
}
catch( ... )
{
     std::cerr << "Unknown exception" << std::endl;
     return EXIT_FAILURE;
}
