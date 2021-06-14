#include <iostream>
#include <cstring>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <functional>

#include <sys/stat.h>
#include <csignal>

namespace
{

/****************************************** КОНСТАНТЫ ************************************************/
/// @brief Имя службы
constexpr auto MODE_AGENT_NAME = "mode_agent";
/// @brief Полный путь конфига
constexpr auto CONFIG_NAME = "/etc/mode_ctrl_config";
/// @brief Полный путь скрипта управления службой
constexpr auto MGMT_SCRIPT = "/etc/init.d/mode_agent_mgmt";
/// @brief Имя службы
constexpr auto SERVICE_NAME = "mode_agent";
/// @brief Количество строк для отображения списка
constexpr auto LINES_NUMBER = 20;

/************************************ ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ******************************************/
/// @brief Вектор с запрещенными приложениями, вычитывается из конфига (далее список, но фактически это не список,
/// а вектор, так как с вектором удобнее работать)
std::vector<std::string> applications;

/// @brief Состояние службы
bool serviceStatus = false;

}

/// @brief Исключения для выхода, чтобы выйти из цикла UI
class ExitException : public std::exception
{
};

/// @brief Действия, доступные для UI
enum class Action : uint8_t
{
     list = 1,
     add,
     remove,
     update,
     toggle,
     exit,

     actionCnt
};
/// @brief Получение pid службы
/// @return pid или -1 в случае ошибки
uint32_t GetSrvcPid()
{
     std::string pidSrvc;
     std::ostringstream ossFile;
     ossFile << "/run/" << SERVICE_NAME << ".pid";
     std::ifstream pidFile( ossFile.str() );
     if( pidFile )
     {
          std::getline( pidFile, pidSrvc );
     }
     else
     {
          return -1;
     }

     if( pidSrvc.empty() )
     {
          return -1;
     }
     int pid = -1;
     if( 1 == sscanf( pidSrvc.c_str(), "%d", &pid ) )
     {
          return pid;
     }
     return -1;
}

/// @brief Проверка возможности записи в конфиг, иначе в конце работы не получится записать изменения
/// @return Если доступ есть - true
bool CheckPermission( const std::string& path )
{
     std::ofstream file( path, std::ios::app );
     if( !file )
     {
          std::cerr << "Ошибка при проверке конфигурационного файла. Проверьте доступ к файлу " << path
               << " и родительским директориям" << std::endl;
          return false;
     }
     return true;
}

/// @brief Чтение конфига
/// @param[in] path полный путь к конфигу
/// @return В случае успеха - true, иначе false
bool ReadConfig( const std::string& path )
{
     std::ifstream file( path );
     if( !file )
     {
          std::cerr << "Open " << path << " failed" << std::endl;
          return false;
     }
     std::string buffer;
     for( auto pos = 0; !file.eof(); ++pos )
     {
          getline( file, buffer );
          if( !buffer.empty() )
          {
               applications.push_back( buffer );
          }
     }
     return true;
}

/// @brief Запись конфига
/// @param[in] path полный путь к конфигу
void WriteAppToFile( const std::string& path )
{
     std::ofstream file( path, std::ios::trunc );
     if( !file )
     {
          std::cerr << "Open " << path << " failed" << std::endl;
          return;
     }
     for( const auto& it : applications )
     {
          file << it << std::endl;
     }
}

/// @brief Управление службой
/// @param[in] cmd команда (доступны - start/stop/status/update)
/// @return В случае успеха - true, иначе false
bool ServiceMgmt( const std::string& cmd )
{
     std::ostringstream ossCmd;
     ossCmd << MGMT_SCRIPT << " " << cmd << " > /dev/null";
     int res = system( ossCmd.str().c_str() );
     switch( res )
     {
          case 0:
          {
               return true;
          }
          case ( 127 << 8 ):
          {
               std::cerr << "Команда управления службой не найдена, необходимо переустановить ПО" << std::endl;
               break;
          }
          default:
          {
               std::cerr << "Ошибка управления службой" << std::endl;
               break;
          }
     }
     return false;
}

/// @brief Преобразование действия в строку с описанием
/// @param[in] act дейстиве
/// @param[in] state флаг состояние службы (true - вкл, false - выкл)
/// @return Строка с описанием, в случае ошибки пустая
std::string ActionToStr( const Action act, bool state )
{
     switch( static_cast<int>( act ) )
     {
          case static_cast<int>( Action::list ):       return std::string( "Отобразить спикок запрещенных программ" );
          case static_cast<int>( Action::add ):        return std::string( "Добавить программу в список запрещенных" );
          case static_cast<int>( Action::remove ):     return std::string( "Удалить программу из списка запрещенных" );
          case static_cast<int>( Action::update ):     return std::string( "Обновить конфигурационный файл" );
          case static_cast<int>( Action::toggle ):     return std::string( ( state ) ? "Выключить" : "Включить" );
          case static_cast<int>( Action::exit ):       return std::string( "Выйти" );
          default:                                     return std::string( "" );
     }
}

/// @brief Отображение списка запрещенных приложений
void ShowList()
{
     size_t index = 0;
     while( true )
     {
          if( applications.empty() )
          {
               std::cout << "Список пуст" << std::endl;
               return;
          }
          std::cout << "Список приложений (" << index + 1 << " - "
               << ( ( index + LINES_NUMBER >= applications.size() ) ? applications.size() : index + LINES_NUMBER )
               << ") из запрещенного списка:" << std::endl;
          for( size_t i = index; i < index + LINES_NUMBER; ++i )
          {
               if( applications.size() <= i )
               {
                    continue;
               }
               std::cout << i + 1 << ") " << applications[i] << std::endl;
          }
          std::cout << std::endl;
          if( LINES_NUMBER > applications.size() )
          {
               return;
          }

          std::stringstream ssMsg;
          if( applications.size() <= index + LINES_NUMBER )
          {
               ssMsg << "Введите 'p' для отображения предыдущих " << LINES_NUMBER << " приложений из списка"
                    << " или 'q' для прекращения просмотра" << std::endl;
          }
          else if( 0 == index )
          {
               ssMsg << "Введите 'n' для отображения следующих " << LINES_NUMBER << " приложений из списка"
                    << " или 'q' для прекращения просмотра" << std::endl;
          }
          else
          {
               ssMsg << "Введите 'p' для отображения предыдущих " << LINES_NUMBER << " приложений из списка"
                    << " или 'n' для следующих " << LINES_NUMBER
                    << ", для прекращения просмотра введите 'q'" << std::endl;
          }

          std::string answer;
          std::cout << ssMsg.str();
          std::cin >> answer;
          std::cin.ignore(); // Очистка потока после std::cin >>

          while( true )
          {
               if( "p" == answer )
               {
                    index -= LINES_NUMBER;
                    break;
               }

               else if( "n" == answer )
               {
                    index += LINES_NUMBER;
                    break;
               }
               else if( "q" == answer )
               {
                    return;
               }
               else
               {
                    std::cout << ssMsg.str();
                    answer.clear();
                    std::cin >> answer;
                    std::cin.ignore(); // Очистка потока после std::cin >>
               }
          }
     }
}

/// @brief Добавить приложение в список запрещенных
void AddApp()
{
     std::string app;
     std::cout << "Введите полное имя приложения или оставьте ввод пустым для отмены" << std::endl;
     std::getline( std::cin, app );
     if( app.empty() )
     {
          std::cout << "Отмена ввода" << std::endl;
          return;
     }

     // Проверка существования
     struct stat st;
     if( 0 != stat( app.c_str(), &st ) )
     {
          std::cerr << "Приложение " << app << " не существует" << std::endl;
          return;
     }

     // Проверка наличия в списке
     if( applications.end() != std::find( applications.begin(), applications.end(), app ) )
     {
          std::cerr << "Приложение " << app << " уже есть в списке" << std::endl;
          return;
     }

     // Тут просто добавили в список, в файл эти изменения сразу не попадают, для этого отдельная команда Update либо
     // перезапись при выходе, дергать файл при каждом добавлении/удалении слишком дорого
     applications.push_back( app );
     std::cout << "Чтобы изменения вступили в силу, обновите конфигурационный файл после изменений" << std::endl;
}

/// @brief Удалить приложение из списка запрещенных
void RemoveApp()
{
     std::string app;
     std::cout << "Введите номер приложения из списка или оставьте ввод пустым для отмены" << std::endl;
     getline( std::cin, app );
     if( app.empty() )
     {
          std::cout << "Отмена ввода" << std::endl;
          return;
     }
     int appNum = 0;
     if( 1 != sscanf( app.c_str(), "%d", &appNum ) || 0 >= appNum )
     {
          std::cerr << "Введено некорректное значение" << std::endl;
          return;
     }
     if( static_cast<int>( applications.size() ) < appNum )
     {
          std::cerr << "Введенный номер отсутствует в списке" << std::endl;
          return;
     }

     applications.erase( applications.begin() + appNum - 1 );

     // Тут просто удалили из списка, в файл эти изменения сразу не попадают, для этого отдельная команда Update либо
     // перезапись при выходе, дергать файл при каждом добавлении/удалении слишком дорого
     std::cout << "Чтобы изменения вступили в силу, обновите конфигурационный файл после изменений" << std::endl;
}

/// @brief Обновить конфиг (перезапись списка)
void Update()
{
     // Перезаписываем конфиг
     WriteAppToFile( CONFIG_NAME );

     int pid = GetSrvcPid();
     if( 0 > pid )
     {
          return;
     }

     // Отправляем команду службе для перечитывания
     if( 0 != kill( pid, SIGUSR2 ) )
     {
          std::cerr << "Ошибка при обновлении списка службы: " << std::strerror( errno );;
          return;
     }
     std::cout << "Конфигурационный файл обновлен успешно" << std::endl;
}

/// @brief Вкл/Выкл службу
void Toggle()
{
     bool needStatus = false;
     bool result = false;
     if( serviceStatus )
     {
          result = ServiceMgmt( "stop" );
     }
     else
     {
          needStatus = true;
          result = ServiceMgmt( "start" );
     }

     std::ostringstream ossCmd;
     ossCmd << "pgrep " << SERVICE_NAME << " > /dev/null";

     serviceStatus = ( 0 == system( ossCmd.str().c_str() ) );
     if( !result || needStatus != serviceStatus )
     {
          std::cerr << "Ошибка " << ( ( needStatus ) ? "запуска" : "останова" ) << " службы" << std::endl;
          return;
     }
     std::cout << "Служба успешно " << ( ( needStatus ) ? "запущена" : "остановлена" ) << std::endl;
}

/// @brief Выход
void Exit()
{
     Update();
     // Бросаем свое исключение, чтобы аккуратно выйти из цикла
     throw ExitException();
}

/// @brief Выполнение команд из UI
/// @param[in] act действие
void DoAction( const Action act )
{
     std::function<void()> action;
     switch( static_cast<int>( act ) )
     {
          case static_cast<int>( Action::list ):       action = [] { ShowList(); }; break;
          case static_cast<int>( Action::add ):        action = [] { AddApp(); }; break;
          case static_cast<int>( Action::remove ):     action = [] { RemoveApp(); }; break;
          case static_cast<int>( Action::update ):     action = [] { Update(); }; break;
          case static_cast<int>( Action::toggle ):     action = [] { Toggle(); }; break;
          case static_cast<int>( Action::exit ):       Exit();

          default: std::cerr << "Запрошена несуществующая команда" << std::endl; return;
     }
     std::cout << "_____________________________________________________________________________________" << std::endl;
     action();
     std::cout << "_____________________________________________________________________________________" << std::endl;
}

int main()
try
{
     if( !CheckPermission( CONFIG_NAME ) )
     {
          return EXIT_FAILURE;
     }

     if( !ReadConfig( CONFIG_NAME ) )
     {
          return EXIT_FAILURE;
     }
     std::ostringstream ossCmd;
     ossCmd << "pgrep " << SERVICE_NAME << " > /dev/null";
     serviceStatus = ( 0 == system( ossCmd.str().c_str() ) );
     std::string answer;
     std::cout << "Выберите действие:" << std::endl;
     while( true )
     {
          answer.clear();
          for( size_t i = 1; i < static_cast<size_t>( Action::actionCnt ); ++i )
          {
               std::cout << i << ") " << ActionToStr( static_cast<Action>( i ), serviceStatus ) << std::endl;
          }
          std::getline( std::cin, answer );
          std::cout << std::endl;
          int act = -1;
          if( 1 == sscanf( answer.c_str(), "%d", &act ) && 0 < act && static_cast<int>( Action::actionCnt ) > act )
          {
               DoAction( static_cast<Action>( act ) );
               std::cout << std::endl;
          }
          else
          {
               std::cerr << "Введите значение от 1 до " << static_cast<size_t>( Action::actionCnt ) - 1 << std::endl;
          }
     }


     return EXIT_SUCCESS;
}
catch( const ExitException& )
{
     return EXIT_SUCCESS;
}
catch( const std::exception& error )
{
     std::cerr << error.what();
     return EXIT_FAILURE;
}
