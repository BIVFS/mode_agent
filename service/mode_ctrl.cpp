#include <mode_ctrl.h>

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <cerrno>
#include <cstring>

#include <sys/stat.h>

namespace mode_ctrl
{

bool GetPermissions( const std::string& file, mode_t& mode )
{
     struct stat st;
     if( stat( file.c_str(), &st ) == 0 )
     {
          mode = st.st_mode;
          return true;
     }
     else
     {
          std::cerr << "Error while getting mode (" << file << "), errno: " << std::strerror( errno ) << std::endl;
          return false;
     }
}

ModeController::ModeController( const std::string& configPath ) throw ( std::exception )
: configPath_( configPath )
, complete_( false )
{
     std::vector<std::string> apps;
     if( !ReadConfig( configPath_, apps ) )
     {
          throw std::runtime_error( "Read application from configuration file failed" );
     }

     for( const auto& it : apps )
     {
          mode_t mode = 0;
          if( GetPermissions( it, mode ) )
          {
               applications_.emplace_back( std::make_pair( it, static_cast<uint32_t>( mode ) ) );
          }
     }
     modeSpy_ = std::thread( &ModeController::CheckApps, this );
}

ModeController::~ModeController()
{
     SetOwnMode();
}

void ModeController::SetOwnMode( const size_t index )
{
     if( 0 != chmod( applications_[index].first.c_str(), applications_[index].second ) )
     {
          std::cerr << "Error while set mode file " << applications_[index].first << ", errno " << errno << ": " << std::strerror( errno );
     }
}

void ModeController::SetOwnMode()
{
     for( const auto& it : applications_ )
     {
          if( 0 != chmod( it.first.c_str(), it.second ) )
          {
               std::cerr << "Error while set mode file " << it.first << ", errno " << errno << ": " << std::strerror( errno );
          }
     }
}

void ModeController::WaitThreads()
{
     Stop();
     if( modeSpy_.joinable() )
     {
          modeSpy_.join();
     }
}

void ModeController::CheckApps()
{
     while( !complete_ )
     {
          // Вынос в отдельный скоп, чтобы отработал RAI lock_guard
          {
               std::lock_guard<std::mutex> lock( listGuard_ );
               for( const auto& it : applications_ )
               {
                    if( 0 != chmod( it.first.c_str(), it.second & ~( S_IXUSR | S_IXGRP | S_IXOTH ) ) )
                    {
                         std::cerr << "Error while set mode file " << it.first << ", errno " << errno << ": " << std::strerror( errno );
                    }
               }
          }
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
     }
}

void ModeController::Update()
{
     std::lock_guard<std::mutex> lock( listGuard_ );
     std::vector<std::string> apps;
     if( !ReadConfig( configPath_, apps ) )
     {
          return;
     }
     if( apps.empty() )
     {
          // Конфиг пуст, возвращаем всех исходный доступ и выходим
          SetOwnMode();
          applications_.clear();
          return;
     }

     // Удаляем приложения, которые убрали из конфига
     for( size_t i = 0; i < applications_.size(); ++i )
     {
          auto itApp = std::find( apps.begin(), apps.end(), applications_[i].first );
          if( apps.end() == itApp )
          {
               SetOwnMode( i );
               applications_.erase( applications_.begin() + i );
          }
     }

     // Теперь выделим новые приложения
     for( const auto& it : applications_ )
     {
          auto itApp = std::find( apps.begin(), apps.end(), it.first );
          if( apps.end() != itApp )
          {
               apps.erase( itApp );
          }
     }

     // Добавляем новые приложения в список
     for( const auto& it : apps )
     {
          mode_t mode = 0;
          if( GetPermissions( it, mode ) )
          {
               applications_.emplace_back( std::make_pair( it, static_cast<uint32_t>( mode ) ) );
          }
     }
}

void ModeController::Stop()
{
     complete_ = true;
}

bool ModeController::ReadConfig( const std::string& path, std::vector<std::string>& apps )
{
     std::ifstream file( path );
     if( !file )
     {
          std::cerr << "Read config " << path << " failed" << std::endl;
          return false;
     }
     std::string buffer;
     while( !file.eof() )
     {
          getline( file, buffer );
          if( !buffer.empty() )
          {
               apps.emplace_back( buffer );
          }
     }
     return true;
}

} // namespace mode_ctrl
