/// @file main.cpp
/// @brief Основной файл демона lcd_agent
///
/// @copyright Copyright 2019 InfoTeCS.

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <sys/types.h>
#include <unistd.h>

#include <mode_ctrl.h>

using namespace mode_ctrl;

namespace
{

/// @brief Номер сигнала, прервавшего работу службы
volatile sig_atomic_t terminate = 0;
/// @brief Условная переменная для пробуждения потока в случае сигнала
std::condition_variable stopProgram;

/// @brief Указатель на объект контроля режима доступа
/// Нужен именно указатель, если класс бросит исключение здесь, то тут его не обработать, поэтому откладываем создание
/// самого объекта
std::unique_ptr<ModeController> ctrl;

/// @brief Запись pid-файла процесса
void WritePid()
{
     pid_t myPid = getpid();
     std::ostringstream path;
     path << "/run/" << MODE_AGENT_NAME << ".pid";
     std::ofstream pidFile( path.str().c_str(), std::ios::trunc );
     if( pidFile )
     {
          pidFile << myPid << std::endl;;
     }
}

/// @brief Удаление pid-файла процесса
void RemovePid()
{
     std::ostringstream ossFile;
     ossFile << "/run/" << MODE_AGENT_NAME << ".pid";
     std::remove( ossFile.str().c_str() );
}

/// @brief Очистка ресурсов при завершении
void Cleanup()
{
     RemovePid();
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
          case SIGUSR2:
          {
               if( ctrl )
               {
                    ctrl->Update();
               }
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
     act.sa_flags = ( signo == SIGUSR2 ) ? SA_NOCLDSTOP : SA_RESETHAND | SA_NOCLDSTOP;

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

int main()
try
{
     if ( !RegisterSighandler( SIGTERM ) || !RegisterSighandler( SIGINT ) || !RegisterSighandler( SIGUSR2 ) )
     {
          throw std::runtime_error( "Can't catch SIGTERM" );
     }

     // А вот тут та самая отложенная процедура создания объекта
     ctrl.reset( new ModeController() );

     std::atexit( Cleanup );
     WritePid();

     // Служба в процессе работы, блокируется основной поток, отвиснет, когда нужно будет завершать работу
     ProgramWait( terminate );

     // Ждем потоки контроллера режима доступа
     // Форсировано ронять нельзя, иначе не успеет вернуть режим доступа в исходный
     ctrl->WaitThreads();

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
