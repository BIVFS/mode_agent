#ifndef RULE_CTRL_H_
#define RULE_CTRL_H_

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <fstream>
#include <thread>
#include <cerrno>
#include <cstring>
#include <atomic>

#include <sys/stat.h>

namespace mode_ctrl
{

/// @brief Имя службы
constexpr auto MODE_AGENT_NAME = "mode_agent";

/// @brief Полный путь к конфигу
constexpr auto CONFIG_NAME = "/etc/mode_ctrl_config";

/// @brief Основной класс контроля режима доступа
class ModeController
{
public:
     /// @brief Конструктор
     /// @param[in] configPath путь к конфигу
     /// @throw std::exception
     ModeController( const std::string& configPath = CONFIG_NAME ) throw ( std::exception );

     /// @brief Деструктор
     ///
     /// ВНИМАНИЕ!!! Не делать по умолчанию, должен возвращать режим доступа обратно
     ~ModeController();

     /// @brief Останов
     void Stop();

     /// @brief Ожидание завершения работы
     void WaitThreads();

     /// @brief Периожическая проверка режима доступа
     void CheckApps();

     /// @brief Перечитать конфиг и обновить список
     void Update();
private:
     /// @brief Прочитать конфиг
     /// @param[in] path путь к конфигу
     /// @param[out] apps список приложений из конфига
     bool ReadConfig( const std::string& path, std::vector<std::string>& apps );

     /// @brief Установить исходный режим доступа по индексу в списке
     /// @param[in] index индекс приложения в списке
     void SetOwnMode( const size_t index );

     /// @brief Установить исходный режим доступа всех приложений
     void SetOwnMode();

private:
     std::string configPath_;      ///< путь к конфигу
     std::vector<std::pair<std::string, uint32_t>> applications_;     ///< список приложений, представляет собой массив
                                                                      /// пар <полное имя, исходный режим доступа>
     std::thread modeSpy_;         ///< Поток контроля режима доступа
     std::atomic<bool> complete_;  ///< Флаг завершения работы
     std::mutex listGuard_;        ///< Разграничение доступа к списку при асинхронных операциях
};

} // namespace mode_ctrl

#endif // RULE_CTRL_H_
