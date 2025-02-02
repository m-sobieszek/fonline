//      __________        ___               ______            _
//     / ____/ __ \____  / (_)___  ___     / ____/___  ____ _(_)___  ___
//    / /_  / / / / __ \/ / / __ \/ _ \   / __/ / __ \/ __ `/ / __ \/ _ `
//   / __/ / /_/ / / / / / / / / /  __/  / /___/ / / / /_/ / / / / /  __/
//  /_/    \____/_/ /_/_/_/_/ /_/\___/  /_____/_/ /_/\__, /_/_/ /_/\___/
//                                                  /____/
// FOnline Engine
// https://fonline.ru
// https://github.com/cvet/fonline
//
// MIT License
//
// Copyright (c) 2006 - 2022, Anton Tsvetinskiy aka cvet <cvet@tut.by>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "Common.h"

#include "Application.h"
#include "Log.h"
#include "Settings.h"

struct ServerScriptSystem
{
    void InitAngelScriptScripting(const char* script_path);
};

struct ClientScriptSystem
{
    void InitAngelScriptScripting(const char* script_path);
};

struct MapperScriptSystem
{
    void InitAngelScriptScripting(const char* script_path);
};

unordered_set<string> CompilerPassedMessages;

#if !FO_TESTING
int main(int argc, char** argv)
#else
[[maybe_unused]] static auto ASCompilerApp(int argc, char** argv) -> int
#endif
{
    try {
        InitApp(argc, argv, "ASCompiler");
        LogWithoutTimestamp();

        auto server_failed = false;
        auto client_failed = false;
        auto mapper_failed = false;

        if (!App->Settings.ASServer.empty()) {
            WriteLog("Compile server scripts at {}", App->Settings.ASServer);

            try {
                ServerScriptSystem().InitAngelScriptScripting(App->Settings.ASServer.c_str());
            }
            catch (std::exception& ex) {
                if (CompilerPassedMessages.empty()) {
                    ReportExceptionAndExit(ex);
                }

                server_failed = true;
            }
        }

        if (!App->Settings.ASClient.empty()) {
            WriteLog("Compile client scripts at {}", App->Settings.ASClient);

            try {
                ClientScriptSystem().InitAngelScriptScripting(App->Settings.ASClient.c_str());
            }
            catch (std::exception& ex) {
                if (CompilerPassedMessages.empty()) {
                    ReportExceptionAndExit(ex);
                }

                client_failed = true;
            }
        }

        if (!App->Settings.ASMapper.empty()) {
            WriteLog("Compile mapper scripts at {}", App->Settings.ASMapper);

            try {
                MapperScriptSystem().InitAngelScriptScripting(App->Settings.ASMapper.c_str());
            }
            catch (std::exception& ex) {
                if (CompilerPassedMessages.empty()) {
                    ReportExceptionAndExit(ex);
                }

                mapper_failed = true;
            }
        }

        if (!App->Settings.ASServer.empty()) {
            WriteLog("Server scripts compilation {}!", server_failed ? "failed" : "succeeded");
        }
        if (!App->Settings.ASClient.empty()) {
            WriteLog("Client scripts compilation {}!", client_failed ? "failed" : "succeeded");
        }
        if (!App->Settings.ASMapper.empty()) {
            WriteLog("Mapper scripts compilation {}!", mapper_failed ? "failed" : "succeeded");
        }
        if (App->Settings.ASServer.empty() && App->Settings.ASClient.empty() && App->Settings.ASMapper.empty()) {
            WriteLog("Nothing to compile!");
        }

        if (server_failed || client_failed || mapper_failed) {
            ExitApp(false);
        }

        ExitApp(true);
    }
    catch (const std::exception& ex) {
        ReportExceptionAndExit(ex);
    }
}
