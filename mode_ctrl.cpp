#include <mode_ctrl.h>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <cerrno>
#include <cstring>

#include <sys/stat.h>

namespace mode_ctrl
{

RuleController::RuleController( const std::string& configPath ) throw ( std::exception )
: configPath_( configPath )
, stop_( false )
, copmlete_( false )
{
     if( !ReadConfig( configPath_ ) )
     {
          throw std::runtime_error( "Read application from configuration file failed" );
     }
     modeSpy_ = std::thread( &RuleController::CheckApps, this );
}

RuleController::~RuleController()
{
     SetOwnMode();
}

void RuleController::SetOwnMode()
{
     for( const auto& it : applications_ )
     {
          if( 0 != chmod( it.first.c_str(), it.second ) )
          {
               std::cerr << "Error while set mode file " << it.first << ", errno " << errno << ": " << std::strerror( errno );
          }
     }
}

void RuleController::WaitThreads()
{
     Stop();
     if( modeSpy_.joinable() )
     {
          modeSpy_.join();
     }
}

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
          std::cerr << "Error while getting mode, errno: " << std::strerror( errno ) << std::endl;
          return false;
     }
}

void RuleController::CheckApps()
{
     while( !stop_ )
     {
          for( const auto& it : applications_ )
          {
               if( 0 != chmod( it.first.c_str(), it.second & ~( S_IXUSR | S_IXGRP | S_IXOTH ) ) )
               {
                    std::cerr << "Error while set mode file " << it.first << ", errno " << errno << ": " << std::strerror( errno );
               }
          }
          std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
     }
}

void RuleController::Stop()
{
     stop_ = true;
}

bool RuleController::ReadConfig( const std::string& path )
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
               mode_t mode = 0;
               if( GetPermissions( buffer, mode ) )
               {
                    applications_.emplace_back( std::make_pair( buffer, static_cast<uint32_t>( mode ) ) );
               }
          }
     }
     return ( !applications_.empty() );
}

} // namespace mode_ctrl
