/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/// \addtogroup Trinityd
/// @{
/// \file

#include "CliRunnable.h"
#include "Config.h"
#include "Util.h"
#include "World.h"

#if TRINITY_PLATFORM != TRINITY_PLATFORM_WINDOWS
#include "Chat.h"
#include "ChatCommand.h"
#include <readline/readline.h>
#include <readline/history.h>
#endif

static constexpr char CLI_PREFIX[] = "TC> ";

static inline void PrintCliPrefix()
{
    printf("%s", CLI_PREFIX);
}

#if TRINITY_PLATFORM != TRINITY_PLATFORM_WINDOWS
char* command_finder(char const* text, int state)
{
    static size_t idx, len;
    char const* ret;
    std::vector<ChatCommand> const& cmd = ChatHandler::getCommandTable();

    if (!state)
    {
        idx = 0;
        len = strlen(text);
    }

    while (idx < cmd.size())
    {
        ret = cmd[idx].Name;
        if (!cmd[idx].AllowConsole)
        {
            ++idx;
            continue;
        }

        ++idx;
        //printf("Checking %s \n", cmd[idx].Name);
        if (strncmp(ret, text, len) == 0)
            return strdup(ret);
    }

    return nullptr;
}

char** cli_completion(char const* text, int start, int /*end*/)
{
    char** matches = nullptr;

    if (start)
        rl_bind_key('\t', rl_abort);
    else
        matches = rl_completion_matches((char*)text, &command_finder);
    return matches;
}

int cli_hook_func()
{
       if (World::IsStopped())
           rl_done = 1;
       return 0;
}

#endif

void utf8print(void* /*arg*/, std::string_view str)
{
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
    std::wstring wbuf;
    if (!Utf8toWStr(str, wbuf))
        return;

    wprintf(L"%s", wbuf.c_str());
#else
{
    printf(STRING_VIEW_FMT, STRING_VIEW_FMT_ARG(str));
    fflush(stdout);
}
#endif
}

void commandFinished(void*, bool /*success*/)
{
    PrintCliPrefix();
    fflush(stdout);
}

#ifdef linux
// Non-blocking keypress detector, when return pressed, return 1, else always return 0
int kb_hit_return()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}
#endif

/// %Thread start
void CliThread()
{
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
    // print this here the first time
    // later it will be printed after command queue updates
    PrintCliPrefix();
#else
    rl_attempted_completion_function = cli_completion;
    rl_event_hook = cli_hook_func;
#endif

    if (sConfigMgr->GetBoolDefault("BeepAtStart", true))
        printf("\a");                                       // \a = Alert

    ///- As long as the World is running (no World::m_stopEvent), get the command line and handle it
    while (!World::IsStopped())
    {
        fflush(stdout);

        std::string command;

#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
        wchar_t commandbuf[256];
        if (fgetws(commandbuf, sizeof(commandbuf), stdin))
        {
            if (!WStrToUtf8(commandbuf, wcslen(commandbuf), command))
            {
                PrintCliPrefix();
                continue;
            }
        }
#else
        char* command_str = readline(CLI_PREFIX);
        rl_bind_key('\t', rl_complete);
        if (command_str != nullptr)
        {
            command = command_str;
            free(command_str);
        }
#endif

        if (!command.empty())
        {
            std::size_t nextLineIndex = command.find_first_of("\r\n");
            if (nextLineIndex != std::string::npos)
            {
                if (nextLineIndex == 0)
                {
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
                    PrintCliPrefix();
#endif
                    continue;
                }

                command.erase(nextLineIndex);
            }

            fflush(stdout);
            sWorld->QueueCliCommand(new CliCommandHolder(nullptr, command.c_str(), &utf8print, &commandFinished));
#if TRINITY_PLATFORM != TRINITY_PLATFORM_WINDOWS
            add_history(command.c_str());
#endif
        }
        else if (feof(stdin))
        {
            World::StopNow(SHUTDOWN_EXIT_CODE);
        }
    }
}
